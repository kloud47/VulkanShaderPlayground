# Architecture & Conventions

This is the **target** shape of the program. You'll grow into it one lesson at a time. Each
module below lists the lesson that introduces it. Come back here whenever you're unsure where
new code belongs.

---

## 1. Folder layout (end of Phase 7)

```
VulkanShaderPlayground/
├─ CMakeLists.txt            build definition
├─ CMakePresets.json         x64-debug / x64-release configurations
├─ vcpkg.json                dependencies (pinned by vcpkg-configuration.json)
├─ vcpkg-configuration.json
├─ docs/                     roadmap, this file, lessons
├─ assets/
│  ├─ models/                your .obj / .fbx / .gltf files
│  └─ textures/              .png / .jpg (albedo, normal maps, noise...)
├─ shaders/
│  ├─ include/               common.glsl, lighting.glsl, noise.glsl   (Phase 7)
│  ├─ builtin/               shaders the app itself needs (debug views, fallback)
│  └─ playground/            YOUR practice shaders (Phase 8 lives here)
└─ src/
   ├─ main.cpp               creates Application, catches fatal errors
   ├─ app/                   Application: owns everything, runs the loop
   ├─ core/                  Window, Log, Timer, Input: no Vulkan in here
   ├─ vk/                    thin Vulkan layer: context, swapchain, buffers, images,
   │                         pipelines, descriptors, shader modules, helpers
   ├─ renderer/              Renderer (frame loop), GpuMesh, Texture, Material
   ├─ scene/                 Camera, Transform, MeshData, Primitives, ModelLoader
   ├─ shader/                ShaderCompiler (shaderc), hot-reload watcher
   └─ ui/                    ImGuiLayer, PlaygroundUI panels
```

## 2. Modules and where they come from

| Module | Files | Responsibility | Lesson |
|---|---|---|---|
| Log | `core/Log.h` | `log::info/warn/error` built on `std::format` | 0.2 |
| Window | `core/Window.*` | GLFW window, framebuffer size, resize flag, title | 0.2 |
| Application | `app/Application.*` | Owns all subsystems, main loop, init/shutdown order | 0.2 |
| VkCheck | `vk/VkCheck.h` | `VK_CHECK(expr)` macro | 1.1 |
| VulkanContext | `vk/VulkanContext.*` | Instance, debug messenger, surface, GPU, device, queue, VMA | 1.1–1.2 |
| VmaUsage | `vk/VmaUsage.cpp` | The single `VMA_IMPLEMENTATION` translation unit | 0.1 |
| Swapchain | `vk/Swapchain.*` | Swapchain images/views, per-image present semaphores, recreation | 1.3 |
| VkUtils | `vk/VkUtils.*` | `transitionImage`, debug names, small helpers | 1.4 |
| Renderer | `renderer/Renderer.*` | Frames in flight, acquire → record → submit → present | 1.4 |
| ShaderModule | `vk/ShaderModule.*` | Load SPIR-V, create `VkShaderModule` | 2.1 |
| PipelineBuilder | `vk/PipelineBuilder.*` | Fluent builder for graphics pipelines (dynamic rendering) | 2.2 |
| Buffer | `vk/Buffer.*` | VMA-backed buffers, staging upload, immediate submit | 2.3 |
| Descriptors | `vk/Descriptors.*` | Set layout builder, descriptor pool/allocator, writer | 3.2 |
| Image | `vk/Image.*` | VMA-backed images + views (depth, textures) | 3.3, 6.1 |
| Input, Camera | `core/Input.*`, `scene/Camera.*` | Mouse/keyboard state; orbit camera | 3.4 |
| ImGuiLayer, PlaygroundUI | `ui/*` | ImGui setup + all panels | 4.1–4.2 |
| MeshData, GpuMesh | `scene/MeshData.h`, `renderer/GpuMesh.*` | CPU geometry vs GPU buffers | 5.1 |
| Primitives | `scene/Primitives.*` | Plane, cube, sphere, cylinder, torus generators | 5.2 |
| ModelLoader | `scene/ModelLoader.*` | Assimp → `MeshData` | 5.3 |
| Texture | `renderer/Texture.*` | stb_image → `Image` + sampler, mipmaps | 6.1–6.2 |
| Material | `renderer/Material.*` | Shader + textures + params → pipeline + set 1 | 6.2, 7.3 |
| ShaderCompiler | `shader/ShaderCompiler.*` | GLSL → SPIR-V at runtime (shaderc), includes, errors | 7.1 |
| DeletionQueue | `vk/DeletionQueue.h` | Deferred destruction of resources still in flight | 7.2 |

## 3. Dependency direction

```
            app
          ╱  │  ╲
        ui  renderer  shader
          ╲  │  ╱   ╲
          scene      vk
             ╲      ╱
               core
```

Rules:
- **`core` knows nothing about Vulkan.** (`Window` may expose `GLFWwindow*`; the Vulkan
  surface is created inside `vk`.)
- **`scene` is CPU-only.** `MeshData` is plain vectors of vertices and indices with no
  Vulkan handles, so you can unit-test or reuse it anywhere.
- **`vk` knows nothing about meshes, cameras or the playground.** It's a reusable "Vulkan
  toolkit".
- **`renderer`** is where scene data meets Vulkan (it turns `MeshData` into a `GpuMesh`).
- Only **`app`** wires everything together.

If you notice yourself including `scene/` from inside `vk/`, stop. That code belongs in
`renderer/`.

## 4. Lifetime & ownership conventions

Vulkan objects must be destroyed **in reverse dependency order**, and most of them need the
`VkDevice` (sometimes also the `VmaAllocator`) to be destroyed. This project uses this rule:

> **Vulkan-owning classes use explicit `init(...)` / `shutdown()`. They're non-copyable, and
> the owner calls `shutdown()` in reverse order of `init()`.**

