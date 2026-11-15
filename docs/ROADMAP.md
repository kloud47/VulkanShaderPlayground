# Vulkan Shader Playground — Learning Roadmap

You're building a desktop app where you pick a 3D object (a built-in primitive or your own model
loaded with Assimp), put a texture and a GLSL shader on it, and see your shader changes live.
You write all of the code yourself. These lessons explain the *why* and the *what*, show
the API shape where it helps, and give you a checkpoint for each lesson.

---

## How to use these lessons

Every lesson follows the same loop:

```
 Read the concepts  ──►  Write the code yourself  ──►  Hit the checkpoint
        ▲                                                   │
        │                                                   ▼
  Next lesson  ◄──  Commit + tick the box below  ◄──  Answer the self-check questions
```

1. **Read the whole lesson before you type anything.** Lessons are short on purpose.
2. **Write the code yourself.** Snippets in lessons are *shapes* (signatures, struct fields,
   call order), not complete solutions. Hints are folded inside `<details>` blocks, so open
   one only when you're stuck.
3. **Hit the checkpoint.** Each lesson ends with something you can see or measure. If it
   doesn't match, check the "Common pitfalls" section first.
4. **Keep the validation layers clean.** Treat any validation message as a bug, even if
   the picture looks right.
5. **Commit after every lesson** (`git commit -m "Lesson 1.2: device + queues"`). You'll want
   to diff against working states later.
6. **Ask for a code review at the end of each phase.** Ask Claude: *"Review my Phase N code"*.
   The next phase's lessons get written after that review, so they can build on what your
   code actually looks like.

### Assumptions about you

| You know | So the lessons... |
|---|---|
| Modern C++ (RAII, smart pointers, lambdas, templates) | don't explain C++. They do point out C++20 features that help with Vulkan. |
| Vulkan hello-triangle (done once) | treat Vulkan basics as a rebuild done properly: they focus on Vulkan 1.3, why things work the way they do, and reusable structure. |
| **No** OpenGL renderer / 3D math background yet | give 3D math (spaces, matrices, cameras) its own full lesson (3.1). |
| **No** GLSL yet | teach GLSL from zero (2.1), then the whole of Phase 8 is GLSL practice. |

---

## Tech stack

| Piece | Choice | Why |
|---|---|---|
| IDE | Visual Studio 2026 (Open Folder + CMake) | Native CMake presets, debugger, bundled vcpkg + Ninja |
| Build | CMake 3.25+ with `CMakePresets.json` | Industry standard, portable, easy to grow |
| Packages | vcpkg (manifest mode, `vcpkg.json`) | Versions pinned per project, no manual lib paths |
| Graphics API | Vulkan **1.3**, plain C API | Dynamic rendering + synchronization2, with less boilerplate than render passes. Runs on both of your GPUs (RTX 3050 = 1.4, AMD iGPU = 1.3) |
| Window/input | GLFW | Small, Vulkan-friendly, creates the surface for you |
| Math | GLM | GLSL-like syntax for vectors and matrices on the CPU |
| GPU memory | Vulkan Memory Allocator (VMA) | Handles suballocation and memory types for you |
| UI | Dear ImGui | Immediate-mode debug UI, the standard for tools like this |
| Models | Assimp | Loads OBJ, FBX, glTF, and many other formats |
| Images | stb_image | Single header, loads PNG/JPG/HDR |
| Shaders | GLSL → SPIR-V via `glslc` (offline), later **shaderc** at runtime | Offline compile to start; runtime compile makes hot reload possible |
| Debugging | Validation layers, Vulkan Configurator, RenderDoc, (Nsight Graphics) | You'll use these constantly |

---

## Phase map

