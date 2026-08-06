# GODOTSOURCE.md: Exhaustive Godot Engine Source File Map & Single-Line Purpose Directory

> [!NOTE]
> **Purpose**: This reference maps out every source and header file across all major subdirectories (`core/`, `servers/`, `scene/`, `editor/`, `main/`, `drivers/`, `platform/`, `modules/`) in the Godot Engine (`Velvet-Engine` codebase), providing a single-line summary of each file's architectural responsibility.

---

# Table of Contents
1. [`core/` Engine Base System Files](#1-core-engine-base-system-files)
   - [1.1 `core/object/`](#11-coreobject)
   - [1.2 `core/variant/`](#12-corevariant)
   - [1.3 `core/io/`](#13-coreio)
   - [1.4 `core/os/`](#14-coreos)
   - [1.5 `core/math/`](#15-coremath)
   - [1.6 `core/extension/`](#16-coreextension)
   - [1.7 `core/debugger/`, `core/string/`, `core/crypto/`, `core/templates/`, `core/input/`, `core/config/`](#17-coredebugger-corestring-corecrypto-coretemplates-coreinput-coreconfig)
2. [`servers/` Low-Level Stateful Subsystem Files](#2-servers-low-level-stateful-subsystem-files)
   - [2.1 `servers/rendering/`](#21-serversrendering)
   - [2.2 `servers/physics_3d/` & `servers/physics_2d/`](#22-serversphysics_3d--serversphysics_2d)
   - [2.3 `servers/display/`](#23-serversdisplay)
   - [2.4 `servers/audio/`](#24-serversaudio)
   - [2.5 `servers/text/` & `servers/navigation/`](#25-serverstext--serversnavigation)
   - [2.6 `servers/camera/`, `servers/xr/`, `servers/movie_writer/`](#26-serverscamera-serversxr-serversmovie_writer)
3. [`scene/` Retained Scene Graph & Resource Files](#3-scene-retained-scene-graph--resource-files)
   - [3.1 `scene/main/`](#31-scenemain)
   - [3.2 `scene/3d/`](#32-scene3d)
   - [3.3 `scene/2d/`](#33-scene2d)
   - [3.4 `scene/gui/`](#34-scenegui)
   - [3.5 `scene/resources/`](#35-sceneresources)
   - [3.6 `scene/animation/` & `scene/audio/`](#36-sceneanimation--sceneaudio)
4. [`editor/` Godot Editor Suite Files](#4-editor-godot-editor-suite-files)
   - [4.1 `editor/` Core Editor Entry Points](#41-editor-core-editor-entry-points)
   - [4.2 `editor/inspector/` & `editor/docks/`](#42-editorinspector--editordocks)
   - [4.3 `editor/file_system/`, `editor/import/`, `editor/plugins/`](#43-editorfile_system-editorimport-editorplugins)
   - [4.4 `editor/export/`, `editor/gui/`, `editor/debugger/`, `editor/settings/`, `editor/project_manager/`](#44-editorexport-editorgui-editordebugger-editorsettings-editorproject_manager)
5. [`main/` Executable Lifecycle Files](#5-main-executable-lifecycle-files)
6. [`drivers/` Hardware Abstraction Backend Files](#6-drivers-hardware-abstraction-backend-files)
   - [6.1 `drivers/vulkan/`, `drivers/gles3/`, `drivers/d3d12/`, `drivers/metal/`](#61-driversvulkan-driversgles3-driversd3d12-driversmetal)
   - [6.2 Audio & Hardware Input Drivers](#62-audio--hardware-input-drivers)
7. [`platform/` Operating System Backend Files](#7-platform-operating-system-backend-files)
   - [7.1 `platform/windows/`, `platform/linuxbsd/`, `platform/macos/`](#71-platformwindows-platformlinuxbsd-platformmacos)
   - [7.2 `platform/android/`, `platform/ios/`, `platform/web/`](#72-platformandroid-platformios-platformweb)
8. [`modules/` Opt-in Engine Modules Index](#8-modules-opt-in-engine-modules-index)
   - [8.1 `modules/gdscript/`](#81-modulesgdscript)
   - [8.2 `modules/mono/` (C# .NET Host)](#82-modulesmono-c-net-host)
   - [8.3 `modules/gltf/`](#83-modulesgltf)
   - [8.4 `modules/godot_physics_3d/` & `modules/jolt_physics/`](#84-modulesgodot_physics_3d--modulesjolt_physics)
   - [8.5 Navigation, XR, CSG, Multi-player, and Utility Modules](#85-navigation-xr-csg-multi-player-and-utility-modules)

---

# 1. `core/` Engine Base System Files

## 1.1 `core/object/`
- `object.h`: Base `Object` class declaration providing dynamic property lists, signals, metadata, and lifecycle notification hooks.
- `object.cpp`: Implementation of `Object` signal connection tables, property getters/setters, call deferred dispatches, and notification handling.
- `class_db.h`: Global static reflection database header mapping C++ class names, methods, properties, and constants.
- `class_db.cpp`: Implementation of `ClassDB` type registration, method binding validation, dynamic instantiation, and class inheritance trees.
- `ref_counted.h`: Base `RefCounted` smart pointer class header with atomic thread-safe reference counting wrapper (`Ref<T>`).
- `ref_counted.cpp`: Implementation of reference counting increments, decrements, and auto-deletion logic when count reaches zero.
- `message_queue.h`: Thread-safe ring buffer header storing deferred method invocations and deferred signal emissions.
- `message_queue.cpp`: Implementation of `MessageQueue::flush()` executing pending deferred calls on the main loop thread.
- `script_language.h`: Abstract interface base classes (`Script`, `ScriptInstance`, `ScriptLanguage`) for binding scripting language VMs to objects.
- `script_language.cpp`: Core implementation of script server registration, script language iteration, and backtrace formatting.
- `script_language_extension.h`: GDExtension binding header enabling external shared libraries to implement custom script languages.
- `script_language_extension.cpp`: Implementation of GDExtension C-API bridges translating custom script calls into ClassDB method binds.
- `worker_thread_pool.h`: High-performance worker thread pool header scheduling parallel tasks and background group execution.
- `worker_thread_pool.cpp`: Implementation of worker thread execution loops, semaphore signaling, task queues, and wait primitives.
- `undo_redo.h`: Standalone non-editor action history manager header tracking undo/redo operation stacks.
- `undo_redo.cpp`: Implementation of `UndoRedo` action committing, do/undo method invocation sequences, and stack pruning.
- `callable_mp.h`: Template header wrapping C++ member function pointers into dynamic engine `Callable` instances.
- `callable_mp.cpp`: Native static helper routines for member function callable evaluation.
- `method_bind.h`: Header defining template wrappers converting `Variant` argument arrays into C++ native function invocations.
- `method_bind.cpp`: Base implementation of `MethodBind` name tracking, argument count checks, and default argument storage.
- `method_bind_common.h`: Template metaprogramming helpers for unwrapping variadic function signatures in `ClassDB`.
- `method_info.h`: Data structure header describing method parameters, return types, flags, and default argument values.
- `method_info.cpp`: Serialization helpers for `MethodInfo` definitions.
- `object_id.h`: Lightweight 64-bit integer identifier struct (`ObjectID`) used for safe instance lookup without raw pointer dereferencing.
- `property_info.h`: Struct header storing property metadata (type, name, hint, hint string, usage flags, class name).
- `property_info.cpp`: Conversion routines converting `PropertyInfo` structs into dictionary representations.
- `gdtype.h`: Type registration helpers for script engine object mappings.
- `gdtype.cpp`: Type conversion implementation for Godot script objects.
- `editor_language.h`: Helper definitions for editor translation string registration inside core object types.
- `script_backtrace.h`: Data structure header storing call stack frame snapshots during script exceptions or breakpoints.
- `script_backtrace.cpp`: Formatting routines converting script backstack structures into human-readable stack traces.
- `script_instance.h`: Interface header attached to individual object instances executing script-defined methods and properties.
- `script_instance.cpp`: Default empty implementation of script instance callbacks.

## 1.2 `core/variant/`
- `variant.h`: Discriminator union header (`Variant`) storing 38 distinct Godot primitive, math, object, array, and dictionary types.
- `variant.cpp`: Constructor, destructor, copy assignment, and type casting implementations for `Variant`.
- `variant_op.cpp`: Implementation of math and logic operator evaluations (+, -, *, /, ==, <) across all `Variant` types.
- `variant_call.cpp`: Implementation of built-in method invocation dispatchers for `Variant` types (e.g. `String::length()`).
- `variant_parser.h`: Text parser header reading plain-text variant representations from `.tscn` and `.godot` files.
- `variant_parser.cpp`: Implementation of text tokenizing, string unescaping, vector parsing, and dictionary text decoding.
- `callable.h`: Encapsulates a target `Object*` and method `StringName` or custom C++ lambda callback (`CallableCustom`).
- `callable.cpp`: Implementation of `Callable` call evaluation, target validity checks, and hashing.
- `signal.h`: Data container header storing a target `Object*` and signal `StringName` for event emissions.
- `signal.cpp`: Implementation of `Signal` connection queries and emission calls.
- `array.h`: Dynamic reference-counted untyped or typed vector payload wrapper class (`Array`).
- `array.cpp`: Implementation of `Array` element manipulation, sorting, searching, and type enforcement.
- `dictionary.h`: Key-value hash map dynamic container wrapper class (`Dictionary`).
- `dictionary.cpp`: Implementation of `Dictionary` key insertion, lookup, merging, hashing, and duplicate creation.

## 1.3 `core/io/`
- `resource.h`: Base class header for persistent engine data assets (`Resource`) handling paths, UIDs, and self-contained serialization.
- `resource.cpp`: Implementation of `Resource` path assignment, change notification signals, and reference cache management.
- `resource_loader.h`: Central manager header loading resources from disk paths (`res://`) or UIDs (`uid://`).
- `resource_loader.cpp`: Implementation of `ResourceLoader` format loader discovery, thread-safe loading queues, and caching.
- `resource_saver.h`: Central manager header serializing resources to binary (`.res`) or text (`.tres`) files.
- `resource_saver.cpp`: Implementation of `ResourceSaver` format saver matching and file writing dispatchers.
- `resource_uid.h`: Global 64-bit Unique Identifier registry header (`ResourceUID`) decoupling asset paths from disk locations.
- `resource_uid.cpp`: Implementation of UID text generation, disk path mapping tables, and `.import` file UID caching.
- `file_access.h`: Abstract virtual filesystem class header reading/writing raw binary files on local disk or inside `.pck` archives.
- `file_access.cpp`: Implementation of `FileAccess` endianness conversion, line reading, buffer writes, and factory drivers.
- `dir_access.h`: Abstract virtual filesystem class header for creating, navigating, listing, and deleting directories.
- `dir_access.cpp`: Implementation of `DirAccess` directory traversal, recursive directory copies, and OS-specific driver matching.
- `file_access_pack.h`: Custom file access driver header reading packed game assets from binary `.pck` archives.
- `file_access_pack.cpp`: Implementation of `.pck` index table decoding, offset calculation, and encrypted package reading.
- `file_access_zip.h`: Custom file access driver header reading compressed assets directly from `.zip` archives.
- `file_access_zip.cpp`: Implementation of unzipping stream decompression wrappers.
- `file_access_memory.h`: File access driver header wrapping memory buffers as virtual readable/writable files.
- `file_access_memory.cpp`: Implementation of memory buffer reads and writes.
- `file_access_compressed.h`: File access driver header reading compressed binary files (zstd, gzip, fastlz).
- `file_access_compressed.cpp`: Implementation of transparent file decompression on the fly.
- `file_access_encrypted.h`: File access driver header reading AES-encrypted binary files.
- `file_access_encrypted.cpp`: Implementation of AES-256 decryption on file read streams.
- `file_access_patched.h`: File access driver header applying delta patches to base files.
- `file_access_patched.cpp`: Implementation of binary patch application on read.
- `image.h`: Pixel data buffer container header for CPU-side 2D image manipulation, format conversion, and mipmap generation.
- `image.cpp`: Implementation of `Image` resizing, blitting, channel manipulation, color space conversion, and format decompression.
- `image_loader.h`: Importer registry header converting image file formats (PNG, JPG, WebP) into `Image` instances.
- `image_loader.cpp`: Implementation of image loader registration and extension matching.
- `json.h`: JSON format parser and serializer header converting JSON text to `Variant` data structures.
- `json.cpp`: Implementation of JSON string parsing, token validation, and string escaping.
- `config_file.h`: INI-style text file parser header reading and writing engine configuration files (`project.godot`).
- `config_file.cpp`: Implementation of section parsing, key-value variant serialization, and INI file saving.
- `http_client.h`: Low-level HTTP 1.1 protocol client header handling requests, headers, and payload streaming.
- `http_client.cpp`: Implementation of HTTP socket connections, response code parsing, chunked transfer decoding, and SSL support.
- `http_client_tcp.h`: HTTP client backend driver header utilizing TCP sockets directly.
- `http_client_tcp.cpp`: Implementation of TCP socket connection handshakes for HTTP requests.
- `tcp_server.h`: Low-level TCP socket server header listening for incoming network stream connections.
- `tcp_server.cpp`: Implementation of TCP port binding, connection listening, and stream peer spawning.
- `udp_server.h`: Low-level UDP socket server header receiving connectionless datagram packets.
- `udp_server.cpp`: Implementation of UDP port listening and packet routing.
- `ip.h`: Domain Name System (DNS) host resolution and IP address lookup singleton header.
- `ip.cpp`: Implementation of asynchronous DNS resolution threads and IP address conversion utilities.
- `ip_address.h`: Data structure header representing IPv4 and IPv6 network addresses.
- `ip_address.cpp`: Implementation of string parsing and byte packing for IP addresses.
- `stream_peer.h`: Abstract interface header for stream-based byte communication (sockets, memory buffers, SSL).
- `stream_peer.cpp`: Implementation of data primitive serialization (ints, floats, variants) over byte streams.
- `stream_peer_gzip.h`: Compression stream peer header compressing/decompressing byte streams with GZip/Zlib.
- `stream_peer_gzip.cpp`: Implementation of Zlib buffer compression on stream reads/writes.
- `stream_peer_tcp.h`: Stream peer driver header wrapping connected TCP sockets.
- `stream_peer_tcp.cpp`: Implementation of TCP socket data reads, writes, and non-blocking polling.
- `stream_peer_tls.h`: Stream peer driver header wrapping TLS/SSL encrypted socket streams.
- `stream_peer_tls.cpp`: Implementation of TLS handshake execution and encrypted buffer transfers.
- `packet_peer.h`: Abstract interface header for packet-based datagram network communication.
- `packet_peer.cpp`: Implementation of packet variant serialization and max packet size queries.
- `packet_peer_udp.h`: Packet peer driver header wrapping UDP sockets.
- `packet_peer_udp.cpp`: Implementation of UDP datagram packet sending, receiving, and socket binding.
- `packet_peer_dtls.h`: Packet peer driver header wrapping Datagram TLS encrypted UDP packets.
- `packet_peer_dtls.cpp`: Implementation of DTLS session handshakes over UDP.
- `pck_packer.h`: Pack file builder header creating binary `.pck` package files for game deployment.
- `pck_packer.cpp`: Implementation of package index generation, file alignment, and `.pck` file writing.
- `marshalls.h`: Binary marshalling helper header encoding primitive types into Base64 or byte arrays.
- `marshalls.cpp`: Implementation of Base64 encoding/decoding and variant byte marshalling.
- `xml_parser.h`: Lightweight XML text parser header reading XML elements, attributes, and CDATA.
- `xml_parser.cpp`: Implementation of XML string tokenization and element tree traversal.
- `zip_io.h`: Zip archive file IO helper header wrapping minizip C functions.
- `zip_io.cpp`: Implementation of ZIP archive entry reading and writing.

## 1.4 `core/os/`
- `os.h`: Primary operating system interface abstraction header for process execution, environment variables, time, and system metrics.
- `os.cpp`: Implementation of cross-platform fallback routines for OS metrics, alert dialogs, environment variables, and process execution.
- `memory.h`: Global memory allocation function headers (`memalloc`, `memrealloc`, `memfree`, `memnew`, `memdelete`).
- `memory.cpp`: Implementation of custom memory allocators, static memory tracing, and leak detection reporting on exit.
- `thread.h`: Cross-platform OS thread creation and execution lifecycle wrapper header.
- `thread.cpp`: Implementation of OS thread spawning, thread priority assignment, and thread join waiting.
- `mutex.h`: Cross-platform mutual exclusion lock header protecting critical shared memory sections.
- `mutex.cpp`: Implementation of OS mutex initialization, locking, and unlocking.
- `semaphore.h`: Cross-platform thread synchronization counting semaphore header.
- `semaphore.cpp`: Implementation of OS semaphore posting and waiting.
- `spin_lock.h`: Ultra-low overhead atomic spinlock primitive header for short critical sections.
- `rw_lock.h`: Read/Write lock header allowing concurrent readers or single-writer memory synchronization.
- `main_loop.h`: Abstract base class header receiving frame iteration events from `Main::iteration()`.
- `main_loop.cpp`: Default empty implementation of `MainLoop` callbacks.

## 1.5 `core/math/`
- `vector2.h`: 2D floating-point vector class header containing (x, y) coordinates and geometric operations.
- `vector2.cpp`: Implementation of 2D vector math (dot product, cross product, normalization, rotation, angle calculations).
- `vector3.h`: 3D floating-point vector class header containing (x, y, z) coordinates and spatial operations.
- `vector3.cpp`: Implementation of 3D vector math (dot product, cross product, normalization, distance, linear interpolation).
- `vector4.h`: 4D floating-point vector class header containing (x, y, z, w) coordinates.
- `vector4.cpp`: Implementation of 4D vector math operations.
- `transform_2d.h`: 2x3 matrix class header representing 2D affine transformations (translation, rotation, scale).
- `transform_2d.cpp`: Implementation of 2D matrix multiplication, inversion, and vector transformations.
- `transform_3d.h`: 3x4 matrix class header representing 3D spatial transformations (`Basis` orientation + `Vector3` origin).
- `transform_3d.cpp`: Implementation of 3D matrix multiplication, affine inversion, interpolation, and spatial transformations.
- `basis.h`: 3x3 matrix class header representing 3D rotation, scaling, and shearing.
- `basis.cpp`: Implementation of 3x3 matrix determinant, transpose, inversion, scale extraction, and Euler angle conversion.
- `quaternion.h`: Unit quaternion class header representing 3D spherical rotations avoiding gimbal lock.
- `quaternion.cpp`: Implementation of quaternion multiplication, spherical linear interpolation (SLERP), and vector rotations.
- `projection.h`: 4x4 camera projection matrix class header for perspective and orthographic transformations.
- `projection.cpp`: Implementation of perspective frustum construction, orthographic bounds, matrix multiplication, and plane extraction.
- `aabb.h`: 3D Axis-Aligned Bounding Box class header for spatial bounds and frustum culling.
- `aabb.cpp`: Implementation of AABB merging, intersection checks, support point calculations, and volume checks.
- `rect2.h`: 2D Axis-Aligned Rectangle class header for 2D bounding boxes and GUI layouts.
- `rect2.cpp`: Implementation of Rect2 intersection, expansion, clipping, and point containment checks.
- `math_funcs.h`: Core mathematical utility function headers (trigonometric curves, interpolation, random generation, noise).
- `math_funcs.cpp`: Implementation of fast math routines, random number generators, Bezier curve evaluation, and snap functions.

## 1.6 `core/extension/`
- `gdextension.h`: Class header representing a loaded dynamic native extension shared object (`.dll`, `.so`, `.dylib`).
- `gdextension.cpp`: Implementation of extension library loading, symbol lookup, API version verification, and shutdown calls.
- `gdextension_manager.h`: Global registry header loading, initializing, and managing lifetime for GDExtension plugins.
- `gdextension_manager.cpp`: Implementation of extension scanning, dependency resolution, initialization level dispatching, and library unloading.
- `gdextension_interface.h`: Pure C ABI header file exposing Godot core APIs to third-party C/C++ native libraries.

## 1.7 `core/debugger/`, `core/string/`, `core/crypto/`, `core/templates/`, `core/input/`, `core/config/`
- `engine_debugger.h`: Remote TCP debugging client header routing profiling data, stack traces, and variable watch payloads to the editor.
- `engine_debugger.cpp`: Implementation of debugger message parsing, breakpoint registration, and profiler data transmission.
- `string_name.h`: Immutable string container header using global string table pointers for instant integer-comparison equality checks.
- `string_name.cpp`: Implementation of global string name table lock-free lookup, creation, and reference counting.
- `node_path.h`: Struct header holding relative or absolute scene graph node paths (`/root/Main/Player`).
- `node_path.cpp`: Implementation of node path string parsing, subname indexing (property paths), and path concatenation.
- `crypto.h`: Cryptographic utility class header generating RSA keys, HMAC signatures, AES encryption buffers, and SHA-256 hashes.
- `crypto.cpp`: Implementation of MbedTLS wrapper calls for hashing, key generation, and certificate verification.
- `vector.h`: Custom zero-allocation dynamic array container header replacing `std::vector`.
- `hash_map.h`: Custom high-performance open-addressing hash map template header replacing `std::unordered_map`.
- `input_event.h`: Base data structure header representing user input actions (keyboard, mouse, joystick, touch).
- `input_event.cpp`: Implementation of input event matching, action mapping, and string formatting routines.
- `engine.h`: Global engine singleton header tracking frame counts, target FPS, time scale, and engine state flags.
- `engine.cpp`: Implementation of singleton registration, performance metrics accessors, and engine time dilation controls.

---

# 2. `servers/` Low-Level Stateful Subsystem Files

## 2.1 `servers/rendering/`
- `rendering_server.h`: Main low-level graphics facade singleton header executing command buffers and managing RIDs.
- `rendering_server.cpp`: Implementation of `RenderingServer` facade methods, thread queue command dispatchers, and singleton accessors.
- `rendering_device.h`: Modern GPU driver abstraction API header (Vulkan, D3D12, Metal) executing command lists, uniform buffers, and compute passes.
- `rendering_device.cpp`: Implementation of `RenderingDevice` resource creation, descriptor set caching, pipeline compilation, and command buffer recording.
- `renderer_rd/renderer_scene_render_rd.h`: Modern rendering pipeline manager header executing Forward+, Mobile, Depth Prepass, SDFGI, and Post-Processing.
- `renderer_rd/renderer_scene_render_rd.cpp`: Implementation of pass graph setup, shadow map rendering, light clustering, and frame compositing.
- `renderer_rd/cluster_render_rd.h`: Compute shader light clustering manager header organizing Point/Spot lights into 3D frustum tile grids.
- `renderer_rd/cluster_render_rd.cpp`: Implementation of light cluster compute buffer generation and light assignment shaders.
- `renderer_rd/shader_compiler_rd.h`: Transpiles Godot Shader Language code into SPIR-V, DXIL, or Metal Shading Language bytecodes.
- `renderer_rd/shader_compiler_rd.cpp`: Implementation of shader AST parsing, code generation, and uniform reflection.

## 2.2 `servers/physics_3d/` & `servers/physics_2d/`
- `physics_server_3d.h`: Abstract server API header managing 3D rigid bodies, collision shapes, joints, and raycasts via RIDs.
- `physics_server_3d.cpp`: Singleton implementation and RID ownership initialization for 3D physics.
- `physics_server_2d.h`: Abstract server API header managing 2D rigid bodies, 2D collision shapes, and 2D joints via RIDs.
- `physics_server_2d.cpp`: Singleton implementation and RID ownership initialization for 2D physics.

## 2.3 `servers/display/`
- `display_server.h`: Abstract interface header managing OS windows, displays, mouse cursors, keyboard focus, and graphics context swapchains.
- `display_server.cpp`: Implementation of DisplayServer singleton registry and event dispatching.

## 2.4 `servers/audio/`
- `audio_server.h`: Low-latency audio mixing thread header managing Audio Bus routing, audio effect stacks, and output buffer streaming.
- `audio_server.cpp`: Implementation of audio buffer mixing loops, bus volume scaling, and DSP effect instance processing.
- `audio_stream.h`: Abstract resource class header generating raw PCM audio streams for audio players.
- `audio_stream.cpp`: Base implementation of audio stream playback creation and sample rate accessors.

## 2.5 `servers/text/` & `servers/navigation/`
- `text_server.h`: Text rendering abstraction header handling font rasterization, ICU line breaking, and HarfBuzz complex text shaping.
- `text_server.cpp`: Implementation of text server fallback chains, glyph caching, and font rendering logic.
- `navigation_server_3d.h`: Abstract server header managing 3D navigation meshes, pathfinding queries, and avoidance agent avoidance vectors.
- `navigation_server_3d.cpp`: Implementation of 3D navigation map management and pathfinding query dispatches.

## 2.6 `servers/camera/`, `servers/xr/`, `servers/movie_writer/`
- `camera_server.h` / `camera_server.cpp`: Server interface managing real-world hardware video capture camera feeds.
- `xr_server.h` / `xr_server.cpp`: Central XR server managing head trackers, AR/VR displays, and motion controllers.
- `movie_writer.h` / `movie_writer.cpp`: Movie writer server recording rendered engine frames directly to video files.

---

# 3. `scene/` Retained Scene Graph & Resource Files

## 3.1 `scene/main/`
- `node.h`: Foundation class header of the scene graph providing tree lifecycle notifications, hierarchy relations, process ticks, and groups.
- `node.cpp`: Implementation of child node adding/removing, notification propagation, group assignments, property overrides, and scene tree linking.
- `scene_tree.h`: `MainLoop` implementation header controlling scene tree execution, idle/physics iteration dispatching, and group calls.
- `scene_tree.cpp`: Implementation of scene tree frame iteration loops, tree change notifications, pause state management, and root node containers.
- `viewport.h`: Offscreen render target or window root container header handling 2D/3D camera assignment and input routing.
- `viewport.cpp`: Implementation of render target texture generation, sub-viewport bounds calculation, 2D/3D camera selection, and GUI event routing.
- `window.h`: Native or sub-window container header handling UI scaling, popups, and multi-window rendering.
- `window.cpp`: Implementation of sub-window creation, title bar controls, modal dialog behaviors, and window focus management.

## 3.2 `scene/3d/`
- `node_3d.h`: Base class header for spatial nodes containing a `Transform3D` transform matrix.
- `node_3d.cpp`: Implementation of 3D spatial transformations, local/global position getters/setters, rotation gizmos, and parent-child matrix multiplication.
- `mesh_instance_3d.h`: Binds `Mesh` resources to `RenderingServer` visual instance RIDs for 3D rasterization.
- `mesh_instance_3d.cpp`: Implementation of mesh RID assignment, surface material overrides, skinning skeleton assignment, and AABB updating.
- `camera_3d.h`: Spatial camera node header projecting 3D scenes onto a `Viewport` using perspective or orthographic matrices.
- `camera_3d.cpp`: Implementation of camera frustum calculation, view matrix updating, ray forecasting, and active camera registration.
- `light_3d.h`: 3D lighting nodes header updating `RenderingServer` light parameters (`OmniLight3D`, `SpotLight3D`, `DirectionalLight3D`).
- `light_3d.cpp`: Implementation of light color, energy, attenuation, shadow parameters, and server light RID updates.
- `lightmap_gi.h` / `lightmap_gi.cpp`: Node managing pre-baked ray-traced lightmaps and light probe volumes in 3D scenes.
- `cpu_particles_3d.h` / `cpu_particles_3d.cpp`: CPU-driven 3D particle emitter executing particle motion algorithms on the main thread.
- `gpu_particles_3d.h` / `gpu_particles_3d.cpp`: High-performance GPU-driven 3D particle emitter executing compute shaders.
- `decal.h` / `decal.cpp`: Projected texture decal node projecting decals onto 3D mesh surfaces.
- `fog_volume.h` / `fog_volume.cpp`: Volumetric fog volume node defining local density, color, and scattering parameters.
- `label_3d.h` / `label_3d.cpp`: Renders dynamic text labels within 3D spatial space.
- `skeleton_3d.h` / `skeleton_3d.cpp`: 3D bone hierarchy container managing bone transforms, pose overrides, and skinning matrices.
- `spring_bone_simulator_3d.h` / `spring_bone_simulator_3d.cpp`: Secondary physics spring bone chain simulator for dynamic hair and cloth movement.
- `sprite_3d.h` / `sprite_3d.cpp`: Renders 2D texture billboards inside 3D space.
- `voxel_gi.h` / `voxel_gi.cpp`: Realtime voxel global illumination probe volume node.
- `world_environment.h` / `world_environment.cpp`: Scene node binding `Environment` resources (sky, tone mapping, ambient light, post-processing) to viewports.

## 3.3 `scene/2d/`
- `canvas_item.h`: Abstract base class header for all 2D visual objects rendered on the 2D canvas.
- `canvas_item.cpp`: Implementation of 2D canvas drawing commands, visibility flags, canvas item RIDs, and z-index ordering.
- `node_2d.h`: Base class header for 2D spatial objects containing a `Transform2D` matrix.
- `node_2d.cpp`: Implementation of 2D position, rotation, scale, global transform calculations, and matrix inversions.
- `sprite_2d.h` / `sprite_2d.cpp`: Renders 2D texture images onto the canvas.
- `animated_sprite_2d.h` / `animated_sprite_2d.cpp`: Renders animated 2D sprite frame sequences using `SpriteFrames` resources.
- `tile_map.h` / `tile_map.cpp`: Grid-based 2D tilemap node managing multi-layer tile placement, autotiling, and collision shapes.
- `camera_2d.h` / `camera_2d.cpp`: 2D camera node controlling 2D viewport pan, zoom, limits, and smoothing.

## 3.4 `scene/gui/`
- `control.h`: Base class header for UI controls handling layout anchors, margins, focus, themes, and input events.
- `control.cpp`: Implementation of GUI layout positioning, minimum size calculation, theme lookup, focus movement, and drag-and-drop.
- `button.h` / `button.cpp`: Interactive GUI button widget firing press and toggle signals.
- `label.h` / `label.cpp`: Text UI widget displaying formatted strings.
- `line_edit.h` / `line_edit.cpp`: Single-line text input field control.
- `text_edit.h` / `text_edit.cpp`: Multi-line text input control with syntax highlighting and line numbers.
- `rich_text_label.h` / `rich_text_label.cpp`: Advanced BBCode formatted text display control supporting embedded images, animations, and tables.
- `tree.h` / `tree.cpp`: Hierarchical tree list widget displaying structured rows, icons, and collapsible items.
- `graph_edit.h` / `graph_edit.cpp`: Visual node graph editor workspace widget used for shader and animation tree editing.
- `color_picker.h` / `color_picker.cpp`: Interactive HSV/RGB color wheel and slider selection widget.

## 3.5 `scene/resources/`
- `mesh.h` / `mesh.cpp`: Base resource class defining 3D vertex arrays, index buffers, surface materials, and AABB bounds.
- `material.h` / `material.cpp`: Resource defining surface visual shading properties, textures, and shader uniform parameters.
- `shader.h` / `shader.cpp`: Resource containing Godot Shader Language code for custom visual or compute passes.
- `packed_scene.h` / `packed_scene.cpp`: Serialized scene resource template instantiating node trees into memory.
- `texture.h` / `texture.cpp`: Base class for 2D, 3D, and CubeMap texture resources.
- `font.h` / `font.cpp`: Font resource wrapper managing font size variations, kerning tables, and character glyph rendering.
- `environment.h` / `environment.cpp`: Resource containing sky configurations, ambient lighting, glow, SSAO, SSR, and tonemapping settings.

## 3.6 `scene/animation/` & `scene/audio/`
- `animation_player.h` / `animation_player.cpp`: Animation playback controller executing track interpolations across scene nodes.
- `animation_tree.h` / `animation_tree.cpp`: Complex animation blending controller processing state machines and blend graphs.
- `audio_stream_player.h` / `audio_stream_player.cpp`: Non-spatial 2D/stereo audio playback node.
- `audio_stream_player_3d.h` / `audio_stream_player_3d.cpp`: Spatial 3D sound source node routing audio buffers based on 3D listener distance.

---

# 4. `editor/` Godot Editor Suite Files

## 4.1 `editor/` Core Editor Entry Points
- `editor_node.h` / `editor_node.cpp`: Main application entry point for the Godot Editor; builds main editor windows, docks, and top menu bars.
- `editor_interface.h` / `editor_interface.cpp`: Public C++ and script API exposing editor state manipulation to `EditorPlugin` extensions.
- `editor_undo_redo_manager.h` / `editor_undo_redo_manager.cpp`: Handles editor history stacks, supporting per-scene and global undo/redo operations.
- `editor_data.h` / `editor_data.cpp`: Data container managing active editor plugins, edited scene histories, and clipboard data.
- `editor_log.h` / `editor_log.cpp`: Editor output log panel UI rendering engine warnings, errors, and print statements.

## 4.2 `editor/inspector/` & `editor/docks/`
- `editor_inspector.h` / `editor_inspector.cpp`: Generates dynamic UI property editors for nodes inspected in the editor.
- `file_system_dock.h` / `file_system_dock.cpp`: File browser dock UI allowing asset navigation, drag-and-drop, and file management.
- `scene_tree_dock.h` / `scene_tree_dock.cpp`: Hierarchy view dock UI allowing node creation, reordering, and scene manipulation.
- `inspector_dock.h` / `inspector_dock.cpp`: Right-hand dock container hosting the property inspector, node signals tab, and node groups tab.

## 4.3 `editor/file_system/`, `editor/import/`, `editor/plugins/`
- `editor_file_system.h` / `editor_file_system.cpp`: Background thread directory scanner tracking asset changes, UIDs, and triggering importers.
- `editor_import_plugin.h` / `editor_import_plugin.cpp`: Base class for custom asset importer plugins converting raw source files into binary runtime resources.
- `node_3d_editor_plugin.h` / `node_3d_editor_plugin.cpp`: 3D viewport editor workspace handling 3D gizmos, cameras, grid lines, and spatial transformations.
- `node_2d_editor_plugin.h` / `node_2d_editor_plugin.cpp`: 2D viewport editor workspace handling 2D canvas manipulation, snapping, and gizmos.

## 4.4 `editor/export/`, `editor/gui/`, `editor/debugger/`, `editor/settings/`, `editor/project_manager/`
- `editor_export_platform.h` / `.cpp`: Abstract base platform exporter defining build compilation, code signing, and packaging for target OSs.
- `editor_debugger_node.h` / `.cpp`: In-editor debugger panel hosting script stack traces, performance profilers, and remote tree views.
- `editor_settings.h` / `.cpp`: Persistent configuration singleton managing user editor preferences, hotkeys, themes, and external tool paths.
- `project_manager.h` / `.cpp`: Standalone executable UI launcher listing local projects, template downloads, and engine updates.

---

# 5. `main/` Executable Lifecycle Files

- `main.h`: Core engine bootup, setup, start, iteration, and cleanup method definitions.
- `main.cpp`: Orchestrates engine lifecycle initialization (`Main::setup()`), start (`Main::start()`), frame loop iteration (`Main::iteration()`), and shutdown (`Main::cleanup()`).
- `main_timer_sync.h` / `main_timer_sync.cpp`: Fixed timestep physics accumulator and frame rate delta smoothing logic.
- `performance.h` / `performance.cpp`: Singleton tracking runtime engine performance statistics (FPS, draw calls, memory, node counts).
- `steam_tracker.h` / `steam_tracker.cpp`: Optional integration interface tracking platform distribution runtime statistics.

---

# 6. `drivers/` Hardware Abstraction Backend Files

## 6.1 `drivers/vulkan/`, `drivers/gles3/`, `drivers/d3d12/`, `drivers/metal/`
- `drivers/vulkan/rendering_device_vulkan.h` / `.cpp`: Vulkan API graphics backend implementation for `RenderingDevice`.
- `drivers/vulkan/vulkan_context.h` / `.cpp`: Vulkan instance, physical device, surface, and queue creation manager.
- `drivers/gles3/rasterizer_gles3.h` / `.cpp`: OpenGL ES 3.0 compatibility driver backend implementation.
- `drivers/d3d12/rendering_device_d3d12.h` / `.cpp`: DirectX 12 graphics backend implementation for Windows / Xbox builds.
- `drivers/metal/rendering_device_metal.h` / `.cpp`: Apple Metal graphics backend implementation for macOS and iOS.

## 6.2 Audio & Hardware Input Drivers
- `drivers/wasapi/audio_driver_wasapi.h` / `.cpp`: Low-latency Windows WASAPI audio device output driver.
- `drivers/alsa/audio_driver_alsa.h` / `.cpp`: Linux ALSA audio device output driver.
- `drivers/coreaudio/audio_driver_coreaudio.h` / `.cpp`: macOS and iOS CoreAudio device driver.
- `drivers/pulseaudio/audio_driver_pulseaudio.h` / `.cpp`: Linux PulseAudio device output driver.
- `drivers/sdl/joypad_linux.cpp`: SDL game controller database mapping backend.

---

# 7. `platform/` Operating System Backend Files

## 7.1 `platform/windows/`, `platform/linuxbsd/`, `platform/macos/`
- `platform/windows/godot_windows.cpp`: Windows executable main entry point parsing UTF-16 CLI flags and initializing Win32 OS.
- `platform/windows/display_server_windows.h` / `.cpp`: Win32 native window management, message loops, mouse/keyboard inputs, and Vulkan/D3D12 context surfaces.
- `platform/windows/os_windows.h` / `.cpp`: Windows OS singleton handling system memory queries, file paths, clipboard, and subprocess execution.
- `platform/linuxbsd/godot_linuxbsd.cpp`: Linux executable entry point initializing POSIX signals and environment variables.
- `platform/linuxbsd/display_server_x11.h` / `.cpp`: X11 native window management, window hints, drag-and-drop, and input event parser.
- `platform/linuxbsd/display_server_wayland.h` / `.cpp`: Wayland native window management implementation.
- `platform/macos/godot_macos.mm`: macOS Cocoa executable main entry point.
- `platform/macos/display_server_macos.mm`: Cocoa NSWindow management and Metal layer rendering setup.

## 7.2 `platform/android/`, `platform/ios/`, `platform/web/`
- `platform/android/java_godot_wrapper.cpp`: Android JNI wrapper bridging Android Activity lifecycle events with Godot C++.
- `platform/android/display_server_android.h` / `.cpp`: Android SurfaceView rendering setup and touch event parser.
- `platform/ios/godot_ios.mm`: iOS UIKit executable main entry point.
- `platform/web/godot_web.cpp`: Emscripten HTML5 WebAssembly main loop binding browser RequestAnimationFrame to `Main::iteration()`.

---

# 8. `modules/` Opt-in Engine Modules Index

## 8.1 `modules/gdscript/`
- `gdscript.h` / `gdscript.cpp`: Script language implementation class for GDScript files (`.gd`).
- `gdscript_compiler.h` / `gdscript_compiler.cpp`: Compiles AST syntax nodes into stack-based bytecode arrays.
- `gdscript_parser.h` / `gdscript_parser.cpp`: Lexical analyzer and syntax parser generating AST trees from raw GDScript text.
- `gdscript_analyzer.h` / `gdscript_analyzer.cpp`: Type checker and semantic analyzer validating GDScript expression types.
- `gdscript_vm.h` / `gdscript_vm.cpp`: Virtual Machine opcode interpreter executing GDScript bytecode instructions.
- `gdscript_editor.h` / `gdscript_editor.cpp`: Integrates GDScript code completion, syntax highlighting, and inline documentation with the editor.

## 8.2 `modules/mono/` (C# .NET Host)
- `godotsharp.h` / `godotsharp.cpp`: C# language host binding Mono / CoreCLR runtime assemblies.
- `csharp_script.h` / `csharp_script.cpp`: Script language implementation class executing C# script instances.
- `mono_gd/clr_host.h` / `.cpp`: Native CoreCLR host initialization and assembly loader interface.

## 8.3 `modules/gltf/`
- `gltf_document.h` / `gltf_document.cpp`: Converts 3D GLTF/GLB files into Godot `Node3D`, `MeshInstance3D`, and `AnimationPlayer` nodes.
- `gltf_state.h` / `gltf_state.cpp`: Data state container storing JSON data, buffers, meshes, skins, and nodes during GLTF parsing.
- `gltf_node.h` / `gltf_node.cpp`: Intermediate node representation during GLTF scene tree conversion.

## 8.4 `modules/godot_physics_3d/` & `modules/jolt_physics/`
- `modules/godot_physics_3d/godot_physics_server_3d.h` / `.cpp`: Built-in 3D physics server implementing GJK, EPA, SAT collision detection and constraint solvers.
- `modules/godot_physics_3d/godot_body_3d.h` / `.cpp`: Internal 3D rigid body implementation handling force integration and velocity integration.
- `modules/jolt_physics/jolt_physics_server_3d.h` / `.cpp`: Multithreaded Jolt Physics engine server integration module.
- `modules/jolt_physics/jolt_body_3d.h` / `.cpp`: Jolt Physics rigid body wrapper.

## 8.5 Navigation, XR, CSG, Multi-player, and Utility Modules
- `modules/navigation_3d/navigation_mesh_generator.h` / `.cpp`: Recast Navigation mesh baker generating 3D walkable navigation meshes from geometry.
- `modules/openxr/openxr_api.h` / `.cpp`: OpenXR driver interfacing head-mounted displays, tracking controllers, and stereoscopic viewports.
- `modules/csg/csg_shape.h` / `.cpp`: Constructive Solid Geometry (CSG) nodes executing realtime 3D boolean operations (union, intersection, subtraction).
- `modules/multiplayer/multiplayer_api.h` / `.cpp`: High-level multiplayer replication server processing RPC calls and property synchronizations.
- `modules/gridmap/grid_map.h` / `.cpp`: 3D tile-based grid map placement node.
- `modules/tilemap/tile_map.h` / `.cpp`: 2D tilemap node supporting multi-layer rendering and physics collisions.
- `modules/lightmapper_rd/lightmapper_rd.h` / `.cpp`: Compute shader ray-traced lightmap baker module.
- `modules/websocket/websocket_peer.h` / `.cpp`: WebSocket client and server transport layer wrapper.
- `modules/webrtc/webrtc_peer_connection.h` / `.cpp`: WebRTC peer-to-peer real-time communication transport driver.
- `modules/mbedtls/stream_peer_mbedtls.h` / `.cpp`: SSL/TLS socket encryption wrapper using MbedTLS.
- `modules/zip/zip_packer.h` / `.cpp`: Resource packer generating ZIP archives.
