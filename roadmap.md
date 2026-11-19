## Todo

### Alpha
- [x] 마우스 입력
- [x] 이벤트 시스템
- [x] 씬 만들기
- [x] 포메터 셋팅
- [x] 네이밍 케이스 정리
- [x] OBJ 임포트
- [x] MTL 머티리얼 임포트
- [x] GLTF 임포트 (https://github.com/jkuhlmann/cgltf)
- [x] camera.hpp를 구현과 헤더로 나누기
- [x] 오디오엔진
- [x] 인스턴싱
- [x] 타이머 (특정 시간이나 특정 주기마다 이벤트 자동실행)

### 그래픽스
- [x] 퐁 셰이더 만들기
- [x] 라이트(light) 오브젝트
- [x] PBR 셰이더 만들기
- [x] 스카이박스 구현하기
- [x] IBL Diffuse
- [x] IBL Specular
- [x] Dir Shadow Map
- [x] Point Shadow Map
- [ ] Cascaded Shadow Maps - 12월
- [x] PCSS
- [ ] Cascaded PCSS
- [x] SSAO
- [ ] 다이나믹 큐브맵
- [ ] SSGI (실험적으로 구현되었지만 제대로 작동하지 않음)
- [x] Reflection Probe
- [ ] Ray Tracing
- [ ] Fog - 12월
- [ ] 대기 산란
- [ ] Subsurface Scattering
- [ ] Volumetric 렌더링/레이마칭
- [x] Post-processing
- [x] Bloom
- [ ] 기타 고급 그래픽 기술

#### 투명한 물체
- [ ] 굴절되는 물체 (유리, 물 등)
- [x] 투명/반투명한 오브젝트

#### 파티클
- [x] 파티클 시스템 (CPU 시뮬레이션)
- [ ] 고도화된 파티클 시스템 (GPU 시뮬레이션 등)

#### 그래픽스 API
- [ ] sokol-gfx (https://github.com/floooh/sokol)
- [ ] Vulkan

### 기타 엔진 시스템
- [ ] EXR 확장자 지원 (TinyEXR 사용) - 12월
- [ ] GUI 시스템
- [x] 빌드시 셰이더를 소스코드에 자동으로 포함하도록 하기(Cmake)
- [x] Seno (ScEne format for NOt engine) 로딩
- [ ] 렌더 패스 커스텀 가능하게 하기

#### 카메라
- [x] 직교 투영 카메라
- [ ] 오브젝트 추적 카메라 - 12월

#### 에니메이션
- [ ] Bone
- [x] tween 에니메이션
- [ ] bone 에니메이션

### 리팩토링
- [ ] resources.h 분리
- [ ] Renderer 분리 - 12월
- [ ] 커스텀 셰이더를 더 자유롭게 만들수있도록 수정하기(제공되는 유니폼 목록 정리하고 더 다양한 셰이더를 만들수 있도록 많은 유니폼을 제공)
- [ ] Scene::loadSeno를 리팩토링하기, 하나의 함수가 너무 길어져있음 또한 버전별로 다르게 로딩하는 것 구현
- [x] Audio를 싱글톤으로 만들기
- [x] resource manager(shader manager)를 싱글톤으로 바꾸기
- [ ] API 다듬기 (리턴, 입력값 통일화, 변수 스타일 통일화 등)

### 문서화
- [x] API 명세
- [ ] docs 완성
- [x] tween.h 문서화

### 라이브러리
- [x] OpenAL

### 최적화
- [ ] 트랜스폼에 더티플래그 추가해서 위치나 회전, 스케일 바뀔때만 셰도우맵 재연산하도록 하기 - 12월

### 기타 아이디어
- [ ] 엔진 전체에서 하나의 내부 ID에 여러 문자열·정수 식별자를 매핑할 수 있는 통합 식별자 시스템
- [x] 하드코딩된 상수들을 하나의 상태머신 헤더로 묶어서 설정하기 편하게 하기
- [ ] Amazon Lumberyard Bistro 렌더링해보기

### 계획되지 않음
- [ ] Image/Texture 클래스 만들기 (아직 필요성을 느끼지 못함)