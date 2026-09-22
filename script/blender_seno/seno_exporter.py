# SPDX-License-Identifier: CC0
"""Install this file as a Blender add-on. No optional OBJ exporter is required."""
bl_info = {
    "name": "Seno Scene Exporter", "author": "Knot contributors",
    "version": (1, 0, 0), "blender": (4, 1, 0),
    "location": "File > Export > Seno Scene (folder)",
    "description": "Export Seno v8 with external OBJ, MTL and textures",
    "category": "Import-Export",
}

import json
import math
from pathlib import Path
import re
import shutil
import tempfile

import bpy
from bpy.props import BoolProperty, EnumProperty, StringProperty
from mathutils import Matrix

# Blender (X, Y, Z) -> engine (X, Z, -Y), a proper rotation.
AXIS = Matrix(((1, 0, 0), (0, 0, 1), (0, -1, 0)))
GEOMETRY_TYPES = {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}


def safe_name(name):
    return re.sub(r'[^A-Za-z0-9_-]+', '_', name).strip('_')[:80] or 'scene'


def numbers(values):
    return ' '.join(format(float(x), '.9g') for x in values)


class Exporter:
    def __init__(self, context, folder, final_folder, relative=True, selected=False):
        self.context = context
        self.folder = Path(folder)
        self.final_folder = Path(final_folder)
        self.relative = relative
        self.selected = selected
        self.images = {}
        self.warnings = set()

    def texture(self, image):
        if image is None:
            return None
        key = image.as_pointer()
        if key in self.images:
            return self.images[key]
        if image.source not in {'FILE', 'GENERATED'}:
            self.warnings.add('UDIM, sequence and movie textures are skipped.')
            return None
        source = Path(bpy.path.abspath(image.filepath, library=image.library))
        ext = source.suffix.lower()
        # The engine uses stb_image; save other formats as PNG.
        copy_file = (source.is_file() and not image.packed_file and not image.is_dirty
                     and ext in {'.png', '.jpg', '.jpeg', '.tga', '.bmp', '.hdr'})
        name = f'tex_{len(self.images):04d}_{safe_name(image.name)}{ext if copy_file else ".png"}'
        target = self.folder / name
        if copy_file:
            shutil.copyfile(source, target)
        else:
            clone = image.copy()
            try:
                # Image.copy() does not reliably copy a dirty/generated pixel buffer.
                clone.pixels.foreach_set(image.pixels[:])
                clone.filepath_raw = str(target)
                clone.file_format = 'PNG'
                clone.save()
            finally:
                bpy.data.images.remove(clone)
        self.images[key] = name
        return name

    def material(self, material, index):
        color, metallic, roughness = (0.8, 0.8, 0.8), 0.0, 0.5
        maps = []
        if material:
            color = material.diffuse_color[:3]
            metallic, roughness = material.metallic, material.roughness
            if material.use_nodes:
                outputs = [n for n in material.node_tree.nodes
                           if n.type == 'OUTPUT_MATERIAL' and n.is_active_output]
                surface = outputs[0].inputs.get('Surface') if outputs else None
                node = surface.links[0].from_node if surface and surface.is_linked else None
                if node and node.type == 'BSDF_PRINCIPLED':
                    color = node.inputs['Base Color'].default_value[:3]
                    metallic = node.inputs['Metallic'].default_value
                    roughness = node.inputs['Roughness'].default_value
                    for socket_name, directive in [('Base Color', 'map_Kd'),
                                                    ('Metallic', 'map_Pm'),
                                                    ('Roughness', 'map_Pr'),
                                                    ('Normal', 'norm')]:
                        socket = node.inputs[socket_name]
                        if not socket.is_linked:
                            continue
                        source = socket.links[0].from_node
                        if source.type == 'NORMAL_MAP' and socket_name == 'Normal':
                            source = (source.inputs['Color'].links[0].from_node
                                      if source.inputs['Color'].is_linked else None)
                        if source and source.type == 'TEX_IMAGE':
                            texture = self.texture(source.image)
                            if texture:
                                maps.append(f'{directive} {texture}')
                        else:
                            self.warnings.add('Procedural/complex material nodes are not baked; using socket defaults.')
                else:
                    self.warnings.add('Non-Principled materials use viewport material values.')
        return '\n'.join([f'newmtl mat_{index}', f'Kd {numbers(color)}',
                          f'Pm {metallic:.9g}', f'Pr {roughness:.9g}',
                          # Engine treats Pr=0 as missing; Ns supplies closest supported gloss.
                          f'Ns {min(1e6, 2 / max(roughness, 0.001)**2 - 2):.9g}',
                          *maps, ''])

    def mesh(self, obj, world, stem, depsgraph):
        mesh = obj.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            if mesh is None or not mesh.polygons:
                return False
            mesh.calc_loop_triangles()
            linear = AXIS @ world.to_3x3()
            if abs(linear.determinant()) < 1e-12:
                self.warnings.add('Zero-scale geometry was skipped.')
                return False
            normal_matrix = linear.inverted().transposed()
            mirrored = linear.determinant() < 0
            uv = mesh.uv_layers.active
            materials = list(mesh.materials) or [None]
            with (self.folder / f'{stem}.mtl').open('w', encoding='utf-8') as out:
                for i, material in enumerate(materials):
                    out.write(self.material(material, i) + '\n')
            with (self.folder / f'{stem}.obj').open('w', encoding='utf-8') as out:
                out.write(f'# Seno external mesh\nmtllib {stem}.mtl\no {stem}\n')
                for vertex in mesh.vertices:
                    out.write('v ' + numbers(linear @ vertex.co) + '\n')
                for loop in mesh.loops:
                    texcoord = uv.data[loop.index].uv if uv else (0, 0)
                    out.write('vt ' + numbers(texcoord) + '\n')
                for normal in mesh.corner_normals:
                    out.write('vn ' + numbers((normal_matrix @ normal.vector).normalized()) + '\n')
                previous = None
                for triangle in mesh.loop_triangles:
                    material = min(triangle.material_index, len(materials) - 1)
                    if material != previous:
                        out.write(f'usemtl mat_{material}\n')
                        previous = material
                    loops = list(triangle.loops)
                    if mirrored:
                        loops.reverse()
                    out.write('f ' + ' '.join(
                        f'{mesh.loops[i].vertex_index + 1}/{i + 1}/{i + 1}'
                        for i in loops) + '\n')
            return True
        finally:
            obj.to_mesh_clear()

    def scene(self):
        scene = {'version': 8, 'objects': [], 'models': [], 'lights': []}
        depsgraph = self.context.evaluated_depsgraph_get()
        for instance in depsgraph.object_instances:
            obj = instance.object
            original = (instance.parent.original if instance.is_instance and instance.parent
                        else obj.original)
            if self.selected and not original.select_get():
                continue
            if original.hide_render or not instance.show_self:
                continue
            world = instance.matrix_world.copy()
            position = list(AXIS @ world.translation)
            rotation = list((AXIS @ world.to_quaternion().to_matrix()).to_quaternion())
            if obj.type in GEOMETRY_TYPES:
                index = len(scene['models'])
                stem = f'model_{index:04d}_{safe_name(obj.name)}'
                if not self.mesh(obj, world, stem, depsgraph):
                    continue
                path = f'{stem}.obj'
                scene['models'].append({'obj': path if self.relative else str(self.final_folder / path)})
                scene['objects'].append({
                    'id': index, 'name': obj.name, 'model': index,
                    'position': position, 'pivot': [0, 0, 0],
                    'rotation': [1, 0, 0, 0], 'scale': [1, 1, 1],
                    'groups': [c.name for c in original.users_collection],
                })
            elif obj.type == 'LIGHT':
                light = obj.data
                entry = {'color': list(light.color), 'intensity': light.energy}
                if light.type == 'SUN':
                    entry.update(type='DirLight', rotation=rotation)
                elif light.type == 'POINT':
                    entry.update(type='PbrPointLight', position=position)
                else:
                    self.warnings.add('Area and spot lights are unsupported and skipped.')
                    continue
                scene['lights'].append(entry)
        camera = self.context.scene.camera
        if camera and (not self.selected or camera.select_get()):
            camera = camera.evaluated_get(depsgraph)
            data = camera.data
            if data.type in {'PERSP', 'ORTHO'}:
                frame = data.view_frame(scene=self.context.scene)
                height = max(v.y for v in frame) - min(v.y for v in frame)
                entry = {'type': 'OrthographicCamera' if data.type == 'ORTHO' else 'PerspectiveCamera',
                         'position': list(AXIS @ camera.matrix_world.translation),
                         'rotation': list((AXIS @ camera.matrix_world.to_quaternion().to_matrix()).to_quaternion()),
                         'near': data.clip_start, 'far': data.clip_end}
                if data.type == 'ORTHO':
                    entry['size'] = height
                else:
                    entry['fov'] = math.degrees(2 * math.atan(height / (2 * abs(frame[0].z))))
                scene['camera'] = entry
                if data.shift_x or data.shift_y:
                    self.warnings.add('Camera lens shift is unsupported.')
        return scene


