# ZELYN FOR ENGINE (ZELYNFORENGINE)
## Architectural Blueprint & Technical Integration Plan

This document outlines the comparative analysis, current engine GDScript implementation review, and complete technical blueprint for making **Zelyn** the **primary scripting language** of the **ZeGFX-Engine**, while maintaining the **core Zelyn runtime as a standalone, general-purpose scripting language**.

---

## 1. Executive Summary & Vision

### 1.1 Goals & Objectives
1. **Primary Scripting Engine**: Transition Zelyn (`.zl`) to be the default, primary scripting language across the engine (editor, node components, scene instantiation, and build target execution).
2. **GDScript Relegation**: Demote GDScript to a secondary, legacy-compatible scripting language layer. GDScript scripts will still be executable and interoperable with Zelyn, but Zelyn will be the default script choice for new scenes, nodes, and templates.
3. **Core Decoupling**: Maintain `Zelyn/` as a pure, standalone C++17 general scripting language (register VM, single-pass zero-AST compiler, standard library, CLI, and C-ABI embedding layer) with zero dependencies on engine code.
4. **Engine Specialization Layer (`modules/zelyn`)**: Create a specialized engine module (`ZelynForEngine` / `EZelyn`) inside `modules/zelyn` that implements the engine's `ScriptLanguage`, `Script`, and `ScriptInstance` interfaces, adapting Zelyn's 24-byte `Value` variant and register VM to ZeGFX Engine's `Variant`, `Object`, and `ClassDB` reflection system.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ZeGFX Engine (Editor / Game Runtime)              │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
            ┌──────────────────────────┴──────────────────────────┐
            ▼                                                     ▼
┌──────────────────────────────────────┐               ┌──────────────────────┐
│       PRIMARY SCRIPT LANGUAGE        │               │ SECONDARY SCRIPT LANG│
│     Zelyn Module (modules/zelyn)     │               │   GDScript Module    │
│ (ZelynScriptLanguage, ZelynInstance) │               │   (modules/gdscript) │
└───────────────────┬──────────────────┘               └──────────────────────┘
                    │ (Embeds via API)
                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Standalone Core Zelyn Engine                            │
│                        (z:\ZeGFX-Engine\Zelyn)                              │
│    • Register Bytecode VM (BytecodeVM)   • Zero-AST Compiler (Compiler)     │
│    • 24-Byte Trivial Value System        • Inline Dictionary Caching        │
│    • Intrusive GC & UserDataObject       • C-ABI Bridge (zelyn_c_api.h)     │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. In-Depth Technical Comparison: GDScript vs. Zelyn

| Feature / Dimension | GDScript (`modules/gdscript`) | Zelyn (`Zelyn/`) |
| :--- | :--- | :--- |
| **Language Paradigm** | Python-like, whitespace/indentation-sensitive, class/inheritance-based OOP. | C/Rust-like procedural data-first syntax with braces `{}` and explicit `;` semicolons. |
| **Execution Entry Point** | File IS a class. Top-level implicit class statements or explicit `extends` / `class_name`. | Functions declared at top-level + explicit `main { ... }` block (or `on_ready()` / `on_process()` lifecycle callbacks in engine mode). |
| **Virtual Machine Architecture** | Stack/register hybrid VM (`GDScriptFunction::call`) operating on `Variant` instructions. Multi-pass AST compilation. | Flat 4096-register file VM (`BytecodeVM`) with 32-bit opcodes, single-pass zero-AST compiler, and direct indexed loading (`reg(r)`). |
| **Value Representation** | Engine `Variant` (20-byte payload handling 25+ types including `Vector2`, `Vector3`, `Transform3D`, `Object*`, `RefCounted*`). | 24-byte `Value` struct (`Nil`, `Bool`, `Number`/Float64, `Symbol`/Interned ID, `Object*`). Engine types wrapped in `UserDataObject`. |
| **Dictionary & Member Lookup** | Dynamic hash map probes on string names during runtime execution. | Inline Dictionary Caching (`CacheEntry` + bytecode instruction patching) converting string dict lookups into direct index offset access. |
| **Memory & Object Management** | RefCounted pointers + Engine `Object` ID tracking. | Dual-mode memory model: Trivial 24-byte value copies for primitives; Intrusive mark-sweep GC & reference counting for heap objects (`ListObject`, `DictObject`, `UserDataObject`). |
| **Coroutines & Asynchronous Logic** | `await` keyword yielding execution via `GDScriptFunctionState`. | Native `CoroutineObject` primitives, yield states, and frame-tick yielding via `wait_seconds()`. |
| **FFI & C++ Host Interop** | Tied to engine `ClassDB`, `MethodBind`, and `Variant::callp`. | `NativeRegistry`, `NativeTable`, fast-native direct function pointers, C-ABI bridge (`zelyn_c_api.h`), and zero-string K2Node autodiscovery. |
| **Compilation Speed** | Multi-pass parsing, AST construction, analyzer pass, and bytecode codegen. | Ultra-fast single-pass zero-AST tokenizer-to-bytecode compiler (`Compiler::compile`). |
| **Engine Dependencies** | Heavy hard-coded engine dependencies (`core/object/`, `core/variant/`, `core/config/`). | Zero engine dependencies in core `Zelyn/`. Embedded via lightweight C/C++ header interface. |

