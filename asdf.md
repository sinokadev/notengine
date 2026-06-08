# Not Engine - 게임 엔진 아키텍처 및 코드 분석 보고서

C++ 및 OpenGL 기반의 경량 3D 게임 엔진 프로젝트인 **Not Engine**에 대한 종합적인 기술 평가 보고서입니다. 엔진의 설계 구조, 메모리 안정성, 기능 구현 상태 및 빌드 시스템을 중심으로 분석하였습니다.

---

## 📌 1. 아키텍처 및 메모리 안정성 분석 (가장 중요)

### ⚠️ 치명적 위험: `ObjectManager`의 메모리 무효화(Dangling Reference) 문제
`ObjectManager` 클래스는 내부적으로 객체들을 `std::vector<Object>`에 저장하며, 객체 생성 시 참조자(`Object&`)를 반환합니다.
```cpp
// include/knot/manager.h & src/manager.cpp
Object& ObjectManager::createObject(Mesh* mesh) {
    unsigned int newId = nextId++;
    objects.emplace_back(mesh, newId);
    idToIndex[newId] = objects.size() - 1;
    return objects.back(); 
}
```
* **문제점**: `std::vector`는 새로운 요소가 추가되어 내부 용량(capacity)을 초과할 때, 전체 데이터를 새로운 메모리 영역으로 재할당(Reallocation) 후 이동시킵니다. 
* **결과**: 이전에 `createObject`를 통해 반환받은 `Object&`를 호출처(예: 데모 코드나 씬 관리 코드)에서 보관하고 있는 경우, 벡터 재할당이 일어나는 순간 **모두 잘못된 메모리 주소를 가리키는 댕글링 참조(Dangling Reference)**가 되어 크래시(Crash)나 정의되지 않은 동작(UB)을 유발합니다.
* **해결 제안**:
  * 객체 생명주기를 안정적으로 보장하기 위해 `std::vector<std::unique_ptr<Object>>` 혹은 `std::list<Object>`를 사용하는 것이 안전합니다.
  * `idToIndex` 맵 역시 인덱스 대신 원시 포인터 혹은 스마트 포인터 자체를 가리키도록 변경하여 벡터 정렬/중간 삭제 시 인덱스가 어긋나는 문제를 방지해야 합니다.

### ⚠️ 에셋 생명주기(Resource Lifecycle) 관리 부재
현재 `Object` 구조체는 Raw Pointer인 `Mesh*`를 멤버로 소유하고 있습니다.
* **문제점**: `demo/demo.cpp`처럼 스택 영역에 `Mesh` 객체를 생성하고 그 주소를 전달할 경우, 해당 스택 스코프가 끝나 `Mesh`가 소멸하더라도 `Object`는 이를 인지하지 못하고 소멸된 객체의 주소를 계속 유지하게 됩니다.
* **해결 제안**: 스마트 포인터(`std::shared_ptr<Mesh>`)를 사용하여 여러 게임 오브젝트가 동일한 메쉬 에셋을 안전하게 공유하고, 참조 카운트가 0이 될 때 자동으로 리소스가 해제되도록 설계해야 합니다.

### ⚠️ 헤더 의존성 누락
* `include/knot/manager.h`에서 `std::unordered_map`을 멤버 변수로 선언하고 있으나, 헤더 파일 상단에 `#include <unordered_map>` 및 `#include <string>` 등이 누락되어 있습니다. 이로 인해 컴파일러 환경에 따라 컴파일 에러가 발생할 수 있습니다.

---

## 🛠️ 2. README TODO 기준 구현 상태 평가

현재 작성된 개발 계획 대비 실제 코드 구현율을 확인한 결과입니다.