```cpp
class Swapchain {
public:
    Swapchain() = default;
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    ~Swapchain() { assert(m_swapchain == VK_NULL_HANDLE && "shutdown() not called"); }

    void init(VulkanContext& ctx, VkExtent2D extent, bool vsync);
    void shutdown();
    ...
};
```

Why not full RAII destructors everywhere?
- **The order stays visible.** `Application::shutdown()` reads top to bottom as the
  teardown sequence, which is half of what Vulkan is teaching you.
- **Recreation is common.** Swapchains, pipelines (hot reload) and depth images get rebuilt
  at runtime. `shutdown(); init(...)` is simple and explicit.
- **GPU work may still be in flight.** A destructor can't know whether the GPU is still using
  a resource. That problem gets solved properly with a **deletion queue** in 7.2.

Non-Vulkan classes (`Window`, `Timer`, `MeshData`, `Camera`) use plain RAII.

Every `shutdown()` must be **safe to call twice**: check for `VK_NULL_HANDLE`, destroy, then
set the handle back to `VK_NULL_HANDLE`.

## 5. Error handling

- **Every `VkResult` is checked.** Use `VK_CHECK(vkCreateX(...))` (lesson 1.1). It logs the
  expression, `string_VkResult(result)`, file and line, then aborts.
- **Results you expect are handled explicitly, not with VK_CHECK.** For example
  `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` from acquire/present mean "recreate the
  swapchain", not "crash".
- **Init failures throw** `std::runtime_error` with a readable message ("No GPU supports
  Vulkan 1.3 + dynamic rendering"). `main()` catches, logs, and returns 1.
- **Runtime-recoverable errors never crash.** From Phase 7 on, a broken shader or a missing
  texture logs an error, shows it in the UI, and the app keeps its last good state.
- **Validation layers are always on in Debug.** A validation message is a bug, even if the
  picture looks right.

## 6. Code style

| Thing | Convention | Example |
|---|---|---|
| Namespace | `vsp` (Vulkan Shader Playground) | `vsp::Renderer` |
| Types | PascalCase | `VulkanContext`, `FrameData` |
| Functions, variables | camelCase | `createSwapchain()`, `imageCount` |
| Members | `m_` prefix | `m_device`, `m_frames` |
| Constants | `k` prefix | `kMaxFramesInFlight` |
| Files | match the main type | `Swapchain.h` / `Swapchain.cpp` |
| Headers | `#pragma once` | |

**Vulkan structs:** always value-initialize, and use C++20 designated initializers. Fields
**must be in declaration order**, or MSVC will reject them:

```cpp
VkSemaphoreCreateInfo info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
VkFenceCreateInfo fenceInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
};
```

Anything you don't name is zero-initialized (`pNext = nullptr`, counts = 0), which is what you
want for almost every field.

**Name your Vulkan objects** with `vkSetDebugUtilsObjectNameEXT` (helper in 1.4). Validation
errors and RenderDoc will then say `"Swapchain Image 2"` instead of `0x2a000000002a`.

## 7. Conventions shared by C++ and GLSL

These are decided now so that every shader you write in Phase 8 plugs in without C++ changes.
Each one gets implemented in the lesson shown.

### 7.1 Coordinate system (3.1)
- World space is **right-handed, +Y up**. The camera looks down **−Z** in view space.
- Projection: `glm::perspective` with `GLM_FORCE_DEPTH_ZERO_TO_ONE` (Vulkan depth range is
  0..1), then **`proj[1][1] *= -1`** because Vulkan's clip-space Y points down.
- Front faces are **counter-clockwise** (`VK_FRONT_FACE_COUNTER_CLOCKWISE`), with back-face
  culling on by default.

### 7.2 Vertex layout (2.3 / 5.1)
| Location | Attribute | Type | Notes |
|---|---|---|---|
| 0 | position | `vec3` | object space |
| 1 | normal | `vec3` | object space, normalized |
| 2 | uv | `vec2` | (0,0) = top-left of the texture |
| 3 | tangent | `vec4` | `xyz` tangent, `w` = ±1 bitangent sign (normal mapping, 8.3) |

### 7.3 Resource binding plan (3.2 / 6.2 / 7.3)
| Slot | Contents | Updated |
|---|---|---|
| Push constants (≤ 128 B) | `mat4 model`, `uint debugMode`, ... | per draw |
| **Set 0, binding 0** | `FrameUBO`: view, proj, viewProj, camera position, time, delta time, resolution, mouse, light | per frame |
| **Set 1, bindings 0–3** | `tex0..tex3` combined image samplers (like Shadertoy's `iChannel0..3`) | per material |
| **Set 1, binding 4** | `MaterialParams` UBO: your shader's tweakable values | per material |

## 8. The frame, at a glance (built in 1.4, extended later)

```
 Application::run()
   while (!window.shouldClose())
     window.pollEvents()
     timer.tick()                       → dt, time
     ui.beginFrame()          (Phase 4)
     ui.buildPanels(state)    (Phase 4) → user picks mesh / texture / shader
     shaders.pollHotReload()  (Phase 7)
     renderer.drawFrame(scene, camera, ui):
        wait fence[frame] ─ acquire image ─ reset fence
        record:  barrier(UNDEFINED → COLOR_ATTACHMENT)
                 beginRendering(color + depth)
                   bind pipeline, set 0, set 1, push constants, draw mesh
                   ImGui draw data             (Phase 4)
                 endRendering
                 barrier(COLOR_ATTACHMENT → PRESENT_SRC)
        submit2: wait imageAvailable[frame], signal renderFinished[image], fence[frame]
        present: wait renderFinished[image]
        frame = (frame + 1) % kMaxFramesInFlight
```
