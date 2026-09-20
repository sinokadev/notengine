# Seno Editor

와 GPT 6 진짜 좋네요

이건 그냥 만들어본거고, 업데이트 할 생각을 딱히 없어요.

이건 처음부터 끝까지 GPT 6 Codex가 만들었어요.

Knot의 `.seno` v7 장면을 편집하는 C++17 / Dear ImGui 데스크톱 에디터입니다.
엔진 코드를 변경하지 않고 이 디렉터리만 추가했습니다. 3D 미리보기에는 Knot의 실제 렌더러를 사용합니다.

## 빌드 및 실행

저장소 루트에서 실행합니다. CMake, C++17 컴파일러, GLFW 개발 패키지, OpenGL 4.3을 지원하는 GPU/드라이버가 필요합니다.
Linux에서 검증했습니다. macOS 기본 OpenGL은 4.3을 지원하지 않습니다.
최초 구성 때 Dear ImGui v1.91.9b의 고정 커밋을 GitHub에서 받습니다.

```sh
cmake -S seno-editor -B build/seno-editor -DCMAKE_BUILD_TYPE=Release
cmake --build build/seno-editor -j 4
./build/seno-editor/seno-editor assets/scene.seno
```

인자 없이 실행하면 빈 장면을 만듭니다. 이미 ImGui 소스가 있다면 구성 시
`-DFETCHCONTENT_SOURCE_DIR_IMGUI=/absolute/path/to/imgui`로 지정하여 오프라인 빌드할 수 있습니다.
기존 엔진 빌드와 별도의 빌드 디렉터리를 사용하세요.

## 사용법

- **왼쪽 목록**: objects / lights / materials / models / meshes를 선택하고 검색합니다. Add, Duplicate, Delete를 제공합니다. 새 장면은 meshes → materials → models → objects 순서로 구성하세요. 기본 메시 경로는 저장소의 `assets/notbox.obj`입니다.
- **오른쪽 Inspector**: 위치, 크기, 회전, 그룹, 모델 인덱스, 조명, PBR 재질을 편집합니다. 회전은 엔진과 동일한 **w, x, y, z 쿼터니언** 순서입니다. 숫자를 드래그하거나 Ctrl+클릭으로 직접 입력할 수 있습니다. 드래그 한 번을 변경 취소 한 단계로 기록합니다.
- **3D Preview**: Blender 기본 키맵처럼 가운데 버튼 드래그로 회전, Shift+가운데 버튼 드래그로 화면 기준 이동, 휠 또는 Ctrl+가운데 버튼 드래그로 확대/축소합니다. 뷰포트에서 시작한 드래그는 패널 경계를 넘어가도 버튼을 놓을 때까지 이어집니다. 오브젝트 선택 후 F를 누르면 해당 위치를 중심으로 봅니다. Auto는 속성 편집 완료 후 장면을 다시 로드합니다. 무거운 장면에서는 끄고 Refresh preview를 사용하세요.
- **Scene settings**: HDR 하늘과 저장할 장면 카메라를 편집합니다. 화면 탐색용 카메라는 파일의 카메라를 변경하지 않습니다.
- **JSON Source**: 전체 JSON을 직접 편집하고 Apply JSON으로 검증·적용합니다. 임의 필드 추가, 필드 삭제, 큰 embedded mesh 편집은 여기에서 합니다. 미적용 JSON이 있을 때 Inspector는 잠깁니다. Save 역시 JSON을 검증하고 적용합니다.
- **파일 브라우저**: 디렉터리와 파일 경로를 직접 입력하거나 목록에서 선택합니다. 기존 파일 덮어쓰기, 미저장 상태의 새 문서·열기·종료는 확인 창을 표시합니다.

| 단축키 | 동작 |
|---|---|
| Ctrl+N / Ctrl+O | 새 장면 / 열기 |
| Ctrl+S / Ctrl+Shift+S | 저장 / 다른 이름으로 저장 |
| Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z | 실행 취소 / 다시 실행 |
| F / 숫자패드 . | 선택 오브젝트에 초점 맞추기 |
| 숫자패드 1 / 3 / 7 | 정면 / 우측 / 위쪽 직교 시점 |
| Ctrl+숫자패드 1 / 3 / 7 | 후면 / 좌측 / 아래쪽 직교 시점 |
| 숫자패드 5 | 원근 / 직교 투영 전환 |
| 숫자패드 2 / 4 / 6 / 8 | 15도씩 시점 회전 |
| 숫자패드 9 | 반대쪽 시점 |
| 숫자패드 + / - | 확대 / 축소 |

숫자패드 조작은 마우스가 뷰포트 위에 있을 때 적용됩니다. 이 엔진은 Y-up이므로 위쪽 시점은 +Y, 정면은 +Z입니다. Blender의 Z-up 좌표계를 파일에 적용하지는 않습니다.

텍스트 입력 중 Ctrl+Z는 해당 입력칸의 편집을 취소합니다. 문서 전체 실행 취소는 Edit 메뉴에서도 가능합니다.
문서 실행 취소는 최대 100단계이며, 저장 시 알 수 없는 JSON 필드도 유지합니다. JSON 공백과 키 순서는 정리됩니다.
잘못된 모델/메시/재질 인덱스, 중복 ID, 벡터 형식 등은 적용 전에 검사합니다.
참조 중인 리소스 삭제는 차단하고, 미참조 리소스 삭제 시 뒤쪽 인덱스를 보정합니다.

`{assetRoot}`는 이 저장소 루트입니다. 미리보기에서 상대 에셋 경로는 `.seno` 파일의 디렉터리를 기준으로 해석합니다.
다른 디렉터리로 Save As할 때 상대 경로는 자동 재작성되지 않으므로 JSON에서 조정하거나 `{assetRoot}` 또는 절대 경로를 사용하세요.
엔진에서 독립적으로 실행할 때에는 엔진의 상대 경로 규칙(실행 디렉터리 기준)을 따릅니다.
파일 저장은 같은 디렉터리의 임시 파일을 완성한 뒤 rename합니다. 외부 변경이 발견되면 덮어쓰기 확인을 받습니다.
Windows에서 기존 파일의 rename 교체를 파일시스템이 거부하면 원본을 보존하고 오류를 표시합니다. 새 파일명으로 Save As할 수 있습니다.

한글 표시에는 시스템의 Noto Sans CJK 또는 Windows 맑은 고딕을 사용합니다.
다른 폰트는 `SENO_FONT=/path/to/font.ttf` 환경 변수로 지정하세요.

## 검증

```sh
ctest --test-dir build/seno-editor --output-on-failure
# 실제 디스플레이 연결이 필요하며, 숨겨진 창에서 5프레임 렌더 후 종료합니다.
./build/seno-editor/seno-editor --smoke-test assets/scene.seno
# 선택적으로 마지막 프레임을 PPM으로 저장합니다.
./build/seno-editor/seno-editor --smoke-test assets/scene.seno /tmp/seno.ppm
```

문서 테스트는 그래픽 의존성 없이도 실행할 수 있습니다.

```sh
cmake -S seno-editor -B build/seno-tests -DSENO_BUILD_GUI=OFF
cmake --build build/seno-tests
ctest --test-dir build/seno-tests --output-on-failure
```

미리보기 로드 실패 시 마지막 성공 장면을 유지하고 오류를 표시합니다. 상세 에셋 로드 오류는 터미널을 확인하세요.
이 에디터는 장면/속성 편집 도구이며, 메시 모델링·애니메이션 편집·뷰포트 변환 기즈모는 포함하지 않습니다.
