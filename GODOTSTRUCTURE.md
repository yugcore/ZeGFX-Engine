# GODOTSTRUCTURE.md: Complete Godot Engine Architecture Map & Internal Reference

---

> [!NOTE]
> **Purpose**: This document provides a comprehensive, repository-wide architectural blueprint of the Godot Engine (`Velvet-Engine` repository). It is written for core engine developers, graphics engineers, systems programmers, and contributors who need to modify, optimize, extend, or replace Godot subsystems without breaking downstream dependencies.

---

# Table of Contents
1. [Executive Summary & Repository Overview](#1-executive-summary--repository-overview)
2. [Complete Repository Directory Breakdown](#2-complete-repository-directory-breakdown)
   - [2.1 `core/` Subsystem](#21-core-subsystem)
   - [2.2 `servers/` Subsystem](#22-servers-subsystem)
   - [2.3 `scene/` Subsystem](#23-scene-subsystem)
   - [2.4 `editor/` Subsystem](#24-editor-subsystem)
   - [2.5 `main/` Subsystem](#25-main-subsystem)
   - [2.6 `drivers/` Subsystem](#26-drivers-subsystem)
   - [2.7 `platform/` Subsystem](#27-platform-subsystem)
   - [2.8 `modules/` Subsystem](#28-modules-subsystem)
   - [2.9 `thirdparty/`, `tests/`, `doc/`, `misc/`](#29-thirdparty-tests-doc-misc)
3. [Engine Boot & Shutdown Process](#3-engine-boot--shutdown-process)
4. [Runtime Core Subsystems Architecture](#4-runtime-core-subsystems-architecture)
5. [Scene System Architecture](#5-scene-system-architecture)
6. [Rendering Architecture & Pipeline](#6-rendering-architecture--pipeline)
7. [Physics Subsystem Architecture](#7-physics-subsystem-architecture)
8. [Audio Subsystem Architecture](#8-audio-subsystem-architecture)
9. [Animation & Skinning Subsystem](#9-animation--skinning-subsystem)
10. [Resource System & Asset Import Pipeline](#10-resource-system--asset-import-pipeline)
11. [Editor Architecture & Tool Mode](#11-editor-architecture--tool-mode)
12. [Module System vs. GDExtension Architecture](#12-module-system-vs-gdextension-architecture)
13. [Build System & SCons Compilation Pipeline](#13-build-system--scons-compilation-pipeline)
14. [Platform Layer & Hardware Abstraction](#14-platform-layer--hardware-abstraction)
15. [Memory Management & Multithreading Architecture](#15-memory-management--multithreading-architecture)
16. [Scripting Subsystem Architecture (GDScript & C#)](#16-scripting-subsystem-architecture-gdscript--c)
17. [Networking & Debug Systems](#17-networking--debug-systems)
18. [Extension Points & "Where to Modify X" Practical Guide](#18-extension-points--where-to-modify-x-practical-guide)
19. [System Dependency Graphs & Call Flow Maps](#19-system-dependency-graphs--call-flow-maps)
20. [Comprehensive Index of Key Engine Classes](#20-comprehensive-index-of-key-engine-classes)

---

# 1. Executive Summary & Repository Overview

Godot is built on a **layered, server-based, object-oriented architecture**. Unlike engines that rely on monolithic component systems (like Unity ECS) or deep inheritance trees bound to world states (like Unreal Actor trees), Godot decouples high-level user nodes (`Node`, `Node3D`, `Control`) from heavy low-level state execution. High-level nodes are thin wrappers that send command streams or IDs to low-level single-threaded or multi-threaded **Servers** (`RenderingServer`, `PhysicsServer3D`, `AudioServer`).

```
+-----------------------------------------------------------------------+
|                             EDITOR / GAME                             |
|        (editor/ - EditorNode, Docks, Inspector, Import, Plugins)      |
+-----------------------------------------------------------------------+
|                              SCENE LAYER                              |
|   (scene/ - Node, Node2D, Node3D, Control, Viewport, SceneTree, Tree) |
+-----------------------------------------------------------------------+
|                            SERVERS LAYER                              |
| (servers/ - RenderingServer, PhysicsServer3D, AudioServer, TextServer)|
+-----------------------------------------------------------------------+
|                           DRIVERS / PLATFORM                          |
| (drivers/ - Vulkan, GLES3, D3D12, Metal | platform/ - Win, Linux, Mac)|
+-----------------------------------------------------------------------+
|                            CORE SUBSYSTEM                             |
| (core/ - Object, RefCounted, ClassDB, Variant, Signal, Memory, IO)    |
+-----------------------------------------------------------------------+
```

## Repository Directory Tree

```
Velvet-Engine/
├── core/                   # Fundamental runtime primitives, Object/Variant systems, IO, OS
│   ├── config/             # Engine settings, feature flags, versioning
│   ├── crypto/             # Hashing, AES, RSA, TLS certificates
│   ├── debugger/           # Remote debugger client, profiling message routing
│   ├── error/              # Error macros, printing, callstacks
│   ├── extension/          # GDExtension C-API layer and bindings loader
│   ├── input/              # Input Event data structures, action maps
│   ├── io/                 # FileAccess, DirAccess, ResourceLoader, Compression, HTTP
│   ├── math/               # Vector2/3/4, Matrix, Transform2D/3D, Quaternion, Basis, Projection
│   ├── object/             # Object base class, ClassDB, Signal, Callable, ScriptLanguage
│   ├── os/                 # OS singleton interface, Thread, Mutex, Semaphore, Memory allocators
│   ├── profiling/          # Low-overhead CPU/Memory profiling markers
│   ├── string/             # String, StringName, NodePath, CharFX, Regex
│   ├── templates/          # Custom STL-free templates: Vector, HashMap, List, LocalVector, PagedAllocator
│   └── variant/            # Variant dynamic type wrapper, Array, Dictionary, Callables
├── drivers/                # Hardware backend implementations for rendering, audio, OS
│   ├── vulkan/             # RenderingDevice Vulkan backend
│   ├── gles3/              # RenderingDevice / RendererCompatibility OpenGL ES 3.0 backend
│   ├── d3d12/              # DirectX 12 backend for Windows/Xbox
│   ├── metal/              # Apple Metal graphics backend
│   ├── wasapi/             # Windows WASAPI audio backend
│   ├── alsa/               # Linux ALSA audio backend
│   ├── coreaudio/          # macOS / iOS CoreAudio backend
│   ├── sdl/                # SDL game controller mapping & input backend
│   └── windows/            # Win32 OS/Display backend hooks
├── editor/                 # Complete Godot Editor suite (runs as a Godot tool application)
│   ├── animation/          # AnimationPlayer editor, blend tree visual editor
│   ├── asset_library/      # In-editor asset store client
│   ├── audio/              # Audio bus layout editor
│   ├── debugger/           # In-editor script debugger, profiler, stack trace views
│   ├── docks/              # Dock windows (FileSystem, Scene Tree, Node Signals, History)
│   ├── export/             # Project export preset manager & platform exporter logic
│   ├── file_system/        # EditorFileSystem directory scanning, UID resolution
│   ├── gui/                # Custom editor widgets (Tree, GraphEdit, ColorPicker, SyntaxHighlighter)
│   ├── import/             # Asset import settings manager & importers
│   ├── inspector/          # Property inspector, custom property editors, gizmo integration
│   ├── plugins/            # Editor plugin manager & built-in plugin implementations
│   ├── project_manager/    # Standalone project launcher / selector executable UI
│   └── settings/           # EditorSettings persistent configuration
├── main/                   # Main executable entry point & lifecycle orchestration
│   ├── main.cpp            # Main::setup(), Main::start(), Main::iteration(), Main::cleanup()
│   ├── main_timer_sync.cpp # Fixed timestep accumulator & frame rate smoothing logic
│   └── performance.cpp     # Engine performance counters & statistics tracker
├── modules/                # Opt-in C++ feature modules & extension packages
│   ├── gdscript/           # GDScript language compiler, parser, byte-code VM engine
│   ├── mono/               # C# / .NET runtime bindings host (GodotSharp)
│   ├── gltf/               # GLTF 2.0 3D scene import/export pipeline
│   ├── godot_physics_3d/   # Default built-in 3D physics server implementation
│   ├── jolt_physics/       # Jolt 3D physics server integration module
│   ├── navigation_3d/      # Recast/Detour 3D navigation mesh & pathfinding server
│   ├── openxr/             # OpenXR VR/AR driver & node integration
│   ├── lightmapper_rd/     # Ray-traced lightmap baker module (Compute Shaders)
│   └── ... (50+ modules)   # Basis Universal, FreeType, WebRTC, WebSocket, MbedTLS, etc.
├── platform/               # OS main functions, DisplayServer implementations, build hooks
│   ├── windows/            # Win32 DisplayServer, OS_Windows, entry point (godot_windows.cpp)
│   ├── linuxbsd/           # X11 / Wayland DisplayServer, OS_LinuxBSD
│   ├── macos/              # Cocoa DisplayServer, OS_MacOS, AppKit wrappers
│   ├── android/            # Java Native Interface (JNI) bridge, DisplayServerAndroid
│   ├── ios/                # UIKit DisplayServer, OS_IOS
│   └── web/                # Emscripten / WebAssembly bindings, HTML5 Canvas DisplayServer
├── scene/                  # High-level retained-mode Scene Graph nodes & UI widgets
│   ├── 2d/                 # Node2D, Sprite2D, TileMap, CanvasItem, CollisionShape2D
│   ├── 3d/                 # Node3D, MeshInstance3D, Camera3D, Light3D, CollisionShape3D
│   ├── animation/          # AnimationPlayer, AnimationTree, AnimationMixer, Skeleton3D
│   ├── audio/              # AudioStreamPlayer, AudioStreamPlayer3D, AudioListener3D
│   ├── gui/                # Control, Button, Label, LineEdit, ScrollContainer, ItemList, Tree
│   ├── main/               # Node, SceneTree, Viewport, Window, CanvasLayer, MultiWindow
│   └── resources/          # Texture, Mesh, Material, Shader, Environment, Curve, Font
├── servers/                # Low-level stateful server subsystems
│   ├── rendering/          # RenderingServer facade, RendererRD, RenderSceneRender, Shaders
│   ├── physics_3d/         # PhysicsServer3D abstract interface & RID management
│   ├── physics_2d/         # PhysicsServer2D abstract interface & RID management
│   ├── audio/              # AudioServer, AudioDriver, AudioStream playback mixer thread
│   ├── display/            # DisplayServer abstract interface (Windows, Events, Native Menus)
│   ├── text/               # TextServer abstract interface (HarfBuzz, FreeType, ICU)
│   └── navigation/         # NavigationServer2D and NavigationServer3D state managers
├── thirdparty/             # Third-party vendor C/C++ libraries (zlib, png, vulkan, etc.)
├── tests/                  # GoogleTest C++ unit test suites
├── doc/                    # XML class documentation schema and reference files
├── misc/                   # Clang format rules, build tools, icon resources, VS project templates
└── SConstruct              # SCons master build buildfile script
```

---

# 2. Complete Repository Directory Breakdown

## 2.1 `core/` Subsystem

`core/` contains zero dependencies on `scene/`, `servers/`, or `editor/`. It defines the fundamental object-oriented, memory, reflection, and type systems used across the entire engine.

```
                  +-----------------------+
                  |     core/object/      |
                  | Object, ClassDB, Sign |
                  +-----------+-----------+
                              |
     +------------------------+------------------------+
     |                        |                        |
+----+------------------+ +---+-------------------+ +--+--------------------+
|    core/variant/      | |     core/io/          | |     core/os/          |
| Variant, Array, Dict  | | FileAccess, Resource  | | OS, Memory, Thread    |
+-----------------------+ +-----------------------+ +-----------------------+
```

### Subdirectories & Responsibilities:
- **`core/object/`**:
  - `object.h/.cpp`: The base `Object` class. Implements runtime type inspection, metadata, signals, dynamic properties, and `NOTIFICATION_POSTINITIALIZE`.
  - `class_db.h/.cpp`: Global reflection database registering C++ classes, methods (`MethodBind`), properties, enums, and constants for GDScript/GDExtension visibility.
  - `ref_counted.h/.cpp`: Reference-counted base class (`RefCounted`). Uses `SafeNumeric<uint32_t>` for thread-safe atomic reference counter tracking (`Ref<T>`).
  - `message_queue.h/.cpp`: Thread-safe deferred method call dispatcher (`call_deferred`, signal emissions).
  - `script_language.h/.cpp`: Base interface for scripting language plugins (`Script`, `ScriptInstance`, `ScriptLanguage`).
- **`core/variant/`**:
  - `variant.h/.cpp`: Discriminator union (`Variant`) capable of storing 38 distinct types (scalars, math vectors, String, Object*, RefCounted*, Array, Dictionary, Callable, Signal, RID).
  - `callable.h/.cpp`: Encapsulates a object instance + method name OR dynamic C++ function callback lambda.
  - `binder_common.h`: Template metaprogramming helpers used by `ClassDB::bind_method()` to auto-unwrap Variant argument arrays into C++ native function calls.
- **`core/io/`**:
  - `resource.h/.cpp`: Base class for persistent objects (`Resource`). Manages asset path (`resource_path`), UID, and self-contained serialization.
  - `resource_loader.h/.cpp` & `resource_saver.h/.cpp`: Registry for format-specific loaders/savers (`ResourceFormatLoader`, `ResourceFormatSaver`).
  - `file_access.h/.cpp` & `dir_access.h/.cpp`: Virtual filesystem interface transparently routing paths (`res://`, `user://`) to disk or zipped `.pck` archives (`PackedData`).
- **`core/os/`**:
  - `os.h/.cpp`: Abstract interface for OS interaction (environment variables, process execution, current time, alert popups).
  - `memory.h/.cpp`: Low-level custom memory allocators (`memalloc`, `memrealloc`, `memfree`, `memnew`, `memdelete`). Tracing memory allocations when `DEBUG_ENABLED` is active.
  - `worker_thread_pool.h/.cpp`: High-performance multi-threaded task queue for background asset loading, lightbaking, and parallel processing.
- **`core/templates/`**: Custom, zero-allocation container headers (`Vector`, `HashMap`, `HashSet`, `List`, `LocalVector`, `PagedAllocator`, `RID_Owner`).

---

## 2.2 `servers/` Subsystem

`servers/` contains low-level, stateful subsystems that process raw engine data. Servers run asynchronously or synchronously via handle-based resource IDs (**`RID`**).

```
                      +-------------------------+
                      |   servers/rendering/    |
                      |  RenderingServerFacade  |
                      +------------+------------+
                                   |
           +-----------------------+-----------------------+
           |                                               |
+----------+----------------+                    +---------+----------------+
|  servers/rendering/rd/    |                    | servers/physics_3d/    |
|   RendererRD (Vulkan/D3D) |                    |  PhysicsServer3D (RID) |
+---------------------------+                    +------------------------+
```

### Key Subsystems & Files:
- **`servers/rendering/`**:
  - `rendering_server.h/.cpp`: The top-level singleton interface (`RenderingServer::get_singleton()`). Operates via commands and returns RIDs (e.g., `mesh_create()`, `material_create()`, `instance_create()`).
  - `renderer_rd/`: Vulkan/D3D12/Metal Modern Rendering Backend (`RendererRD`). Manages Render Graphs, Rasterization passes, Compute Shaders, SDFGI, and Forward+/Mobile pipelines.
  - `renderer_storage/`: Storage classes for Meshes, Textures, Materials, Shaders, and Utilities on the GPU.
- **`servers/physics_3d/` & `servers/physics_2d/`**:
  - `physics_server_3d.h/.cpp` & `physics_server_2d.h/.cpp`: Pure abstract interfaces for physics world manipulation (`body_create()`, `shape_create()`, `area_create()`, `joint_create()`).
  - Backends are plugged in via modules (`modules/godot_physics_3d`, `modules/jolt_physics`).
- **`servers/display/`**:
  - `display_server.h/.cpp`: Abstract layer managing OS native windows, screens, native cursors, mouse/keyboard/touch events, clipboard, and rendering surface setup.
- **`servers/audio/`**:
  - `audio_server.h/.cpp`: High-speed audio processing unit. Manages audio buses, audio effect stacks, spatial audio listener positioning, and sample mixing buffers.
- **`servers/text/`**:
  - `text_server.h/.cpp`: Complex text layout engine handling font rasterization (FreeType), BiDi text shaping (HarfBuzz), and line breaking (ICU).

---

## 2.3 `scene/` Subsystem

`scene/` implements the retained-mode scene graph, nodes, spatial transformations, UI controls, and resources.

```
                        +----------------------+
                        |   scene/main/Node    |
                        +----------+-----------+
                                   |
         +-------------------------+-------------------------+
         |                                                   |
+--------+--------+                                 +--------+--------+
|  scene/2d/      |                                 |  scene/3d/      |
|  CanvasItem     |                                 |  Node3D         |
+--------+--------+                                 +--------+--------+
         |                                                   |
+--------+--------+                                 +--------+--------+
|  scene/gui/     |                                 | scene/resources/|
|  Control        |                                 | Mesh, Material  |
+-----------------+                                 +-----------------+
```

### Key Subsystems & Files:
- **`scene/main/`**:
  - `node.h/.cpp`: Core building block of the scene graph. Manages parent-child lists, tree enter/exit notifications, process flags, signals, groups, and node paths.
  - `scene_tree.h/.cpp`: MainLoop implementation managing active scene trees, main viewports, physics/idle process loops, groups, and pause states.
  - `viewport.h/.cpp` & `window.h/.cpp`: Viewport rendering targets, multi-window routing, 2D/3D camera assignment, and input event propagation.
- **`scene/3d/`**:
  - `node_3d.h/.cpp`: Base class for spatial objects containing a 3D `Transform3D` (Basis + Vector3 position).
  - `visual_instance_3d.h/.cpp` & `mesh_instance_3d.h/.cpp`: Binds high-level 3D nodes to `RenderingServer` visual instances via RIDs.
  - `physics_body_3d.h/.cpp` (`RigidBody3D`, `CharacterBody3D`, `StaticBody3D`): Wraps `PhysicsServer3D` bodies for high-level scripting.
- **`scene/2d/`**:
  - `canvas_item.h/.cpp` & `node_2d.h/.cpp`: Base classes for 2D spatial objects containing `Transform2D` and CanvasItem draw orders.
- **`scene/gui/`**:
  - `control.h/.cpp`: Base class for all UI widgets. Manages GUI layout anchors, margins, focus handling, custom minimum sizing, and theme overrides.
- **`scene/animation/`**:
  - `animation_player.h/.cpp` & `animation_tree.h/.cpp`: Animation playback, track blending, state machines, and property interpolation.

---

## 2.4 `editor/` Subsystem

The Godot Editor is itself written entirely as a Godot Application using `Control`, `Tree`, `GraphEdit`, and `Node3D`.

```
                     +---------------------------+
                     |    editor/editor_node.cpp |
                     |      (Editor App Root)    |
                     +-------------+-------------+
                                   |
    +------------------------------+------------------------------+
    |                              |                              |
+---+-------------------+ +--------+---------------+ +------------+------------+
| editor/inspector/     | | editor/docks/          | | editor/plugins/         |
| EditorInspector       | | FileSystemDock, Scene  | | Node3DGizmoPlugin       |
+-----------------------+ +------------------------+ +-------------------------+
```

### Key Files:
- `editor/editor_node.h/.cpp`: Primary entry point for the editor application. Initializes editor UI layouts, docks, main screens (2D, 3D, Script, AssetLib), menu bars, and project settings.
- `editor/editor_interface.h/.cpp`: Public API scriptable bridge allowing EditorPlugins to interact with editor state.
- `editor/inspector/editor_inspector.h/.cpp`: Dynamic property inspector generating UI editors for exported node properties.
- `editor/file_system/editor_file_system.h/.cpp`: Background thread directory scanner. Monitors file modifications, updates asset dependencies, generates UIDs, and triggers importers.
- `editor/plugins/`: Contains built-in tools (Node3D/Node2D gizmos, TileMap editor, Shader editor, AnimationCurve editor).

---

## 2.5 `main/` Subsystem

`main/` orchestrates engine bootup, command line parsing, frame loop scheduling, and shutdown.

### Key Files:
- `main/main.h/.cpp`:
  - `Main::setup(const char *execpath, int argc, char *argv[], bool p_second_stage)`: Parses command line flags, initializes low-level singletons (`OS`, `Memory`, `ClassDB`, `ProjectSettings`), creates DisplayServer, and boots Servers.
  - `Main::start()`: Boots `SceneTree`, creates `EditorNode` (if editor mode) or instantiates the user's root scene (if game mode).
  - `Main::iteration()`: Executes one single engine frame (processes input, physics step, idle process tick, render frame dispatch, sync timers).
  - `Main::cleanup()`: Tears down all singletons in strict reverse order to prevent dangling pointers.
- `main/main_timer_sync.h/.cpp`: Manages frame delta smoothing, target FPS capping, and fixed timestep accumulation for physics.

---

## 2.6 `drivers/` Subsystem

`drivers/` implements low-level graphics, audio, OS integration, and controller interfaces.

- **Graphics Drivers**:
  - `drivers/vulkan/`: Vulkan API abstraction layer for `RenderingDevice`. Manages VkDevice, VkQueue, Vulkan Memory Allocator (VMA), pipeline layout caches, descriptor sets.
  - `drivers/gles3/`: OpenGL ES 3.0 driver backend for low-spec compatibility rendering (`RendererCompatibility`).
  - `drivers/d3d12/`: DirectX 12 driver for Windows / Xbox builds.
  - `drivers/metal/`: Apple Metal graphics API driver for macOS/iOS.
- **Audio Drivers**:
  - `drivers/wasapi/`, `drivers/alsa/`, `drivers/coreaudio/`, `drivers/pulseaudio/`: Low-latency audio device buffers for Windows, Linux, macOS, and POSIX targets.

---

## 2.7 `platform/` Subsystem

`platform/` contains platform-specific entry points and OS abstraction overrides.

- `platform/windows/godot_windows.cpp`: Windows WinMain entry point. Initializes COM, HINSTANCE, UTF-16 command-line arguments, and spawns `Main::setup()`.
- `platform/linuxbsd/godot_linuxbsd.cpp`: Linux main entry point. Initializes X11/Wayland signals, POSIX threads, environment paths.
- `platform/android/java_godot_wrapper.cpp`: Android JNI glue interfacing Android Activity lifecycle with Godot C++ runtime.
- `platform/web/godot_web.cpp`: Emscripten web main loop wrapper hooking HTML5 RequestAnimationFrame into `Main::iteration()`.

---

## 2.8 `modules/` Subsystem

`modules/` contains optional or isolated feature implementations compiled conditionally via SCons.

```
                     +---------------------------+
                     |    modules/SCsub Master   |
                     +-------------+-------------+
                                   |
    +------------------------------+------------------------------+
    |                              |                              |
+---+-------------------+ +--------+---------------+ +------------+------------+
| modules/gdscript/     | | modules/mono/          | | modules/jolt_physics/   |
| GDScript Language     | | C# .NET Bindings Host  | | Jolt Physics Server     |
+-----------------------+ +------------------------+ +-------------------------+
```

### Key Modules:
- `modules/gdscript/`: Full GDScript language compiler, lexer, AST parser, byte-code generator (`gdscript_compiler.cpp`), and stack-based virtual machine (`gdscript_vm.cpp`).
- `modules/mono/`: C# language host binding Mono/CoreCLR runtime via C API to execute `.NET` assemblies (`GodotSharp`).
- `modules/gltf/`: GLTF 2.0 importer/exporter pipeline converting 3D GLTF assets to Godot `Node3D`, `MeshInstance3D`, and `AnimationPlayer` nodes.
- `modules/godot_physics_3d/`: Default internal 3D physics server implementing custom collision algorithms (GJK, EPA, SAT) and constraint solvers.
- `modules/jolt_physics/`: High-performance multithreaded 3D physics engine integration replacing or running alongside Godot Physics.

---

## 2.9 `thirdparty/`, `tests/`, `doc/`, `misc/`

- **`thirdparty/`**: Contains vendor source code kept untampered (e.g. `zlib`, `libpng`, `freetype`, `harfbuzz`, `vulkan`, `enet`, `mbedtls`). SCons scripts compile these directly into the Godot executable binary.
- **`tests/`**: GoogleTest framework suites validating `Core`, `Math`, `Variant`, `String`, and `Navigation` logic.
- **`doc/`**: XML documentation files (`doc/classes/*.xml`) describing Godot C++ classes for the built-in editor documentation and online manual.

---

# 3. Engine Boot & Shutdown Process

Godot boots through a deterministic 8-phase lifecycle sequence:

```
[OS Entry Point] (platform/windows/godot_windows.cpp)
       │
       ▼
[Phase 1: Main::setup()] ──► Init Memory, OS, ProjectSettings, Command-Line
       │
       ▼
[Phase 2: Core Bootstrap] ──► Register ClassDB, Variant, WorkerThreadPool
       │
       ▼
[Phase 3: DisplayServer] ──► Win32/X11 Window Setup, GPU Context Creation
       │
       ▼
[Phase 4: Servers Init] ───► Boot RenderingServer, PhysicsServer, AudioServer
       │
       ▼
[Phase 5: Scene System] ───► Create SceneTree, Viewports, Root Window
       │
       ▼
[Phase 6: App Launch] ───► Launch EditorNode (Editor) OR User Scene (Game)
       │
       ▼
[Phase 7: Main Loop] ──────► Loop Main::iteration() until quit signal
       │
       ▼
[Phase 8: Main::cleanup()] ─► Tear down SceneTree ──► Servers ──► Memory
```

## Step-by-Step Execution Chain

1. **Executable Entry Point**:
   - `platform/windows/godot_windows.cpp` -> `wide_char_to_utf8()` -> calls `main(argc, argv)` inside `main/main.cpp`.
2. **Phase 1: Early OS & Project Settings Setup**:
   - `Main::setup()` initializes `Memory`, `OS::get_singleton()`, parses CLI flags (`--editor`, `--project-path`, `--headless`).
   - `ProjectSettings::get_singleton()->setup()` loads `project.godot` configuration file into memory.
3. **Phase 2: Core Type & ClassDB Bootstrap**:
   - Calls `register_core_types()` (`core/register_core_types.cpp`). Registers `Object`, `RefCounted`, `Resource`, `Variant`, `StringName`.
   - Starts `WorkerThreadPool::get_singleton()->init()`.
4. **Phase 3: DisplayServer & Rendering Hardware Setup**:
   - `DisplayServer::create()` instantiates platform-specific display layer (e.g., `DisplayServerWindows`). Creates native HWND window and Vulkan/OpenGL swapchain surface context.
5. **Phase 4: Low-Level Servers Initialization**:
   - Calls `register_server_types()`. Instantiates `RenderingServer`, `PhysicsServer3D`, `PhysicsServer2D`, `AudioServer`, `TextServer`.
   - `RenderingServer::get_singleton()->init()` starts GPU pipeline storage, shader compilers, and render queues.
6. **Phase 5: Scene Subsystem & Scripting Engines**:
   - Calls `register_scene_types()`. Registers `Node`, `Node3D`, `Control`, `CanvasItem`, `Viewport`.
   - Script languages initialize: `GDScriptLanguage::get_singleton()->init()`.
7. **Phase 6: App Launch Mode Selection**:
   - If `--editor` or `editor` binary target: instantiates `EditorNode`, builds editor UI hierarchy.
   - If game project mode: loads root scene designated in `project.godot` (`application/run/main_scene`) via `ResourceLoader::load()`, instantiates scene tree, attaches to `SceneTree::get_singleton()->get_root()`.
8. **Phase 7: Main Execution Loop (`Main::iteration()`)**:
   - **Input Processing**: `DisplayServer::process_events()`, dispatches `InputEvent` objects to `SceneTree::input_event()`.
   - **Physics Step**: `MainTimerSync` evaluates delta time accumulators; executes `PhysicsServer3D::step()` and triggers `NOTIFICATION_PHYSICS_PROCESS` on active nodes.
   - **Idle Process Step**: Triggers `NOTIFICATION_PROCESS` across scene graph nodes.
   - **Deferred Message Queue**: `MessageQueue::get_singleton()->flush()` processes `call_deferred` calls and emitted signals.
   - **Render Dispatch**: `RenderingServer::get_singleton()->draw()` renders frame buffers to screen.
   - **Swap Buffers**: `DisplayServer::swap_buffers()`.
9. **Phase 8: Engine Tear-Down (`Main::cleanup()`)**:
   - `SceneTree` freed, destroying all active nodes.
   - Script languages uninitialized (`GDScriptLanguage::get_singleton()->finish()`).
   - Servers shut down in reverse dependency order: `RenderingServer` -> `PhysicsServer3D` -> `AudioServer` -> `DisplayServer`.
   - `ClassDB` cleared, static singletons freed, custom memory leak checker executed.

---

# 4. Runtime Core Subsystems Architecture

## 4.1 Object System & ClassDB Reflection

All reflected engine classes inherit from `Object` (`core/object/object.h`).

```
                    +-------------------+
                    |      Object       |
                    | (core/object.h)   |
                    +---------+---------+
                              |
                    +---------+---------+
                    |    RefCounted     |
                    | (core/ref_counted)|
                    +---------+---------+
                              |
                    +---------+---------+
                    |     Resource      |
                    |  (core/io/res.h)  |
                    +-------------------+
```

- **Runtime Type Information (RTTI)**: Godot bypasses standard C++ RTTI (`typeid`) in favor of light macros: `GDCLASS(DerivedClass, ParentClass)`.
- **`ClassDB`**: Global static registry (`core/object/class_db.h`). Holds method tables (`MethodBind`), property lists (`PropertyInfo`), enums, and constructor pointers.
- **Dynamic Property Binding**: Method bindings are constructed via templates (`ClassDB::bind_method(D_METHOD("set_position", "position"), &Node3D::set_position)`), automatically validating type signatures using `Variant`.

## 4.2 Variant & Signal Dispatch System

- **`Variant` (`core/variant/variant.h`)**: 20-byte tagged union representing any scriptable or C++ type. Contains an explicit type field (`Variant::Type`) and payload storage for primitive data types or pointers (`Object*`, `Ref<RefCounted>`, `RID`).
- **Signals & Callables**:
  - `Signal`: Pair of `(Object* source, StringName signal_name)`.
  - `Callable`: Pair of `(Object* target, StringName method_name)` OR custom C++ lambda function (`CallableCustom`).
  - Signal emission iterates over an internal array of connected `Callable` instances. Deferred signal emissions push command payloads to `MessageQueue`.

## 4.3 Virtual Filesystem & IO

- Paths starting with `res://` map to the project root directory or `.pck` archive payload.
- Paths starting with `user://` map to per-OS persistent app data folders (`AppData/Roaming/Godot` on Windows, `~/.local/share/godot` on Linux).
- `FileAccess` and `DirAccess` dynamically instantiate OS implementations (`FileAccessUnix`, `FileAccessWindows`, `FileAccessPack`) based on runtime file location.

---

# 5. Scene System Architecture

The Scene System represents the runtime scene graph.

```
                        +------------------+
                        |    SceneTree     |
                        +--------+---------+
                                 |
                        +--------+---------+
                        |  Window (Root)   |
                        +--------+---------+
                                 |
                        +--------+---------+
                        |     Viewport     |
                        +--------+---------+
                                 |
              +------------------+------------------+
              |                                     |
     +--------+--------+                   +--------+--------+
     |     Node3D      |                   |     Control     |
     +-----------------+                   +-----------------+
```

## Node Lifecycle & Notifications

Nodes receive state changes via integer notification IDs delivered to `Node::_notification(int p_what)`:

1. `NOTIFICATION_POSTINITIALIZE`: Object memory allocated, properties set.
2. `NOTIFICATION_ENTER_TREE`: Node inserted into an active `SceneTree` hierarchy.
3. `NOTIFICATION_READY`: Node and ALL child subtree nodes have processed `NOTIFICATION_ENTER_TREE`. Triggered child-first, root-last.
4. `NOTIFICATION_PROCESS`: Called once per rendered frame when `set_process(true)` is enabled.
5. `NOTIFICATION_PHYSICS_PROCESS`: Called on fixed physics timesteps when `set_physics_process(true)` is enabled.
6. `NOTIFICATION_EXIT_TREE`: Node removed from active `SceneTree`.
7. `NOTIFICATION_PREDELETE`: Object destructor about to execute.

## Scene Tree Instantiation & PackedScene

- A scene file (`.tscn` or `.scn`) deserializes into a `PackedScene` resource (`scene/resources/packed_scene.h`).
- `PackedScene::instantiate()` constructs the root `Node` and recursively instantiates child nodes, reapplying saved property overrides.
- **Node Ownership (`Node::set_owner()`)**: Node serialization only saves nodes whose `owner` property points to the root node of the scene being saved.

---

# 6. Rendering Architecture & Pipeline

Godot's rendering engine isolates high-level visual representation (`MeshInstance3D`, `Light3D`) from GPU hardware execution using a **Client-Server Architecture**.

```
[MeshInstance3D] (Scene Layer)
       │
       │ (Sends Mesh RID & Transform3D)
       ▼
[RenderingServer] (Facade / Server API)
       │
       │ (Commands queued into Render Queue)
       ▼
[RendererRD] (Modern Vulkan/D3D12/Metal Driver Engine)
       │
       ├── RendererSceneRenderRD (Draw Pass / Pass Graph Setup)
       ├── RenderForwardClustered (Forward+ Depth Prepass & Light Clustering)
       └── ShaderCompilerRD (Generates SPIR-V / DXIL / MSL Shaders)
       │
       ▼
[RenderingDevice] (Hardware Abstraction Layer)
       │
       ▼
[Vulkan / Direct3D12 / Metal Driver] ──► [GPU Hardware]
```

## Render Pass Pipeline (Forward+)

1. **Depth Prepass**: Renders opaque geometry depth into a depth buffer to eliminate overdraw during pixel shading.
2. **Light Clustering**: Computes screen-space 3D frustum clusters (3D tile grid) using Compute Shaders (`cluster_render_rd.cpp`) to bin PointLights and SpotLights into cluster buffers.
3. **Base Opaque Pass**: Renders opaque materials, executing PBR BRDF lighting shaders referencing the light clusters.
4. **Global Illumination Pass**: Accumulates indirect lighting contributions from SDFGI, VoxelGI, or LightmapGI lightmaps.
5. **Sky & Background Pass**: Renders skybox shaders.
6. **Transparent Pass**: Back-to-front sorted render of semi-transparent geometry.
7. **Post-Processing Pass**: Tone-mapping, Bloom, Depth of Field, FXAA/TAA, and Motion Blur applied via full-screen compute passes.

---

# 7. Physics Subsystem Architecture

Godot decouples physics simulation state into `PhysicsServer3D` (`servers/physics_3d/physics_server_3d.h`).

```
[CharacterBody3D / RigidBody3D] (Scene Layer Nodes)
       │
       │ (Update Transform3D & Velocity)
       ▼
[PhysicsServer3D] (Abstract RID Server API)
       │
       ▼
[GodotPhysics3D / Jolt Physics Module] (Backend Solver Engine)
       │
       ├── Broadphase: Dynamic AABB Tree / BVH (Finds potential overlapping pairs)
       ├── Narrowphase: GJK + EPA / SAT Algorithms (Computes exact contact points)
       └── Constraint Solver: Sequential Impulse Solver (Resolves velocities & impulses)
```

## Physics Simulation Iteration Step

1. **Integrate Forces**: Updates velocities based on gravity and applied forces (`body_set_force()`).
2. **Broadphase Query**: Identifies bounding-box overlaps using spatial partitioning trees (`broadphase_bvh.h`).
3. **Narrowphase Collision Detection**: Computes exact manifold contact points, normals, and penetration depths using GJK (Gilbert-Johnson-Keerthi) and EPA (Expanding Polytope Algorithm).
4. **Constraint & Impulse Solving**: Solves velocity and position constraints iteratively for rigid bodies and joints.
5. **Integrate Velocities**: Moves bodies to their final transformed coordinates and notifies `SceneTree` nodes.

---

# 8. Audio Subsystem Architecture

Audio processing takes place on a high-priority, low-latency dedicated audio thread managed by `AudioServer` (`servers/audio/audio_server.h`).

```
[AudioStreamPlayer3D] (Scene Node)
       │
       │ (Calculates 3D Attenuation & Panning based on Camera3D)
       ▼
[AudioServer] (Mixing Thread Engine)
       │
       ├── Audio Bus 0 (Master)
       │     ├── Audio Effect 1 (Reverb)
       │     └── Audio Effect 2 (Limiter)
       ├── Audio Bus 1 (Music)
       └── Audio Bus 2 (SFX)
       │
       ▼
[AudioDriver] (WASAPI / ALSA / CoreAudio Device Buffer)
```

- **Audio Streams**: `AudioStream` resources generate raw PCM audio samples via `AudioStreamPlayback`.
- **Bus Routing**: Each node outputs audio into a designated Audio Bus (`AudioServer::Bus`). Audio buses can be chained and fitted with realtime DSP effect instances (`AudioEffectInstance`).
- **Spatial 3D Audio**: `AudioStreamPlayer3D` calculates 3D distance attenuation curves, Doppler pitch shifts, and stereo/surround panning matrices relative to active `AudioListener3D` nodes.

---

# 9. Animation & Skinning Subsystem

Animation processing integrates skeletal skinning and property interpolation across scene nodes.

```
[AnimationPlayer / AnimationTree] (Scene Layer)
       │
       │ (Evaluates Track Curves & Blend Trees)
       ▼
[Skeleton3D] (Bone Hierarchy Container)
       │
       │ (Computes Local Transforms -> Global Transforms -> Skinning Matrices)
       ▼
[RenderingServer] (GPU Mesh Skinning)
       │
       ▼
[Vertex Shader] (Applies Bone Weights & Matrices to Mesh Vertices on GPU)
```

- **Track Interpolation**: `Animation` resources contain tracks targeting specific `NodePath` properties. Supports step, linear, cubic, and bezier curve interpolations.
- **`AnimationTree`**: Evaluates complex animation state machines, blend trees (`AnimationNodeBlendTree`), 2D directional blend spaces (`AnimationNodeBlendSpace2D`), and IK solvers.
- **Skeletal Mesh Skinning**: `Skeleton3D` updates bone matrices. These matrices are transferred to the GPU as uniform arrays or texture buffers, where vertex shaders execute dual-quaternion or linear blend skinning.

---

# 10. Resource System & Asset Import Pipeline

## Resource Lifecycle & UIDs

- **Resource Identity**: Every asset in Godot is assigned a 64-bit Unique Identifier (`ResourceUID`), stored in `.import` files (e.g. `uid://c8x12m4q9k7p`).
- **Caching**: `ResourceLoader` maintains a weak-reference cache of loaded resources (`ResourceCache`). Submitting a request for an already-loaded resource path returns the existing `Ref<Resource>` instance.

## Asset Import Pipeline Flow

```
[Raw Asset File] (e.g., character.gltf, texture.png)
       │
       │ (Detected by EditorFileSystem Background Thread)
       ▼
[EditorImportPlugin] (Matches File Extension)
       │
       │ (Executes Import Logic & Formats Binary Data)
       ▼
[.godot/imported/ Directory Payload] (e.g., character.scn, texture.ctex)
       │
       └── Writes .import Metadata File (Holds UUID, Hash, Import Parameters)
```

1. **FileSystem Scanning**: `EditorFileSystem` scans project folders for new or updated raw files.
2. **Importer Match**: Discovers registered `EditorImportPlugin` instances matching the file extension.
3. **Import Execution**:
   - `TextureImporter`: Compresses raw PNG/JPG images into Basis Universal or GPU-compressed formats (S3TC, ETC2, ASTC) and outputs `.ctex` files.
   - `SceneImporter`: Converts `.gltf`/`.fbx` files into optimized binary `.scn` scene resources.
4. **Metadata Persistence**: Generates a `.import` text file adjacent to the raw asset storing import configurations and UID mappings.

---

# 11. Editor Architecture & Tool Mode

The Godot Editor is a Godot executable running with `--editor` flags.

```
+-----------------------------------------------------------------------+
|                              EditorNode                               |
+-----------------------------------------------------------------------+
|  Menu Bar | Main Screen Buttons (2D, 3D, Script, AssetLib)            |
+---------------------+---------------------------+---------------------+
| FileSystem Dock     | Main Spatial Editor View  | Inspector Dock      |
| (EditorFileSystem)  | (Node3DEditorViewport)    | (EditorInspector)   |
|                     |                           |                     |
| Scene Tree Dock     | Bottom Panel              | Node / Signals Dock |
| (SceneTreeDock)     | (Output, Debugger, Audio) | (ConnectionsDock)   |
+---------------------+---------------------------+---------------------+
```

- **`EditorNode` (`editor/editor_node.h`)**: The root node container managing layout frames, dock initialization, editor themes, and main screen switching.
- **Tool Mode (`@tool`)**: When a script is marked with `@tool`, its `_process()`, `_ready()`, and notification callbacks execute inside the Editor viewport environment.
- **`EditorUndoRedoManager`**: Handles atomic multi-step action stacks for undoing and redoing editor modifications.
- **Inspector Engine (`EditorInspector`)**: Parses `PropertyInfo` metadata returned by `ClassDB` to instantiate custom property control editors (`EditorProperty`).

---

# 12. Module System vs. GDExtension Architecture

Godot provides two mechanisms for extending engine capabilities in C++:

```
+-----------------------------------+-----------------------------------+
|          BUILT-IN MODULES         |            GDEXTENSION            |
+-----------------------------------+-----------------------------------+
| Compiled directly into binary.    | Shared library (.dll / .so / .dylib)|
| Defined under modules/<name>/.    | Loaded dynamically at runtime.    |
| Static linkage with engine code.  | Interfacing via C API ABI.        |
| Accesses all internal headers.    | Uses C++ bindings wrapper.        |
| Requires full engine recompilation| Recompiled independently of engine|
+-----------------------------------+-----------------------------------+
```

## Module Registration Hooks (`modules/<module_name>/register_types.cpp`)

Built-in modules expose two primary lifecycle hooks:

```cpp
void initialize_my_module_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<MyCustomNode>();
    }
}

void uninitialize_my_module_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        // Cleanup resources
    }
}
```

---

# 13. Build System & SCons Compilation Pipeline

Godot uses **SCons** for cross-platform build orchestration (`SConstruct` and `methods.py`).

## Key SCons Command-Line Switches

- `target=editor`: Builds the Godot Editor executable.
- `target=template_release`: Builds stripped, optimized export runtime templates for games.
- `target=template_debug`: Builds export runtime templates containing debugging symbols and assertions.
- `platform=windows|linuxbsd|macos|android|ios|web`: Selects target platform build toolchain.
- `scu=yes`: Enables Single Compilation Unit (SCU) build mode, concatenating multiple C++ source files into single translation units to increase compilation speeds.
- `custom_modules="path/to/module"`: Compiles external C++ module folders outside the core repository.

---

# 14. Platform Layer & Hardware Abstraction

Godot isolates platform dependencies using dual abstractions: `OS` and `DisplayServer`.

```
[Scene Graph / Engine Logic]
       │
       ├── Calls OS::get_singleton() (Environment, Threads, Time, Paths)
       └── Calls DisplayServer::get_singleton() (Windows, Mouse, Cursors, GPU Surfaces)
              │
              ├── platform/windows  ──► (Win32 API / HWND / DirectX)
              ├── platform/linuxbsd ──► (X11 / Wayland / POSIX)
              ├── platform/macos    ──► (Cocoa / AppKit / Metal)
              └── platform/android  ──► (Android NDK / JNI / SurfaceView)
```

## Input Event Pipeline

1. Native platform events (e.g. `WM_KEYDOWN` on Windows) caught by `DisplayServerWindows`.
2. Encapsulated into an `InputEventKey` or `InputEventMouseButton` object (`core/input/input_event.h`).
3. Sent to `Input::get_singleton()->parse_input_event()`.
4. Routed to `SceneTree::root_window` -> active `Viewport` -> Focused `Control` UI widget -> `Node::_unhandled_input()`.

---

# 15. Memory Management & Multithreading Architecture

## Custom Memory Management

Godot avoids standard C++ dynamic memory allocations (`new`/`delete`) to track leaks and enforce custom pool alignment:

- `memnew(Class)`: Allocates memory and invokes constructor.
- `memdelete(instance)`: Invokes destructor and releases memory through `Memory::free_static()`.
- `Ref<T>` (`core/object/ref_counted.h`): Smart pointer wrapper maintaining reference counting on `RefCounted` derivatives. Thread-safe increment/decrement via atomics (`SafeNumeric<uint32_t>`).

## Multithreading Architecture

- **`WorkerThreadPool`**: Primary worker pool executing split tasks in parallel across CPU cores.
- **Rendering Threading Modes**:
  - *Single-Threaded*: Scene and rendering executed sequentially on main thread.
  - *Threaded Command Queue (`RenderQueue`)*: Main thread pushes rendering commands into a ring buffer; dedicated rendering thread processes GPU commands asynchronously.

---

# 16. Scripting Subsystem Architecture (GDScript & C#)

Scripting languages integrate with Godot by implementing the abstract interface `ScriptLanguage` (`core/object/script_language.h`).

```
[Script File (.gd)]
       │
       ▼
[GDScriptParser] (Lexical analysis & AST Tree Generation)
       │
       ▼
[GDScriptAnalyzer] (Type checking & constant folding)
       │
       ▼
[GDScriptCompiler] (Generates GDScript Bytecode Array)
       │
       ▼
[GDScriptVM] (Executes OpCodes inside stack-based Virtual Machine)
```

- **Script Instances**: When a script is attached to a `Node`, a `ScriptInstance` object is attached to the C++ `Object` instance. Method invocations on the object check `ScriptInstance::has_method()` before falling back to `ClassDB`.
- **C# / .NET Host**: `modules/mono/` hosts the CoreCLR runtime. Native-to-managed calls pass through C function pointers managed by GCHandles.

---

# 17. Networking & Debug Systems

## High-Level Multiplayer API

- `MultiplayerAPI` (`modules/multiplayer/`): Manages RPC (`@rpc`) calls, scene replication (`MultiplayerSynchronizer`), and authority management (`set_multiplayer_authority()`).
- Transport layer implemented via `MultiplayerPeer` implementations (`ENetMultiplayerPeer`, `WebSocketMultiplayerPeer`, `WebRTCMultiplayerPeer`).

## Engine Debugger & Remote Inspector

- `EngineDebugger` (`core/debugger/engine_debugger.h`): Remote debug server interfacing over TCP sockets.
- Transmits stack trace states, variable dumps, performance counter metrics, and remote node tree inspection payloads to the editor debugger panel (`editor/debugger/`).

---

# 18. Extension Points & "Where to Modify X" Practical Guide

Use this section to locate exact source files when customizing or replacing engine subsystems.

### 1. Want to Add or Modify a Graphics Render Pass / Shader
- **Primary Files**: `servers/rendering/renderer_rd/renderer_scene_render_rd.h/.cpp`, `servers/rendering/renderer_rd/shaders/`
- **Steps**:
  1. Add pass invocation method inside `RendererSceneRenderRD::_render_scene()`.
  2. Write GLSL shader files in `servers/rendering/renderer_rd/shaders/`.
  3. Rebuild GLSL header files using `glsl_builders.py`.

### 2. Want to Replace or Modify the 3D Physics Engine
- **Primary Files**: `servers/physics_3d/physics_server_3d.h`, `modules/godot_physics_3d/`, `modules/jolt_physics/`
- **Steps**:
  1. Create a class inheriting from `PhysicsServer3D`.
  2. Implement body, shape, area, joint creation methods and step functions.
  3. Register physics server factory inside `register_types.cpp` of your custom module.

### 3. Want to Add a Custom Core Node Type
- **Primary Files**: `scene/3d/` or `scene/2d/`, `scene/register_scene_types.cpp`
- **Steps**:
  1. Create `MyNode` inheriting from `Node3D` or `Control`.
  2. Use `GDCLASS(MyNode, Node3D)` macro.
  3. Override `_notification(int p_what)`.
  4. Register class inside `scene/register_scene_types.cpp` using `ClassDB::register_class<MyNode>()`.

### 4. Want to Create a Custom Asset Importer
- **Primary Files**: `editor/import/`, `editor/plugins/`
- **Steps**:
  1. Inherit from `EditorImportPlugin` (`editor/import/editor_import_plugin.h`).
  2. Override `get_importer_name()`, `get_recognized_extensions()`, `import()`.
  3. Register plugin inside `editor/register_editor_types.cpp` using `EditorFileSystem::add_import_plugin()`.

### 5. Want to Add a New Custom Scripting Language
- **Primary Files**: `core/object/script_language.h`
- **Steps**:
  1. Inherit from `ScriptLanguage`, `Script`, and `ScriptInstance`.
  2. Implement parsing, execution, and property getters/setters.
  3. Register custom language using `ScriptServer::register_language()`.

### 6. Want to Create a Custom Resource Format Saver / Loader
- **Primary Files**: `core/io/resource_loader.h`, `core/io/resource_saver.h`
- **Steps**:
  1. Inherit from `ResourceFormatLoader` and `ResourceFormatSaver`.
  2. Implement `load()`, `save()`, `get_recognized_extensions()`.
  3. Register using `ResourceLoader::add_resource_format_loader()`.

### 7. Want to Modify Window & Display Backends
- **Primary Files**: `servers/display/display_server.h`, `platform/<target>/display_server_<target>.h/.cpp`
- **Steps**: Modify platform display server files (e.g. `platform/windows/display_server_windows.cpp`).

---

# 19. System Dependency Graphs & Call Flow Maps

## 19.1 Engine Subsystem Dependency Flow Diagram

```
                +-------------------------------+
                |         platform/<os>         |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |          main/main.cpp        |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |             editor/           |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |             scene/            |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |            servers/           |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |            drivers/           |
                +---------------+---------------+
                                |
                                ▼
                +-------------------------------+
                |             core/             |
                +-------------------------------+
```

## 19.2 Per-Frame Main Loop Execution Flow

```
Main::iteration()
  │
  ├── DisplayServer::process_events() ────────► Pumps Native OS Windows Messages
  │
  ├── Input::parse_input_event() ──────────────► Dispatches InputEvents to SceneTree
  │
  ├── MainTimerSync::advance() ────────────────► Calculates physics delta accumulators
  │
  ├── PhysicsServer3D::step() ─────────────────► Runs Physics Step & Collision Solvers
  │     │
  │     └── Nodes receive NOTIFICATION_PHYSICS_PROCESS
  │
  ├── SceneTree::process() ────────────────────► Nodes receive NOTIFICATION_PROCESS
  │
  ├── MessageQueue::flush() ───────────────────► Calls call_deferred() & Signal emissions
  │
  ├── RenderingServer::draw() ─────────────────► Submits draw commands to Render Queue
  │
  └── DisplayServer::swap_buffers() ───────────► Swaps backbuffer to screen
```

---

# 20. Comprehensive Index of Key Engine Classes

| Class Name | File Location | Subsystem Layer | Core Responsibilities | Key Dependencies |
| :--- | :--- | :--- | :--- | :--- |
| `Object` | `core/object/object.h` | Core | Base class for all engine objects; handles signals, metadata, notifications. | None |
| `RefCounted` | `core/object/ref_counted.h` | Core | Atomic reference counting base class. | `Object` |
| `Variant` | `core/variant/variant.h` | Core | Dynamic type container union for 38 Godot engine types. | `Object`, `RID`, `String` |
| `ClassDB` | `core/object/class_db.h` | Core | Global C++ reflection database for method binds and property maps. | `Variant`, `Object` |
| `Resource` | `core/io/resource.h` | Core | Base persistent asset data class. | `RefCounted` |
| `RenderingServer` | `servers/rendering/rendering_server.h` | Servers | Facade singleton interface for graphics server. | `RID` |
| `RenderingDevice` | `servers/rendering/rendering_device.h` | Servers | Low-level GPU driver abstraction (Vulkan/D3D12/Metal). | Hardware Drivers |
| `PhysicsServer3D` | `servers/physics_3d/physics_server_3d.h` | Servers | Low-level 3D physics simulation facade server. | `RID` |
| `DisplayServer` | `servers/display/display_server.h` | Servers | Window management, display output, OS input backend. | Platform OS |
| `AudioServer` | `servers/audio/audio_server.h` | Servers | Realtime audio mixing, bus routing, audio thread control. | `AudioDriver` |
| `Node` | `scene/main/node.h` | Scene | Core building block for scene graph tree nodes. | `Object` |
| `Node3D` | `scene/3d/node_3d.h` | Scene | 3D spatial node container holding `Transform3D`. | `Node` |
| `Control` | `scene/gui/control.h` | Scene | Base class for UI widgets, anchors, layout sizing. | `CanvasItem` |
| `Viewport` | `scene/main/viewport.h` | Scene | Framebuffer rendering target and input event root. | `Node` |
| `SceneTree` | `scene/main/scene_tree.h` | Scene | Manages main execution loop, active scene tree hierarchy. | `MainLoop` |
| `EditorNode` | `editor/editor_node.h` | Editor | Main root UI controller for the Godot Editor application. | `Node`, `Control` |
| `EditorFileSystem` | `editor/file_system/editor_file_system.h` | Editor | Background thread asset scanner, dependency resolver, importer trigger. | `Thread` |
| `Main` | `main/main.h` | Main | Engine bootup, command parsing, frame loop iteration, tear-down logic. | Servers, SceneTree |