---

## 3. Analysis of GDScript Implementation in ZeGFX-Engine

In ZeGFX Engine (derived from Godot engine architecture), scripting languages implement three core abstract base classes defined in `core/object/script_language.h`: `ScriptLanguage`, `Script`, and `ScriptInstance`.

### 3.1 Module Components (`modules/gdscript`)
1. **`GDScriptLanguage`** (`modules/gdscript/gdscript.h`):
   - Global singleton implementing `ScriptLanguage`.
   - Manages language lifecycle (`init()`, `finish()`), script reload queues, thread synchronization (`languages_mutex`), debug targets, language server (LSP), and documentation generators.
   - Registers extension `.gd`.

2. **`GDScript`** (`modules/gdscript/gdscript.h`):
   - Resource subclass (`public Script`) representing a compiled script asset.
   - Holds script member functions (`HashMap<StringName, GDScriptFunction*>`), constants, member property definitions (`MemberInfo`), subclass references, and signal signatures.
   - Instantiates script instances via `_create_instance()`.

3. **`GDScriptInstance`** (`modules/gdscript/gdscript.cpp`):
   - Implements `ScriptInstance`. Attached directly to an engine `Object*` instance (e.g. `Node2D`, `CharacterBody3D`).
   - Owns a array of `Variant` variables corresponding to instance member fields.
   - Implements `callp()`, `set()`, `get()`, and forwards engine notifications (`NOTIFICATION_READY`, `NOTIFICATION_PROCESS`) to script methods (`_ready`, `_process`).

4. **Compiler Pipeline**:
   - `GDScriptTokenizer` -> `GDScriptParser` (builds AST) -> `GDScriptAnalyzer` (type checks) -> `GDScriptCompiler` -> `GDScriptFunction` (bytecode array + constant table).

5. **`ScriptServer` Registration**:
   - `ScriptServer::register_language(&GDScriptLanguage)` registers GDScript into the engine's global language registry array (`ScriptServer::_languages[MAX_LANGUAGES]`).

---

## 4. Architecture Blueprint for Zelyn Specialization (`ZelynForEngine`)

To make Zelyn the primary scripting language while preserving standalone core `Zelyn/`, we establish a strict **two-tier module separation**:

