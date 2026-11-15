# Vulkan Shader Playground

A learning project: a Vulkan 1.3 app for practicing GLSL. You pick an object (a built-in
**cube, plane, sphere, torus or cylinder**, or **your own model loaded with Assimp**), attach
a **texture** and a **shader**, and see edits live through hot reload.

> Built by hand, one lesson at a time. The code in `src/` is my own; `docs/` holds the
> lessons it follows.

## Where to start

1. [docs/ROADMAP.md](docs/ROADMAP.md): the full plan, with progress checkboxes
2. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): where code goes, and the conventions used
3. First lesson: [docs/lessons/00-foundations/0.1-toolchain-cmake-vcpkg.md](docs/lessons/00-foundations/0.1-toolchain-cmake-vcpkg.md)

## Build (once lesson 0.1 is done)

Requirements: Visual Studio 2026 (Desktop development with C++, CMake tools, vcpkg), Vulkan
SDK 1.3+ with `VULKAN_SDK` set, and a GPU that supports Vulkan 1.3.

- **Visual Studio:** File → Open → Folder… → pick this folder → choose the **x64 Debug**
  preset → F5.
- **Command line** (in *Developer PowerShell for VS 2026*):
  ```powershell
  cmake --preset x64-debug
  cmake --build --preset x64-debug
  .\out\build\x64-debug\VulkanShaderPlayground.exe
  ```

## Tech

C++20 · Vulkan 1.3 (dynamic rendering, synchronization2) · GLFW · GLM · VMA · Dear ImGui ·
Assimp · stb_image · shaderc · CMake + vcpkg
