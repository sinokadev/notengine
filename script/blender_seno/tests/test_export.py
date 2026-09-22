"""Run: blender --background --factory-startup --python tests/test_export.py
Or run with a Python environment containing the official bpy package.
"""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

import bpy
from mathutils import Vector

MODULE = Path(__file__).resolve().parents[1] / 'seno_exporter.py'
spec = importlib.util.spec_from_file_location('seno_exporter', MODULE)
exporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exporter)


class ExportTest(unittest.TestCase):
    def setUp(self):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.mesh.primitive_cube_add(location=(1, 2, 3))
        self.obj = bpy.context.object
        self.obj.name = 'Cube / 한글'
        self.obj.scale = (-2, 3, 0.5)
        self.obj.rotation_euler = (0.1, 0.2, 0.3)
        material = bpy.data.materials.new('Paint')
        material.use_nodes = True
        material.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value = 0.25
        image = bpy.data.images.new('Packed test', width=2, height=2)
        image.generated_color = (0.2, 0.4, 0.8, 1)
        tex = material.node_tree.nodes.new('ShaderNodeTexImage')
        tex.image = image
        material.node_tree.links.new(tex.outputs['Color'], material.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
        self.obj.data.materials.append(material)
        self.obj.data.materials.append(bpy.data.materials.new('Second'))
        self.obj.data.polygons[0].material_index = 1
        modifier = self.obj.modifiers.new('Triangulate', 'TRIANGULATE')
        bpy.context.view_layer.update()

    def test_bundle_and_geometry(self):
        with tempfile.TemporaryDirectory() as folder:
            path, count, warnings = exporter.export_scene(bpy.context, folder)
            self.assertEqual(count, 1)
            scene = json.loads(path.read_text())
            self.assertEqual(scene['version'], 8)
            self.assertNotIn('meshes', scene)
            self.assertEqual(scene['objects'][0]['position'], [1, 3, -2])
            model = Path(folder) / scene['models'][0]['obj']
            lines = model.read_text().splitlines()
            vertices = [Vector(tuple(map(float, line.split()[1:]))) for line in lines if line.startswith('v ')]
            expected = exporter.AXIS @ (self.obj.matrix_world @ self.obj.data.vertices[0].co)
            actual = vertices[0] + Vector(scene['objects'][0]['position'])
            self.assertLess((expected - actual).length, 1e-6)
            normals = [Vector(tuple(map(float, line.split()[1:]))) for line in lines if line.startswith('vn ')]
            faces = [line.split()[1:] for line in lines if line.startswith('f ')]
            self.assertEqual(len(faces), 12)
            for face in faces:
                indices = [list(map(int, vertex.split('/'))) for vertex in face]
                a, b, c = [vertices[index[0] - 1] for index in indices]
                normal = normals[indices[0][2] - 1]
                self.assertGreater((b - a).cross(c - a).normalized().dot(normal), 0.99)
            self.assertIn('usemtl mat_0', lines)
            self.assertIn('usemtl mat_1', lines)
            mtl = model.with_suffix('.mtl').read_text()
            self.assertIn('Pr 0.25', mtl)
            texture = next(line.split()[1] for line in mtl.splitlines() if line.startswith('map_Kd '))
            self.assertTrue((Path(folder) / texture).is_file())
            before = {p.name: p.read_bytes() for p in Path(folder).iterdir()}
            with self.assertRaises(FileExistsError):
                exporter.export_scene(bpy.context, folder)
            self.assertEqual(before, {p.name: p.read_bytes() for p in Path(folder).iterdir()})
            exporter.export_scene(bpy.context, folder, overwrite=True, relative=False)
            self.assertTrue(Path(json.loads(path.read_text())['models'][0]['obj']).is_absolute())

    def test_selection_and_registration(self):
        exporter.register()
        try:
            self.assertTrue(hasattr(bpy.ops.export_scene, 'seno'))
            self.obj.select_set(False)
            with tempfile.TemporaryDirectory() as folder:
                path, count, _ = exporter.export_scene(bpy.context, folder, selected=True)
                self.assertEqual(count, 0)
                self.assertEqual(json.loads(path.read_text())['models'], [])
        finally:
            exporter.unregister()

    def test_camera_and_light(self):
        bpy.ops.object.camera_add(location=(0, -5, 2))
        bpy.context.scene.camera = bpy.context.object
        bpy.ops.object.light_add(type='SUN', location=(0, 0, 5))
        bpy.context.object.data.energy = 2
        with tempfile.TemporaryDirectory() as folder:
            path, _, _ = exporter.export_scene(bpy.context, folder)
            scene = json.loads(path.read_text())
            self.assertEqual(scene['camera']['position'], [0, 2, 5])
            self.assertGreater(scene['camera']['fov'], 0)
            self.assertEqual(scene['lights'][0]['type'], 'DirLight')
            self.assertEqual(scene['lights'][0]['intensity'], 2)


if __name__ == '__main__':
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ExportTest))
    if not result.wasSuccessful():
        raise SystemExit(1)