```
z:\ZeGFX-Engine/
├── Zelyn/                             <-- TIER 1: Standalone Core Zelyn Runtime
│   ├── include/zelyn/                 • Pure C++17 (No engine includes)
│   │   ├── common.h (Value, HeapObject)
│   │   ├── bytecode_vm.h (BytecodeVM)
│   │   ├── compiler.h (Compiler)
│   │   └── zelyn_c_api.h
│   └── src/core/ (bytecode_vm.cpp, compiler.cpp, executor.cpp)
│
└── modules/zelyn/                     <-- TIER 2: Engine Specialization Layer
    ├── SCsub & config.py               • Adapts Zelyn for ZeGFX-Engine
    ├── register_types.h/cpp            • Registers Zelyn as PRIMARY language
    ├── zelyn_script_language.h/cpp     • Implements ScriptLanguage for .zl
    ├── zelyn_script.h/cpp              • Implements Script resource for .zl
    ├── zelyn_script_instance.h/cpp     • Implements ScriptInstance for Object*
    ├── zelyn_variant_bridge.h/cpp      • Value <-> Variant marshalling & UserDataObject
    └── zelyn_classdb_binding.h/cpp     • Exposes ClassDB & engine math to Zelyn VM
```

---

### 4.1 Type System & Marshalling Bridge (`zelyn_variant_bridge.h`)

Core Zelyn represents values using the 24-byte `Value` struct. ZeGFX Engine uses `Variant`. We bridge them via custom `UserDataObject` handles and fast conversion routines:

#### Type Mapping Matrix

| Zelyn `Value` Type | ZeGFX Engine `Variant` Type | Conversion Mechanism |
| :--- | :--- | :--- |
| `ValueType::Nil` | `Variant::NIL` | Direct 1:1 mapping. |
| `ValueType::Bool` | `Variant::BOOL` | Direct boolean flag conversion. |
| `ValueType::Number` | `Variant::FLOAT` / `Variant::INT` | Cast double <-> float/int64_t. |
| `ValueType::Symbol` | `Variant::STRING` / `Variant::STRING_NAME` | `SymbolTable::getString()` <-> `StringName`. |
| `HeapObject::Kind::String` | `Variant::STRING` | `StringObject::data` <-> `String`. |
| `HeapObject::Kind::List` | `Variant::ARRAY` | Deep copy / proxy wrapper around `Array`. |
| `HeapObject::Kind::Dict` | `Variant::DICTIONARY` | Symbol-keyed `DictObject` <-> `Dictionary`. |
| `HeapObject::Kind::UserData` | Engine `Object*` / `RefCounted*` | Wrapped in `ZelynEngineUserData` with `ObjectID` validation. |
| `HeapObject::Kind::UserData` | `Vector2`, `Vector3`, `Color`, etc. | Stack-allocated or pooled value struct payload inside `UserDataObject`. |

#### Safe Object Wrapper Layout (`ZelynEngineUserData`) & Stale-Pointer Invalidation

```cpp
// modules/zelyn/zelyn_variant_bridge.h
#include "zelyn/common.h"
#include "core/object/object.h"
#include "core/object/object_id.h"
#include "core/variant/variant.h"

struct ZelynEngineUserData {
    ObjectID object_id;           // Primary safe engine object lookup (prevents stale pointers)
    Object*  raw_ptr = nullptr;   // Transient cached pointer for frame-local micro-optimizations
    bool     is_ref_counted = false;
    Variant  variant_payload;     // For math types (Vector2, Vector3, Transform3D, Color)

    // Airtight validation method
    _FORCE_INLINE_ Object* get_valid_object() {
        if (object_id.is_null()) {
            raw_ptr = nullptr;
            return nullptr;
        }
        // Query Engine ObjectDB to verify object hasn't been freed/deleted
        Object* live_obj = ObjectDB::get_instance(object_id);
        if (unlikely(live_obj != raw_ptr)) {
            raw_ptr = live_obj; // Synchronize or null out if deleted
        }
        return live_obj;
    }
};

// Functions for conversion
Value variant_to_zelyn(const Variant& p_variant);
Variant zelyn_to_variant(const Value& p_value);
```