```
 Phase 0  Foundations ───────► empty window with FPS in the title
    │
 Phase 1  Vulkan core ───────► animated clear color, resize-safe, 0 validation errors
    │
 Phase 2  Pipelines/geometry ► a colored quad drawn from vertex + index buffers
    │
 Phase 3  Going 3D ──────────► a depth-tested cube you orbit with the mouse
    │
 Phase 4  Debug UI ──────────► ImGui panels: stats, transform, camera, wireframe
    │
 Phase 5  Meshes ────────────► dropdown: cube/plane/sphere/torus/cylinder + your Assimp models
    │
 Phase 6  Textures ──────────► texture browser; any texture on any mesh
    │
 Phase 7  Shader engine ─────► save a .frag in VS and see it update; errors show in the UI
    │
 Phase 8  GLSL practice ─────► lighting, procedural, vertex FX, toon, PBR... (exercises)
    │
 Phase 9  Expansions ────────► pick-your-own: post FX, shadows, compute, IBL, ray tracing...
```

**When does GLSL practice start?** You can do 8.1 and 8.2 as soon as Phase 5 is done
(recompiling offline). They're much more fun after Phase 7, because hot reload gives you an
edit-save-see loop of about one second.

---

## Phase 0 — Foundations
**Goal:** a project that builds from a clean clone and opens a window.

- [ ] [0.1 Toolchain, CMake & vcpkg](lessons/00-foundations/0.1-toolchain-cmake-vcpkg.md): the project skeleton, a dependency smoke test, and git
- [ ] [0.2 Window, main loop & app skeleton](lessons/00-foundations/0.2-window-and-main-loop.md): GLFW, `Application`, delta time, FPS

**Checkpoint:** a 1600×900 window titled `Vulkan Shader Playground | 1234 FPS`; Esc closes it.

## Phase 1 — Vulkan core, the 1.3 way
**Goal:** a correct, resize-safe frame loop. Everything in later phases builds on it.

- [ ] [1.1 Instance, validation layers & debug messenger](lessons/01-vulkan-core/1.1-instance-and-validation.md)
- [ ] [1.2 Surface, GPU selection, device, queues & VMA](lessons/01-vulkan-core/1.2-device-queues-vma.md)
- [ ] [1.3 The swapchain](lessons/01-vulkan-core/1.3-swapchain.md)
- [ ] [1.4 Frames in flight, synchronization2 & clearing the screen](lessons/01-vulkan-core/1.4-frames-sync-clear.md)

**Checkpoint:** the screen clears to a slowly cycling color. Resizing, minimizing and closing
produce zero validation messages, including with synchronization validation turned on. A
RenderDoc capture shows your `vkCmdBeginRendering`.

## Phase 2 — Pipelines & geometry *(written after your Phase 1 review)*
- [ ] 2.1 GLSL first steps & the SPIR-V build step (`glslc` from CMake)
- [ ] 2.2 A graphics pipeline builder + your first triangle (dynamic rendering, dynamic viewport/scissor)
- [ ] 2.3 GPU buffers with VMA: vertex/index buffers, staging uploads, immediate submit

**Checkpoint:** an indexed, vertex-colored quad.

## Phase 3 — Going 3D
- [ ] 3.1 3D math for graphics: vectors, matrices, model/view/projection, Vulkan clip space, GLM
- [ ] 3.2 Push constants, uniform buffers & descriptor sets (per-frame data in set 0)
- [ ] 3.3 The depth buffer
- [ ] 3.4 Input handling & an orbit camera

**Checkpoint:** a spinning, depth-correct cube you orbit, pan and zoom with the mouse.

## Phase 4 — Debug UI with Dear ImGui
- [ ] 4.1 Integrating ImGui with dynamic rendering (and sRGB gotchas)
- [ ] 4.2 Playground panels v1: stats, clear color, transform, camera, wireframe pipeline, vsync toggle

**Checkpoint:** tweak the object and camera live from UI panels.

## Phase 5 — Meshes: primitives & models
- [ ] 5.1 Mesh architecture: CPU `MeshData` vs GPU `GpuMesh`, the vertex layout contract, bounds
- [ ] 5.2 Procedural primitives: plane, cube, UV sphere, cylinder, torus (normals, UVs, tangents)
- [ ] 5.3 Loading models with Assimp (submeshes, a file dialog, auto-framing)

**Checkpoint:** a dropdown that switches between all 5 primitives and any .obj/.fbx/.gltf you load.

