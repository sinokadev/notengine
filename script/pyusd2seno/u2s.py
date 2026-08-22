#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path

from pxr import (
    Gf,
    Usd,
    UsdGeom,
    UsdLux,
    UsdShade,
)


# ============================================================
# Utility
# ============================================================

def vec3(value, default=(0.0, 0.0, 0.0)):
    if value is None:
        return list(default)

    return [
        float(value[0]),
        float(value[1]),
        float(value[2]),
    ]


def quat(value):
    """
    Seno rotation format:
        [w, x, y, z]

    USD/Gf quaternion:
        Gf.Quatd(real, imaginary)
    """

    if value is None:
        return [1.0, 0.0, 0.0, 0.0]

    return [
        float(value.GetReal()),
        float(value.GetImaginary()[0]),
        float(value.GetImaginary()[1]),
        float(value.GetImaginary()[2]),
    ]


def matrix_to_transform(matrix):
    """
    Extract translation / rotation / scale from a USD transform.
    """

    translation = matrix.ExtractTranslation()
    rotation = matrix.ExtractRotation()

    # ExtractRotation() gives a Gf.Rotation.
    # USD matrices may contain non-uniform scale, so calculate
    # scale from the basis vectors.
    x = Gf.Vec3d(matrix[0][0], matrix[0][1], matrix[0][2])
    y = Gf.Vec3d(matrix[1][0], matrix[1][1], matrix[1][2])
    z = Gf.Vec3d(matrix[2][0], matrix[2][1], matrix[2][2])

    scale = [
        x.GetLength(),
        y.GetLength(),
        z.GetLength(),
    ]

    return {
        "position": vec3(translation),
        "scale": scale,
        "rotation": quat(rotation.GetQuat()),
    }


def get_world_transform(prim):
    xformable = UsdGeom.Xformable(prim)

    if not xformable:
        return {
            "position": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "rotation": [1.0, 0.0, 0.0, 0.0],
        }

    matrix = xformable.ComputeLocalToWorldTransform(
        Usd.TimeCode.Default()
    )

    return matrix_to_transform(matrix)


def resolve_asset_path(stage, path):
    """
    Convert USD asset path into something usable by Seno.

    Example:
        /project/assets/foo.obj

    becomes:
        {assetRoot}/assets/foo.obj
    """

    if not path:
        return path

    resolved = stage.ResolveIdentifierToEditTarget(path)

    if not resolved:
        resolved = path

    resolved = os.path.normpath(resolved)

    # Try to express the path relative to the USD file.
    root = os.path.dirname(stage.GetRootLayer().realPath)

    try:
        relative = os.path.relpath(resolved, root)
    except ValueError:
        relative = resolved

    relative = relative.replace("\\", "/")

    if not relative.startswith("."):
        return "{assetRoot}/" + relative

    return relative


# ============================================================
# Material
# ============================================================

def export_material(material):
    """
    Convert UsdShade material to Seno PBR material.

    Expected Seno:

    {
        "shader": "pbrShader",
        "albedo": [...],
        "metallic": ...,
        "roughness": ...,
        "ao": ...
    }
    """

    result = {
        "shader": "pbrShader",
        "albedo": [1.0, 1.0, 1.0],
        "metallic": 0.0,
        "roughness": 0.5,
        "ao": 1.0,
    }

    if not material:
        return result

    surface = material.ComputeSurfaceSource()

    if not surface:
        return result

    shader = surface[0]

    if not shader:
        return result

    def get_input(name):
        inp = shader.GetInput(name)

        if not inp:
            return None

        value = inp.Get()

        return value

    # Common USD Preview Surface names.
    #
    # diffuseColor -> albedo
    diffuse = get_input("diffuseColor")

    if diffuse is not None:
        result["albedo"] = vec3(diffuse)

    metallic = get_input("metallic")

    if metallic is not None:
        result["metallic"] = float(metallic)

    roughness = get_input("roughness")

    if roughness is not None:
        result["roughness"] = float(roughness)

    occlusion = get_input("occlusion")

    if occlusion is not None:
        result["ao"] = float(occlusion)

    return result


# ============================================================
# Mesh
# ============================================================