##### Explicit Invalidation Trigger Architecture
To guarantee zero use-after-free or dangling pointer bugs (solving the classic `ObjectID` indirection challenge):
1. **Validation Boundary**: `raw_ptr` is NEVER dereferenced directly in Zelyn VM calls without invoking `get_valid_object()`, which validates `object_id` against the engine's central `ObjectDB`.
2. **Predelete Signal Hook (`NOTIFICATION_PREDELETE`)**: When an engine `Object` is queued for deletion via `memdelete()`, the engine dispatches `NOTIFICATION_PREDELETE`. `ZelynScriptInstance` hooks this notification to immediately zero out `raw_ptr = nullptr` and unbind all active `UserDataObject` handles associated with that `ObjectID`.
3. **Safe Script Fallback**: If a script attempts to invoke a method or access a field on a deleted object (`get_valid_object() == nullptr`), the VM catches the invalid handle, returns `Value::nil()`, and emits a non-crashing runtime error (`"Attempt to call method on a deleted Object instance"`), ensuring complete memory safety.

---

### 4.2 Engine Script Infrastructure Implementation (`modules/zelyn`)

#### 1. `ZelynScriptLanguage` (inherits `ScriptLanguage`)
- **Primary Registration**: Registers extension `.zl` (and `.zelyn`).
- **File Watcher & Live Hot Reload**: Integrates with engine directory watcher to trigger VM re-evaluations under 50ms upon file save.
- **Reserved Keywords**: `func`, `fn`, `main`, `var`, `let`, `return`, `if`, `else`, `while`, `for`, `in`, `wait_seconds`, `out`.
- **Primary Language Priority**: Returns high preference ranking so editor defaults new script dialogs to Zelyn.

#### 2. `ZelynScript` (inherits `Script`)
- **Bytecode Storage**: Wraps a compiled `ZChunk` from Zelyn's `Compiler`.
- **Lifecycle Inspection**: Parses top-level Zelyn functions and maps standard engine lifecycle callbacks:
  - `on_ready()` -> Called on `NOTIFICATION_READY`.
  - `on_process(dt)` -> Called on `NOTIFICATION_PROCESS`.
  - `on_physics_process(dt)` -> Called on `NOTIFICATION_INTERNAL_PHYSICS_PROCESS`.
  - `on_input(event)` -> Called on `NOTIFICATION_INPUT`.
- **Property Export**: Exposes script-level variables to the Engine Inspector window using `PropertyInfo`.

#### 3. `ZelynScriptInstance` (inherits `ScriptInstance`) & Register Memory Architecture
- Attaches to an engine `Object*` owner.
- **Register Memory Allocation & Call-Frame Pooling**:
  - **No Per-Instance 4096-Register File**: A persistent 4096-register file per script instance would consume ~98 KB per instance (causing ~980 MB overhead for 10,000 active script nodes).
  - **Instance State**: `ZelynScriptInstance` ONLY stores persistent script member properties (`Vector<Value> member_values`), requiring minimal memory ($O(M)$ where $M$ is member count, e.g. ~120 bytes per instance).
  - **Shared Thread-Local VM Register Stack**: Execution is driven by a shared thread-local `BytecodeVM` stack. When `callp` or lifecycle methods (`on_process`) execute, the script instance borrows a **transient call-frame window** (`CallFrame`) on the thread's shared VM register stack.
  - Upon function completion, the frame is popped and the register slice is recycled back to the thread pool immediately, keeping idle scene memory virtually identical to GDScript.
- **Dispatch Loop**: `callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error)`:
  1. Converts incoming `Variant` arguments to Zelyn `Value`s using `variant_to_zelyn`.
  2. Leases a transient call frame on the thread-local `BytecodeVM`.
  3. Executes target `ZChunk` / user function on the VM.
  4. Converts return `Value` back to engine `Variant` via `zelyn_to_variant`.

```cpp
// Example Zelyn Engine Script: res://scripts/player.zl

func on_ready(self) {
    out("Player initialized via Zelyn primary script engine!");
}

func on_process(self, dt) {
    let pos = self.get_position();
    pos.x += 100.0 * dt;
    self.set_position(pos);
}
```