def export_scene(context, directory, scene_name='scene', relative=True, selected=False, overwrite=False):
    folder = Path(bpy.path.abspath(directory)).expanduser().resolve()
    folder.mkdir(parents=True, exist_ok=True)
    # Finish conversion in staging before touching any existing export.
    with tempfile.TemporaryDirectory(prefix='.seno-', dir=folder) as temp:
        exporter = Exporter(context, temp, folder, relative, selected)
        scene = exporter.scene()
        scene_file = Path(temp) / (safe_name(scene_name) + '.seno')
        scene_file.write_text(json.dumps(scene, ensure_ascii=False, indent=2, allow_nan=False) + '\n', encoding='utf-8')
        files = list(Path(temp).iterdir())
        conflicts = [p.name for p in files if (folder / p.name).exists()]
        if conflicts and not overwrite:
            raise FileExistsError('Files already exist. Enable Overwrite or choose another folder: ' + ', '.join(conflicts[:3]))
        for file in sorted(files, key=lambda p: p.suffix == '.seno'):
            file.replace(folder / file.name)
    return folder / scene_file.name, len(scene['objects']), sorted(exporter.warnings)


class EXPORT_SCENE_OT_seno(bpy.types.Operator):
    bl_idname = 'export_scene.seno'
    bl_label = 'Export Seno Folder'
    bl_options = {'PRESET'}

    directory: StringProperty(name='Export Folder', subtype='DIR_PATH')
    filter_folder: BoolProperty(default=True, options={'HIDDEN'})
    scene_name: StringProperty(name='Scene Name', default='scene')
    selected_only: BoolProperty(name='Selected Only', default=False)
    overwrite: BoolProperty(name='Overwrite Existing Files', default=False)
    path_mode: EnumProperty(name='OBJ Paths', items=[
        ('RELATIVE', 'Relative', 'Portable folder; OBJ paths are relative to the Seno scene file'),
        ('ABSOLUTE', 'Absolute', 'Load from any working directory; re-export after moving folder'),
    ], default='RELATIVE')

    def invoke(self, context, event):
        self.scene_name = Path(bpy.data.filepath).stem if bpy.data.filepath else 'scene'
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

    def execute(self, context):
        if not self.directory:
            self.report({'ERROR'}, 'Choose an export folder.')
            return {'CANCELLED'}
        if context.mode != 'OBJECT':
            self.report({'ERROR'}, 'Switch to Object Mode before exporting.')
            return {'CANCELLED'}
        try:
            path, count, warnings = export_scene(context, self.directory, self.scene_name,
                self.path_mode == 'RELATIVE', self.selected_only, self.overwrite)
        except Exception as error:
            self.report({'ERROR'}, f'Seno export failed: {error}')
            return {'CANCELLED'}
        for warning in warnings:
            self.report({'WARNING'}, warning)
        self.report({'INFO'}, f'Exported {count} objects: {path}')
        return {'FINISHED'}


def menu_export(self, context):
    self.layout.operator(EXPORT_SCENE_OT_seno.bl_idname, text='Seno Scene (folder)')


def register():
    bpy.utils.register_class(EXPORT_SCENE_OT_seno)
    bpy.types.TOPBAR_MT_file_export.append(menu_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_export)
    bpy.utils.unregister_class(EXPORT_SCENE_OT_seno)


if __name__ == '__main__':
    register()