## Phase 6 — Textures & materials
- [ ] 6.1 Images, staging uploads & mipmap generation (stb_image)
- [ ] 6.2 Samplers, sRGB vs linear data, material descriptor set (set 1), default textures
- [ ] 6.3 A texture browser panel (folder scan + ImGui thumbnails)

**Checkpoint:** pick any texture from the browser and see it on any mesh, with correct color space.

## Phase 7 — The shader playground engine
- [ ] 7.1 Runtime GLSL compilation with shaderc (`#include` support, readable error messages)
- [ ] 7.2 Hot reload & safe destruction of in-flight resources (deletion queue)
- [ ] 7.3 The playground shader contract (`common.glsl`), shader browser, keep-last-good-pipeline

**Checkpoint:** edit a `.frag` in Visual Studio and save; the object updates within about a
second. Break the shader and the compile error appears in the UI while the app keeps running.

## Phase 8 — GLSL practice track (exercises inside your playground)
- [ ] 8.1 Debug views: UVs, normals, depth, world position
- [ ] 8.2 Lighting I: Lambert, Blinn-Phong, rim/Fresnel, multiple lights
- [ ] 8.3 Texturing: tiling/scrolling, triplanar mapping, normal mapping (TBN)
- [ ] 8.4 Procedural patterns & noise: stripes, checkers, value/gradient noise, fBm, marble/wood
- [ ] 8.5 Vertex animation: waves, twist, explode-along-normals
- [ ] 8.6 Stylized: toon/cel shading, dissolve with glowing edges, hologram, x-ray
- [ ] 8.7 PBR: Cook-Torrance GGX with metallic/roughness

## Phase 9 — Expansion menu (any order, any time after Phase 7)
- [ ] Auto-generated shader parameter UI via SPIR-V reflection (sliders appear from your `uniform` block)
- [ ] Multiple objects + scene save/load (JSON)
- [ ] Skybox & environment reflections (cubemaps)
- [ ] Shadow mapping
- [ ] Offscreen rendering + post-processing; a "Shadertoy mode" fullscreen shader
- [ ] Compute shaders: GPU particles
- [ ] Image-based lighting (irradiance, prefiltered env, BRDF LUT)
- [ ] Bindless textures (descriptor indexing)
- [ ] GPU timings in the stats panel (timestamp queries)
- [ ] Mesh shaders (your RTX 3050 supports them)
- [ ] Ray-traced reflections/shadows (your RTX 3050 supports `VK_KHR_ray_tracing_pipeline`)

---

## Reference shelf

Bookmark these. Lessons link to specific pages.

**Vulkan**
- Vulkan Guide (Khronos): https://docs.vulkan.org/guide/latest/
- Vulkan Specification: https://docs.vulkan.org/spec/latest/
- vkguide.dev: uses Vulkan 1.3 + dynamic rendering + VMA like this project; great for a second explanation
- Sascha Willems' samples: https://github.com/SaschaWillems/Vulkan (see `dynamicrendering`)
- Khronos synchronization examples: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
- VMA docs: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/
- GLFW Vulkan guide: https://www.glfw.org/docs/latest/vulkan_guide.html

**Math & graphics concepts**
- 3Blue1Brown, *Essence of Linear Algebra* (YouTube): watch episodes 1–9 before lesson 3.1
- LearnOpenGL: https://learnopengl.com. It's OpenGL, but the chapters on transformations,
  cameras, lighting, and normal mapping teach concepts that carry over directly
- *Real-Time Rendering, 4th ed.* (book) for when you want depth

**GLSL**
- The Book of Shaders: https://thebookofshaders.com
- Inigo Quilez articles: https://iquilezles.org/articles/
- Shadertoy: https://www.shadertoy.com
- GLSL spec (bookmark it): https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf

**Tools**
- RenderDoc: https://renderdoc.org
- NVIDIA Nsight Graphics (works with your RTX 3050)
- Vulkan Configurator (`vkconfig-gui.exe`) ships with the SDK in `D:\Vulkan\Bin`