---

### 4.3 `ClassDB` & Engine API Auto-Binding

To give Zelyn scripts full access to engine C++ classes (`Node2D`, `Sprite2D`, `RigidBody3D`, `AudioStreamPlayer`, etc.):

1. **Fast-Native Dispatch Bridge**: Use Zelyn's `NativeRegistry` to bind engine `ClassDB` method resolution.
2. **Method Binding Dispatch**: When a script calls `self.set_position(pos)`:
   - Zelyn's VM checks `UserDataObject` type.
   - Finds method `set_position` via `ClassDB::get_method(class_name, method_name)`.
   - Invokes `MethodBind::call()` directly with converted arguments.
3. **Engine Math Acceleration**: Register `Vector2`, `Vector3`, `Transform3D`, and `Color` directly into Zelyn's VM register operations (`ZOP_ADD`, `ZOP_MUL`) for zero-allocation math performance.

---

### 4.4 Primary vs. Secondary Language Strategy

To make Zelyn the primary language and push GDScript to secondary status:

1. **Registration Order in `register_types.cpp`**:
   In `modules/register_module_types.gen.cpp`, register `ZelynScriptLanguage` FIRST before `GDScriptLanguage`.
   ```cpp
   // ScriptServer registration order
   ScriptServer::register_language(&ZelynScriptLanguage::get_singleton()); // Primary (Index 0)
   ScriptServer::register_language(&GDScriptLanguage::get_singleton());    // Secondary (Index 1)
   ```

2. **Editor Default Script Creation**:
   Update `editor/script_create_dialog.cpp` so that when a user clicks "Attach Script" on any node, **Zelyn (.zl)** is selected by default.

3. **Cross-Language Interoperability**:
   Because both Zelyn and GDScript implement the standard `ScriptInstance` interface, Zelyn scripts can call GDScript methods seamlessly via `object.call("gdscript_func")` and vice versa.

---

### 4.5 Bytecode Versioning & Binary Cache (.zlc) Compatibility

To prevent crashes or corrupted memory when engine updates alter bytecode opcode numbers, instruction layouts, or constant table schemas, Zelyn introduces an explicit **Bytecode Cache Header Spec** for `.zlc` binary files:

#### `.zlc` Binary File Header Layout

```cpp
struct ZelynBytecodeHeader {
    char     magic[4] = {'Z', 'L', 'C', '1'}; // "ZLC1" magic identifier
    uint32_t bytecode_version;                // Incremented on any opcode/IR schema change
    uint32_t engine_build_hash;               // Hash of engine version string & commit SHA
    uint32_t compiler_flags;                  // Debug/Optimization flags used during compilation
    uint64_t source_modified_time;            // Source .zl file timestamp
};
```

#### Automatic Invalidation & Re-Compilation Trigger
1. **Header Inspection**: When `ResourceFormatLoaderZelyn` loads a cached `.zlc` binary script, it parses `ZelynBytecodeHeader`.
2. **Version Check**: If `magic != "ZLC1"` OR `bytecode_version != CURRENT_ZELYN_BYTECODE_VERSION` OR `engine_build_hash != CURRENT_ENGINE_HASH`:
   - The loader flags the `.zlc` cache as **invalid/stale**.
3. **Transparent Re-compilation**: `ResourceFormatLoaderZelyn` silently invokes `Compiler::compile()` to re-generate a fresh `ZChunk` from the `.zl` source file, overwrites the invalid `.zlc` cache file on disk, and loads the fresh bytecode seamlessly without interrupting engine startup or scene loading.

---

## 5. Step-by-Step Implementation & Migration Roadmap

### Phase 1: Core Zelyn Audit & C-Bridge Hardening (Location: `Zelyn/`)
- Ensure `Zelyn/` builds cleanly as a standalone static/shared library `libzelyn`.
- Validate that `Zelyn/include/zelyn/zelyn_c_api.h` and `Zelyn/include/zelyn/common.h` provide complete access to `BytecodeVM`, `Compiler`, `Value`, `ZChunk`, and `UserDataObject`.

