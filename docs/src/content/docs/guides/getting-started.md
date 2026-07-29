---
title: 시작하기
description: 이 게임 엔진을 처음 접하는 사람들을 위한 가이드입니다.
slug: guides/getting-started
---

이 문서는 Not Engine을 처음 접하는 사람들을 위해 빌드하고 데모를 실행하는 방법과 기본적인 기능을 안내합니다.

## 설치하기

아직 사전 빌드된 바이너리를 제공하지 않기 때문에 직접 빌드해야 합니다.

### 요구사항

- OpenGL 4.3 이상을 지원하는 그래픽 카드

### 빌드

:::caution[주의]
이 엔진은 현재 **Linux 환경에서만 테스트**되었습니다. Windows나 macOS 환경에서의 동작 및 안정성은 보장되지 않습니다.
:::

#### Linux (권장)

빌드를 하기 위해서는 `cmake`와 `glfw3` 패키지가 필요합니다.

##### Debian/Ubuntu

```bash
sudo apt update
sudo apt install cmake libglfw3-dev build-essential
```

##### Fedora

```bash
sudo dnf check-update
sudo dnf install cmake glfw-devel @development-tools
```

패키지 설치를 완료하였다면 아래 명령어를 입력하여 빌드하고 설치할 수 있습니다.

```bash
# 라이브러리 릴리즈 빌드
./script/library_release_build.sh
# 설치
sudo cmake --install build
```

또는 cmake를 직접 사용해서

```bash
# 라이브러리 릴리즈 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target demo
# 설치
sudo cmake --install build
```

#### Windows


빌드를 하기 위해서는 **CMake**와 **GLFW**가 필요합니다.

- **CMake**: [cmake.org](https://cmake.org/download/)에서 설치 가능
- **GLFW**:
  - **vcpkg 사용 (권장)**:
    ```powershell
    vcpkg install glfw3:x64-windows
    ```
  - **공식 바이너리**: [glfw.org](https://www.glfw.org/download.html)에서 Windows용 사전 빌드된 바이너리 다운로드

##### Visual Studio (MSVC) 사용 시

```powershell
# 라이브러리 릴리즈 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target demo

# 설치 (설치시 관리자 권한 필요)
cmake --install build --config Release
```

:::tip[vcpkg 연동]
vcpkg로 GLFW를 설치했다면 1번 Configure 단계에 toolchain 옵션을 추가하세요.
:::

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="[vcpkg 설치경로]/scripts/buildsystems/vcpkg.cmake"
```

##### MinGW (GCC) 사용 시

```powershell
# 라이브러리 릴리즈 빌드
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target demo

# 설치 (설치시 관리자 권한 필요)
cmake --install build
```

#### macOS

:::danger[사용 불가]
안타깝게도 macOS는 애플의 폐쇄적인 정책과 시대에 뒤떨어진 독자 API 사용 때문에 **OpenGL 4.1** 이상의 버전을 지원하지 않아 **OpenGL 4.3** 이상을 필요로 하는 Not Engine을 사용할 수 없습니다. 

만약 Not Engine이 Vulkan을 지원하게 되더라도 macOS는 Metal을 제외한 모든 그래픽 API를 지원하지 않고, Vulkan 또한 직접적으로 지원하지 않기 때문에 MoltenVK를 사용하지 않는 한 사용이 불가능합니다. 우리는 아직까지 공식적으로 macOS를 지원하지 않으므로 macOS에서의 빌드에 관한 가이드를 지원하지 않을 겁니다.
:::