def export_mesh(stage, prim):
    """
    Export UsdGeomMesh as embedded Seno mesh.

    Seno embedded mesh:

    {
        "type": "embedded",
        "vertices": [
            {
                "position": [...],
                "normal": [...],
                "texcoord": [...],
                "tangent": [...]
            }
        ],
        "indices": [...]
    }
    """

    mesh = UsdGeom.Mesh(prim)

    points = mesh.GetPointsAttr().Get()

    if points is None:
        return {
            "type": "embedded",
            "vertices": [],
            "indices": [],
        }

    normals = mesh.GetNormalsAttr().Get()

    # --------------------------------------------------------
    # UV
    # --------------------------------------------------------

    texcoords = None

    primvars = UsdGeom.PrimvarsAPI(mesh)

    for name in ("st", "uv", "texcoord"):
        pv = primvars.GetPrimvar(name)

        if pv and pv.HasValue():
            texcoords = pv.Get()
            break

    # --------------------------------------------------------
    # Tangents
    # --------------------------------------------------------

    tangents = None

    for name in ("tangent", "tangents"):
        pv = primvars.GetPrimvar(name)

        if pv and pv.HasValue():
            tangents = pv.Get()
            break

    # --------------------------------------------------------
    # Build vertex list
    # --------------------------------------------------------

    vertices = []

    for i, position in enumerate(points):

        vertex = {
            "position": vec3(position),
            "normal": [0.0, 1.0, 0.0],
            "texcoord": [0.0, 0.0],
            "tangent": [1.0, 0.0, 0.0],
        }

        if normals is not None and i < len(normals):
            vertex["normal"] = vec3(normals[i])

        if texcoords is not None and i < len(texcoords):
            uv = texcoords[i]

            vertex["texcoord"] = [
                float(uv[0]),
                float(uv[1]),
            ]

        if tangents is not None and i < len(tangents):
            vertex["tangent"] = vec3(tangents[i])

        vertices.append(vertex)

    # --------------------------------------------------------
    # USD face topology
    # --------------------------------------------------------

    face_counts = mesh.GetFaceVertexCountsAttr().Get()
    face_indices = mesh.GetFaceVertexIndicesAttr().Get()

    indices = []

    if face_counts and face_indices:

        cursor = 0

        for count in face_counts:

            face = face_indices[cursor:cursor + count]

            cursor += count

            # Seno currently expects triangles.
            #
            # Triangulate polygons using a simple fan:
            #
            #   0---1
            #   |\  |
            #   | \ |
            #   |  \|
            #   3---2
            #
            # polygon [0,1,2,3]
            # =>
            # [0,1,2], [0,2,3]

            if count < 3:
                continue

            for i in range(1, count - 1):
                indices.extend([
                    face[0],
                    face[i],
                    face[i + 1],
                ])

    return {
        "type": "embedded",
        "vertices": vertices,
        "indices": [int(i) for i in indices],
    }


# ============================================================
# External mesh reference
# ============================================================

def get_mesh_asset_reference(prim):
    """
    Try to determine whether a mesh is backed by an external
    asset rather than being native USD geometry.

    For normal UsdGeom.Mesh this usually returns None.
    """

    mesh = UsdGeom.Mesh(prim)

    if not mesh:
        return None

    # Some pipelines put the source asset in custom metadata.
    for key in (
        "assetPath",
        "sourceAsset",
        "senoAsset",
    ):
        value = prim.GetCustomDataByKey(key)

        if value:
            return value

    return None


# ============================================================
# Lights
# ============================================================

def export_light(stage, prim):
    """
    Convert USD lights to Seno lights.
    """

    transform = get_world_transform(prim)

    # --------------------------------------------------------
    # Directional
    # --------------------------------------------------------

    if prim.IsA(UsdLux.DistantLight):

        light = UsdLux.DistantLight(prim)

        color = light.GetColorAttr().Get()
        intensity = light.GetIntensityAttr().Get()

        return {
            "type": "DirLight",

            # Seno directional light uses quaternion rotation.
            "rotation": transform["rotation"],

            "color": vec3(color, (1.0, 1.0, 1.0)),
            "intensity": float(
                intensity if intensity is not None else 1.0
            ),

            "ambient": [0.0, 0.0, 0.0],
            "diffuse": [1.0, 1.0, 1.0],
            "specular": [1.0, 1.0, 1.0],
        }

    # --------------------------------------------------------
    # Point
    # --------------------------------------------------------

    if prim.IsA(UsdLux.SphereLight):

        light = UsdLux.SphereLight(prim)

        color = light.GetColorAttr().Get()
        intensity = light.GetIntensityAttr().Get()

        return {
            "type": "PbrPointLight",
            "position": transform["position"],
            "color": vec3(color, (1.0, 1.0, 1.0)),
            "intensity": float(
                intensity if intensity is not None else 1.0
            ),
        }

    # --------------------------------------------------------
    # Legacy PointLight
    # --------------------------------------------------------

    # Current OpenUSD releases use SphereLight for point lights.  Some older
    # bindings exposed PointLight as a separate schema, so only access it when
    # the installed USD version provides that symbol.
    point_light_type = getattr(UsdLux, "PointLight", None)

    if point_light_type is not None and prim.IsA(point_light_type):

        light = point_light_type(prim)

        color = light.GetColorAttr().Get()
        intensity = light.GetIntensityAttr().Get()

        return {
            "type": "PbrPointLight",
            "position": transform["position"],
            "color": vec3(color, (1.0, 1.0, 1.0)),
            "intensity": float(
                intensity if intensity is not None else 1.0
            ),
        }

    return None