### Phase 2: Engine Module Skeleton Creation (Location: `modules/zelyn/`)
- Create directory `modules/zelyn/`.
- Add `config.py` and `SCsub` build scripts linking against `Zelyn/`.
- Implement `register_types.h/cpp` to hook into engine module initialization.

### Phase 3: Type Adapter & Marshalling Layer (`zelyn_variant_bridge`)
- Implement `variant_to_zelyn()` and `zelyn_to_variant()`.
- Implement `ZelynEngineUserData` wrapper around `ObjectID` for engine objects.
- Create unit tests verifying zero memory leaks during object passing.

### Phase 4: Core Engine Script Classes (`ScriptLanguage`, `Script`, `ScriptInstance`)
- Implement `ZelynScriptLanguage` with `.zl` file extension support.
- Implement `ZelynScript` for compiling and holding bytecode chunks.
- Implement `ZelynScriptInstance` for executing script logic on attached nodes.

### Phase 5: Engine Lifecycle Integration & ClassDB Binding
- Bind engine notifications (`NOTIFICATION_READY`, `NOTIFICATION_PROCESS`, `NOTIFICATION_PHYSICS_PROCESS`) to Zelyn callbacks (`on_ready`, `on_process`, `on_physics_process`).
- Integrate `ClassDB` lookup for dynamic script invocation of engine methods.

### Phase 6: Primary Language Elevation & Editor Integration
- Change `ScriptServer` language registration order to register Zelyn as Index 0.
- Update Editor Node script creation templates to default to Zelyn (`.zl`).
- Relegate GDScript to secondary language option in drop-down menus.

### Phase 7: Hot-Reloading, Secondary GDScript Fallback & Cross-Language Interop Matrix
- Implement directory watching for live script hot-reloading during Play-In-Editor (PIE) in < 50ms.
- **Explicit Cross-Language Interoperability Matrix & Verification Plan**:
  To guarantee zero regression in GDScript secondary fallback mode and seamless multi-language scene composition, execute the following interop verification suite:

| Interop Test Case | Invocation Path | Expected Behavior & Verification |
| :--- | :--- | :--- |
| **Zelyn calling GDScript Method** | `zelyn_node.call("gd_method", arg1)` | Dispatches via engine `ScriptInstance::callp()`. Variant arguments marshalled from `Value`, returns GDScript result converted to `Value`. |
| **GDScript calling Zelyn Method** | `gdscript_node.call("zl_method", arg1)` | Dispatches via `ZelynScriptInstance::callp()`. Variant arguments marshalled to `Value`, returns Zelyn `Value` converted to `Variant`. |
| **Signal Crossing (Zelyn -> GDScript)** | `zl_node.emit_signal("health_changed", 80)` | GDScript receiver connected via `connect("health_changed", gd_node, "on_health_changed")` triggers correctly with expected argument values. |
| **Signal Crossing (GDScript -> Zelyn)** | `gd_node.emit_signal("player_spawned", player)` | Zelyn listener function connected via `connect("player_spawned", zl_node, "on_player_spawned")` receives `ZelynEngineUserData` wrapped player reference. |
| **Cross-Language Property Access** | `gd_node.set("speed", 250.0)` / `zl_node.get("score")` | Inspector and script `get()` / `set()` calls dynamically read/write exported properties across language boundary via `ScriptInstance`. |
| **Scene Tree Composition** | Zelyn Root Node containing GDScript Children | Parent Zelyn node iterates over GDScript children (`get_children()`) and calls methods/properties without type errors or engine crashes. |

---

## 6. Conclusion

By separating **Core Zelyn** (`Zelyn/`) from the **Engine Specialization Layer** (`modules/zelyn/`), ZeGFX Engine gains a ultra-fast, register-based, modern procedural scripting language as its primary driver, while preserving GDScript for secondary compatibility and maintaining Zelyn as a clean standalone general-purpose scripting language.
