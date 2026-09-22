# Blender Seno Exporter

Blender 4.1 이상용 애드온입니다. Seno v8 장면과 모델별 OBJ·MTL, 이미지 텍스처를 **선택한 폴더에 함께** 저장합니다. 인라인 `meshes`는 만들지 않으며 `models[].obj`로 외부 모델을 참조합니다. 별도 OBJ 애드온이나 Python 패키지는 필요 없습니다.

## 설치와 사용

1. 이 디렉터리의 `seno_exporter.py`를 다운로드합니다.
2. Blender의 **Edit → Preferences → Add-ons → Install from Disk**에서 파일을 선택합니다. 버전에 따라 Install 메뉴는 오른쪽 위 드롭다운에 있습니다.
3. **Seno Scene Exporter**를 활성화합니다.
4. Object Mode에서 **File → Export → Seno Scene (folder)**를 선택합니다.
5. 출력 폴더와 Scene Name을 지정하고 **Export Seno Folder**를 누릅니다.

예시 출력:

```text
export/
  scene.seno
  model_0000_Cube.obj
  model_0000_Cube.mtl
  model_0001_Sphere.obj
  model_0001_Sphere.mtl
  tex_0000_BaseColor.png
```

- **Selected Only**: 선택한 오브젝트만 내보냅니다. 인스턴스는 인스턴서를 선택합니다.
- **OBJ Paths / Relative** (기본): 이동 가능한 폴더입니다. Seno Editor에서는 바로 열 수 있습니다. 현재 엔진은 상대 경로를 실행 디렉터리 기준으로 해석하므로, 엔진에서 직접 로드할 때 출력 폴더를 작업 디렉터리로 지정하세요.
- **OBJ Paths / Absolute**: 현재 저장 위치의 절대 경로를 기록합니다. 엔진 실행 위치에 무관하게 로드되지만 폴더 이동 후에는 다시 내보내야 합니다.
- **Overwrite Existing Files**: 기본값은 꺼져 있습니다. 충돌 시 기존 파일을 변경하지 않고 오류를 표시합니다. 켜면 이번에 생성하는 동명 파일만 교체하며 이전 익스포트의 불필요한 파일을 삭제하지 않습니다. 전용 출력 폴더 사용을 권장합니다.

변환은 임시 디렉터리에서 끝낸 뒤 결과 파일을 이동하고 `.seno`를 마지막으로 저장합니다. 파일 묶음 전체에 대한 원자적 교체는 아니므로 디스크 오류 등으로 이동이 중단되면 일부 파일만 갱신될 수 있습니다.

## 지원 범위

- 현재 프레임·활성 View Layer의 평가된 메시, 모디파이어, 커브·텍스트, depsgraph에서 제공하는 인스턴스.
- Blender Z-up → 엔진 Y-up: `(x, y, z)` → `(x, z, -y)`.
- 위치는 Seno 오브젝트에, 회전·스케일·부모로 인한 shear는 OBJ 정점과 노멀에 반영합니다. 따라서 Seno의 회전/스케일은 identity이고 pivot은 0입니다. 원점은 유지하며 미러링 시 면 방향도 보정합니다.
- 삼각형, 활성 UV 레이어, 모서리 노멀, 여러 재질 슬롯.
- 활성 Material Output에 직접 연결된 Principled BSDF의 Base Color / Metallic / Roughness. 해당 소켓에 직접 연결된 Image Texture와 Normal Map → Image Texture를 MTL에 기록합니다. 외부 이미지는 복사하고 패킹·생성·수정된 이미지는 PNG로 저장합니다.
- 활성 원근/직교 카메라, Point/Sun 조명. 조명 에너지는 수치 그대로 기록하므로 엔진에서 밝기 조정이 필요할 수 있습니다.

애니메이션, 리깅, 월드/HDR 하늘, Area/Spot 조명, 파노라마 카메라, 카메라 lens shift, 알파·투명·방출 재질, 노드 그래프 베이킹, 텍스처 매핑 변환, UDIM은 지원하지 않습니다. 복잡한 재질은 먼저 단일 이미지로 베이크하세요. Metallic/Roughness 이미지의 채널 분리는 지원하지 않습니다. Geometry Nodes 인스턴스가 빠지면 Realize Instances로 메시를 실체화하세요. 엔진과 Blender의 조명·색 관리 차이 때문에 렌더 결과가 완전히 같지는 않습니다.

## 검증

Blender 실행 파일 또는 공식 `bpy` 패키지가 있는 Python 환경에서 실행합니다.

```sh
blender --background --factory-startup --python script/blender_seno/tests/test_export.py
# 또는
python script/blender_seno/tests/test_export.py
```

미러링 및 좌표 변환, 노멀/면 방향, 다중 재질, 텍스처 파일, 외부 OBJ 참조, 선택 내보내기, 덮어쓰기 방지, 카메라·조명 및 애드온 등록을 검사합니다.

평가된 메시의 추출/해제는 [Blender depsgraph API](https://docs.blender.org/api/current/bpy.types.Depsgraph.html)를 사용합니다.