# ============================================================
# Camera
# ============================================================

def export_camera(prim):
    camera = UsdGeom.Camera(prim)

    transform = get_world_transform(prim)

    position = transform["position"]

    rotation = transform["rotation"]

    # USD camera uses focalLength + horizontalAperture.
    focal_length = camera.GetFocalLengthAttr().Get()
    aperture = camera.GetHorizontalApertureAttr().Get()

    if focal_length is None:
        focal_length = 50.0

    if aperture is None:
        aperture = 36.0

    # Approximate horizontal FOV.
    import math

    fov = 2.0 * math.degrees(
        math.atan(
            (float(aperture) * 0.5) /
            float(focal_length)
        )
    )

    # Seno wants yaw/pitch.
    #
    q = Gf.Quatd(
        rotation[0],
        Gf.Vec3d(
            rotation[1],
            rotation[2],
            rotation[3],
        )
    )

    rot = Gf.Rotation(q)

    # USD camera looks down -Z.
    forward = rot.TransformDir(Gf.Vec3d(0.0, 0.0, -1.0))

    yaw = math.degrees(
        math.atan2(forward[2], forward[0])
    )

    pitch = math.degrees(
        math.asin(
            max(-1.0, min(1.0, forward[1]))
        )
    )

    return {
        "type": "MovingCamera",
        "position": position,
        "yaw": float(yaw),
        "pitch": float(pitch),
        "fov": float(fov),
        "near": 0.1,
        "far": 100.0,
    }


# ============================================================
# Main exporter
# ============================================================