### 1단계: 빌드 환경 및 핵심 프레임워크 (진행률: 70%)
* [x] **빌드 시스템(CMake) 설계**: `CMakeLists.txt`를 통해 라이브러리(`notengine`)와 데모 앱 빌드가 설정되어 있으나, 외부 의존성(GLFW, GLM)이 로컬 시스템에 미리 설치되어 있어야 하는 한계가 있습니다.
* [x] **`Engine` 관리자 클래스 완성**: 라이프사이클 조율 틀(`init`, `run`, `update`, `render`)은 정상 동작하나, 세부적인 리소스/수학 로직 미비로 인해 실질적인 구동 범위가 제한적입니다.
* [~] **Mesh GPU 버퍼 업로드 기능 구현**: `resources.h`의 `Mesh` 자체에는 로직이 없으나, `include/knot/utility/mesh_helper.hpp`에 `setupMesh` 인라인 함수 형식으로 임시 구현되어 있습니다. 아키텍처상 리소스 로딩 모듈과의 통일성이 필요합니다.

### 2단계: 렌더링 파이프라인 기초 및 수학 연동 (진행률: 20%)
* [ ] **카메라 및 렌더러 기능 강화**: `Renderer::renderObject`가 단순히 VAO를 바인딩하고 그릴 뿐, 셰이더 선택(`shader.use()`)이나 3D 공간을 표현하기 위한 MVP(Model-View-Projection) 행렬을 전달하는 렌더 파이프라인 로직이 부재합니다.
* [ ] **수학 라이브러리(`math.h`) 작성**: `include/knot/math.h` 파일은 `namespace knot {}`만 적힌 빈 파일입니다. GLM에 대한 래핑 또는 사용 정의가 필요합니다.
* [~] **데모 어플리케이션 제작**: `demo/demo.cpp`에 창 생성까지는 연동되었으나, 실제 3D 렌더링 및 카메라 이동 등의 예제 로직이 구현되지 않았습니다.

### 3단계: 리소스 매니저 및 텍스처 시스템 (진행률: 0%)
* [ ] **리소스 매니저 클래스 개발**: 아직 구현된 흔적이 없습니다.
* [ ] **텍스처 지원**: 이미지 로드 및 텍스처 버퍼 업로드 기능이 없습니다.

---

## ✍️ 3. 코드 컨벤션 및 가독성 피드백

### 🚫 가독성을 해치는 비표준 매크로 정의
`demo/demo.cpp` 파일의 최상단에 다음과 같은 매크로가 정의되어 있습니다.
```cpp
#define def auto
def main() -> int { ... }
```
C++의 가독성을 파이썬 등의 언어처럼 바꾸려는 의도로 보이나, 협업 및 유지보수 측면에서 표준 C++ 컨벤션을 어긋나게 만들고 IDE의 코드 힌트 기능을 방해하므로 제거하는 것이 좋습니다.

### 🚫 불완전하게 방치된 구문
`demo/demo.cpp` 내의 16번 라인 코드는 다음과 같습니다.
```cpp
engine.getObjectManager().createObject(&mesh).scale;
```
생성된 객체의 `scale` 변수를 조회만 하고 아무 처리도 하지 않은 채 세미콜론으로 종결되었습니다. 코드 정리 및 디버깅 흔적 제거가 필요합니다.

---

## 🚀 4. 아키텍처 개선을 위한 권장 사항 (코드 예시)

### 1) `ObjectManager`의 안전한 포인터 관리
참조값 무효화 문제를 해결하기 위해 스마트 포인터 기반으로 수정하는 것을 권장합니다.

```cpp
// include/knot/manager.h (수정본)
#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <knot/resources.h>

namespace knot {
    class ObjectManager {
    public:
        // unique_ptr을 반환하여 소유권을 전달하거나, 관리를 유지하기 위해 raw pointer 전달
        Object* createObject(std::shared_ptr<Mesh> mesh);
        Object* getObjectById(unsigned int id);
        std::vector<std::unique_ptr<Object>>& getObjectList();

    private:
        std::vector<std::unique_ptr<Object>> objects;
        std::unordered_map<unsigned int, Object*> idToPtr;
        unsigned int nextId = 1;
    };
}
```

### 2) CMake 의존성 관리 개선
로컬 환경에 종속되지 않도록 CMake의 `FetchContent`나 `vcpkg`를 도입하여 GLFW 및 GLM을 자동 다운로드 및 구성하도록 개선하면 이식성이 향상됩니다.
