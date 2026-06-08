# Not Engine

Not Engine은 C++과 OpenGL(GLFW/GLAD) 기반의 경량 3D 게임 엔진 프로젝트입니다.

---

## 📋 우선 개발 TODO 리스트

현재 구현 상태를 바탕으로 엔진이 정상적으로 구동되고 확장될 수 있도록 우선순위별 개발 작업을 정의합니다.

### 1단계: 빌드 환경 및 핵심 프레임워크 구축 (최우선)
- [x] **빌드 시스템(CMake) 설계**: 엔진 라이브러리와 데모 프로그램을 빌드하기 위한 `CMakeLists.txt` 작성 및 외부 의존성(GLFW, GLAD, GLM) 링크 설정
- [x] **`Engine` 관리자 클래스 완성**: 불완전하게 선언된 `Engine` 클래스를 헤더([engine.h](file:///home/sinokadev/notengine/include/knot/engine.h))와 소스([engine.cpp](file:///home/sinokadev/notengine/src/engine.cpp))로 분리하고, 윈도우/렌더러/오브젝트 매니저의 라이프사이클(`init`, `run`, `update`, `render`) 조율 시스템 구축
- [x] **Mesh GPU 버퍼 업로드 기능 구현**: [resources.h](file:///home/sinokadev/notengine/include/knot/resources.h)의 `Mesh` 구조체에 Vertex/Index 데이터를 GPU로 전송하여 VAO/VBO/EBO를 실제로 초기화하는 로직 추가

### 2단계: 렌더링 파이프라인 기초 및 수학 연동
- [ ] **카메라 및 렌더러(Renderer) 기능 강화**: [renderer.cpp](file:///home/sinokadev/notengine/src/renderer.cpp)의 `renderObject`에 MVP(Model-View-Projection) 행렬 변환 및 쉐이더 적용 로직을 연계하여 3D 좌표계 렌더링 지원
- [ ] **수학 라이브러리([math.h](file:///home/sinokadev/notengine/include/knot/math.h)) 작성**: GLM 라이브러리를 활용하거나 호환되는 벡터, 행렬, 쿼터니언 연산 래퍼 정의
- [x] **데모 어플리케이션 제작**: [demo.cpp](file:///home/sinokadev/notengine/demo/demo.cpp)에 GLFW 창을 띄우고 기본적인 3D 셰이프(큐브 등)를 렌더링하는 최소 구동 코드 작성

### 3단계: 리소스 매니저 및 텍스처 시스템
- [ ] **리소스 매니저(ResourceManager) 클래스 개발**: 쉐이더, 메쉬, 텍스처 리소스를 파일에서 로드하고 캐싱하여 중복 생성을 방지하는 에셋 관리자 구축
- [ ] **텍스처(Texture) 지원**: `stb_image` 등의 이미지 로더 라이브러리를 연동하여 텍스처 이미지를 파싱하고 OpenGL 텍스처 버퍼에 업로드하는 클래스 및 쉐이더 연동 구현
- [ ] **입력과 이벤트**