class SenoExporter:

    def __init__(self, usd_path):
        self.usd_path = os.path.abspath(usd_path)

        self.stage = Usd.Stage.Open(self.usd_path)

        if not self.stage:
            raise RuntimeError(
                f"Failed to open USD: {usd_path}"
            )

        self.result = {
            "version": 3,

            "sky": {
                "type": "HDR",
                "path": "",
            },

            "objects": [],
            "lights": [],
            "models": [],
            "meshes": [],
            "materials": [],
            "camera": {
                "type": "MovingCamera",
                "position": [0.0, 0.0, 0.0],
                "yaw": 0.0,
                "pitch": 0.0,
                "fov": 45.0,
                "near": 0.1,
                "far": 100.0,
            },
        }

        # USD prim path -> Seno mesh index
        self.mesh_map = {}

        # USD material path -> Seno material index
        self.material_map = {}

        # USD mesh -> Seno model index
        self.model_map = {}

    # --------------------------------------------------------
    # Materials
    # --------------------------------------------------------

    def collect_materials(self):

        for prim in self.stage.Traverse():

            if not prim.IsA(UsdShade.Material):
                continue

            material = UsdShade.Material(prim)

            index = len(self.result["materials"])

            self.material_map[str(prim.GetPath())] = index

            self.result["materials"].append(
                export_material(material)
            )

    # --------------------------------------------------------
    # Meshes
    # --------------------------------------------------------

    def collect_meshes(self):

        for prim in self.stage.Traverse():

            if not prim.IsA(UsdGeom.Mesh):
                continue

            path = str(prim.GetPath())

            if path in self.mesh_map:
                continue

            asset = get_mesh_asset_reference(prim)

            if asset:
                mesh = resolve_asset_path(
                    self.stage,
                    asset
                )

            else:
                mesh = export_mesh(
                    self.stage,
                    prim
                )

            index = len(self.result["meshes"])

            self.mesh_map[path] = index

            self.result["meshes"].append(mesh)

    # --------------------------------------------------------
    # Material binding
    # --------------------------------------------------------

    def get_material_index(self, prim):

        binding = UsdShade.MaterialBindingAPI(
            prim
        ).ComputeBoundMaterial()

        material = binding[0]

        if not material:
            return 0

        path = str(material.GetPath())

        if path not in self.material_map:

            index = len(self.result["materials"])

            self.material_map[path] = index

            self.result["materials"].append(
                export_material(material)
            )

        return self.material_map[path]

    # --------------------------------------------------------
    # Models
    # --------------------------------------------------------

    def collect_models(self):

        for prim in self.stage.Traverse():

            if not prim.IsA(UsdGeom.Mesh):
                continue

            mesh_path = str(prim.GetPath())

            mesh_index = self.mesh_map[mesh_path]

            material_index = self.get_material_index(
                prim
            )

            # A model is basically a mesh/material pair.
            key = (
                mesh_index,
                material_index,
            )

            if key in self.model_map:
                continue

            model_index = len(
                self.result["models"]
            )

            self.model_map[key] = model_index

            self.result["models"].append({
                "mesh": mesh_index,
                "material": material_index,
            })

    # --------------------------------------------------------
    # Objects
    # --------------------------------------------------------

    def collect_objects(self):

        for prim in self.stage.Traverse():

            if not prim.IsA(UsdGeom.Mesh):
                continue

            mesh_path = str(prim.GetPath())

            mesh_index = self.mesh_map[mesh_path]

            material_index = self.get_material_index(
                prim
            )

            model_index = self.model_map[
                (mesh_index, material_index)
            ]

            transform = get_world_transform(prim)

            self.result["objects"].append({
                "position": transform["position"],
                "scale": transform["scale"],
                "rotation": transform["rotation"],
                "model": model_index,
            })

    # --------------------------------------------------------
    # Lights
    # --------------------------------------------------------

    def collect_lights(self):

        for prim in self.stage.Traverse():

            light = export_light(
                self.stage,
                prim
            )

            if light is not None:
                self.result["lights"].append(light)

    # --------------------------------------------------------
    # Camera
    # --------------------------------------------------------

    def collect_camera(self):

        # Prefer the active camera.
        camera_path = None

        try:
            camera_path = self.stage.GetCameraPrimPath(
                Usd.TimeCode.Default()
            )
        except Exception:
            camera_path = None

        if camera_path and not camera_path.IsEmpty():

            prim = self.stage.GetPrimAtPath(
                camera_path
            )

            if prim and prim.IsA(UsdGeom.Camera):
                self.result["camera"] = export_camera(
                    prim
                )
                return

        # Fallback: first camera.
        for prim in self.stage.Traverse():

            if prim.IsA(UsdGeom.Camera):

                self.result["camera"] = export_camera(
                    prim
                )

                return

    # --------------------------------------------------------
    # Sky
    # --------------------------------------------------------

    def collect_sky(self):

        # Look for a custom Seno HDR attribute first.
        root = self.stage.GetPseudoRoot()

        for key in (
            "seno:sky",
            "seno:skyPath",
            "sky",
            "skyPath",
        ):
            value = root.GetCustomDataByKey(key)

            if value:

                self.result["sky"] = {
                    "type": "HDR",
                    "path": resolve_asset_path(
                        self.stage,
                        str(value),
                    ),
                }

                return

        # Also check dome lights.
        for prim in self.stage.Traverse():

            if not prim.IsA(UsdLux.DomeLight):
                continue

            dome = UsdLux.DomeLight(prim)

            texture = dome.GetTextureFileAttr().Get()

            if texture:

                self.result["sky"] = {
                    "type": "HDR",
                    "path": resolve_asset_path(
                        self.stage,
                        str(texture),
                    ),
                }

                return

    # --------------------------------------------------------
    # Export
    # --------------------------------------------------------

    def export(self):

        # A composed USD can open successfully even when referenced layers
        # cannot be resolved.  In that case traversal may still find a camera
        # but no mesh or light prims, producing a misleading, nearly empty
        # Seno file.  Report the composition failure before exporting.
        composition_errors = self.stage.GetCompositionErrors()

        if composition_errors:
            example = str(composition_errors[0]).splitlines()[0]
            raise RuntimeError(
                "USD composition failed "
                f"({len(composition_errors)} error(s)). "
                "Make all referenced USD assets available relative to the "
                f"input file, or flatten the stage before conversion. "
                f"First error: {example}"
            )

        self.collect_sky()

        self.collect_materials()

        self.collect_meshes()

        self.collect_models()

        self.collect_objects()

        self.collect_lights()

        self.collect_camera()

        return self.result


# ============================================================
# CLI
# ============================================================

def main():

    parser = argparse.ArgumentParser(
        description="Convert OpenUSD scene to Seno JSON."
    )

    parser.add_argument(
        "input",
        help="Input USD/USDA/USDC file",
    )

    parser.add_argument(
        "output",
        help="Output Seno JSON file",
    )

    args = parser.parse_args()

    exporter = SenoExporter(
        args.input
    )

    seno = exporter.export()

    output_path = Path(args.output)

    output_path.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    with open(
        output_path,
        "w",
        encoding="utf-8"
    ) as f:

        json.dump(
            seno,
            f,
            indent=4,
            ensure_ascii=False,
        )

        f.write("\n")

    print(
        f"Converted USD -> Seno: "
        f"{args.input} -> {args.output}"
    )


if __name__ == "__main__":
    main()
