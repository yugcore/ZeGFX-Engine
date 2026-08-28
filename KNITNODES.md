# KnitNodes User Reference Manual

Welcome to the **KnitNodes Visual Scripting System** reference guide. KnitNodes is the high-performance visual scripting language embedded within the **ZeGFX / Velvet Engine**.

KnitNodes graphs compile directly down to a lightweight, register-based virtual machine bytecode, delivering near-C++ execution speed with zero runtime reflection overhead while retaining 100% API and workflow parity with GDScript.

---

## Table of Contents
1. [Visual Language & Pin Conventions](#1-visual-language--pin-conventions)
2. [Node Categories](#2-node-categories)
3. [Events & Engine Lifecycle Nodes](#3-events--engine-lifecycle-nodes)
4. [Flow Control Nodes](#4-flow-control-nodes)
5. [Variables & Memory Nodes](#5-variables--memory-nodes)
6. [Operators & Arithmetic Nodes](#6-operators--arithmetic-nodes)
7. [Math, Trigonometry & Geometry Nodes](#7-math-trigonometry--geometry-nodes)
8. [Interpolation, Curves & Easing Nodes](#8-interpolation-curves--easing-nodes)
9. [Random Number Generation Nodes](#9-random-number-generation-nodes)
10. [Containers & Strings Nodes](#10-containers--strings-nodes)
11. [Signals & Callables Nodes](#11-signals--callables-nodes)
12. [Types & Reflection Nodes](#12-types--reflection-nodes)
13. [Actions, Logging & Debugging Nodes](#13-actions-logging--debugging-nodes)
14. [Annotations & Inspector Decorators](#14-annotations--inspector-decorators)
15. [Practical Workflow Recipes](#15-practical-workflow-recipes)

---

## 1. Visual Language & Pin Conventions

Every node in KnitNodes communicates via **Pins** connected by **Wires**.

```
  +-------------------------------------------------------+
  |                   KnitNode Anatomy                    |
  |                                                       |
  |  [ > Flow In ]                                        |
  |  (● Data In A )           [ My Node ]     [ > Flow Out]
  |  (● Data In B )                           (● Result ) |
  +-------------------------------------------------------+
```

### Pin Kinds
- **Execution Pin (`>`)**: White chevron/arrow that defines the order in which actions and statements execute.
- **Data Pin (`●`)**: Colored circle representing typed data values passed between nodes.

### Data Pin Color Palette
| Data Type | Color | Description |
| :--- | :--- | :--- |
| **Execution Flow** | **White** (`#FFFFFF`) | Control flow sequence |
| **Bool** | **Coral Red** (`#F25959`) | `true` or `false` |
| **Int (Int32 / Int64)** | **Sea Green** (`#40D999`) | Integer numbers (`0, 1, -42`) |
| **Float / Double** | **Pale Cyan** (`#59CCFF`) | Fractional numbers (`3.14, -0.5`) |
| **String / StringName** | **Magenta** (`#E673E6`) | Text strings (`"Hello"`) |
| **Vector2 / Vector2i** | **Bright Gold** (`#F2CC59`) | 2D coordinates `(X, Y)` |
| **Vector3 / Vector3i** | **Yellow** (`#FFD940`) | 3D coordinates `(X, Y, Z)` |
| **Vector4 / Vector4i** | **Peach** (`#FFBF73`) | 4D coordinates `(X, Y, Z, W)` |
| **Color** | **Lavender** (`#F2B3F2`) | RGBA color channels |
| **Transform2D / 3D** | **Orange** (`#D98C4D`) | Matrix transform & position |
| **Basis / Quat / Plane** | **Terracotta** (`#CC8066`) | Rotations, bounding boxes |
| **NodePath** | **Sky Blue** (`#73BFD9`) | Scene tree path references |
| **RID** | **Pale Green** (`#99D999`) | Internal resource ID handle |
| **ObjectRef** | **Bright Blue** (`#59A6FF`) | Node / Entity instance pointer |
| **Callable** | **Cyan** (`#4DE6E6`) | First-class function handle |
| **Signal** | **Gold** (`#F2D933`) | Event broadcast emitter |
| **Array** | **Ochre** (`#D9A666`) | Ordered list `[A, B, C]` |
| **Dictionary** | **Turquoise** (`#66D9D9`) | Key-value table `{k: v}` |
| **Packed Arrays** | **Tan** (`#BF9959`) | High-performance packed buffers |
| **Wildcard (`T`)** | **Light Gray** (`#CCCCCC`) | Generic polymorphic data pin |

---

## 2. Node Categories

1. **`Event`**: Entry points triggered by the engine (e.g. `_ready`, `_process`, custom signals).
2. **`FlowControl`**: Directs execution branches, loops, and conditional pathways (`Branch`, `While`, `For`).
3. **`PureFunction`**: Deterministic math and query calculations with no side effects. Evaluated on-demand when pulled by execution nodes.
4. **`ImpureAction`**: Operations with side effects (moving characters, playing sounds, printing text).
5. **`VariableGet` / `VariableSet`**: Reads or writes script and local variables.
6. **`SubGraph`**: Encapsulates reusable graphs or `.knit_macro` assets into a single clean node.
7. **`Reroute`**: Zero-cost visual wire routing knots to keep graphs clean and readable.

---

## 3. Events & Engine Lifecycle Nodes

| KnitNode Name | Input Pins | Output Pins | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Ready Event`** | *None* | `FlowOut` | `func _ready():` | Fired when the node and all children enter the active scene tree. |
| **`Process Event`** | *None* | `FlowOut`, `delta (Float)` | `func _process(delta):` | Fired every rendered visual frame with elapsed frame time `delta`. |
| **`Physics Process Event`** | *None* | `FlowOut`, `delta (Float)` | `func _physics_process(delta):` | Fired on fixed physics simulation ticks (e.g. 60Hz). |
| **`Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _input(event):` | Intercepts raw unhandled viewport input events. |
| **`Unhandled Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _unhandled_input(event):` | Fallback input event processing when not consumed by UI. |
| **`GUI Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _gui_input(event):` | Control element mouse/touch/keyboard interaction. |
| **`Draw Event`** | *None* | `FlowOut` | `func _draw():` | CanvasItem 2D custom rendering hook. |
| **`Custom Signal Event`** | *None* | `FlowOut`, *Signal Args...* | `signal my_sig; func _on_my_sig():` | Executes when the designated signal is fired. |

---

## 4. Flow Control Nodes

| KnitNode Name | Input Pins | Output Pins | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Branch (If / Else)`** | `FlowIn`, `Condition (Bool)` | `True (Exec)`, `False (Exec)` | `if condition: ... else: ...` | Routes execution based on whether `Condition` is true or false. |
| **`Sequence`** | `FlowIn` | `Then 0 (Exec)`, `Then 1 (Exec)`, `...` | Sequential lines of code | Executes multiple execution flow branches sequentially on the same frame. Expandable with `+ Pin`. |
| **`Do Once`** | `In (Exec)`, `Reset (Exec)`, `Start Closed (Bool)` | `Out (Exec)` | Stateful single-trigger gate | Executes its `Out` flow branch only once until explicitly reset via the `Reset` pin. |
| **`Flip Flop`** | `In (Exec)` | `A (Exec)`, `B (Exec)`, `Is A (Bool)` | Alternating toggle | Alternates execution between outputs `A` and `B` on each incoming impulse, outputting current state. |
| **`Gate`** | `In (Exec)`, `Open`, `Close`, `Toggle`, `Start Closed` | `Out (Exec)`, `Is Open (Bool)` | Controllable gate | Pass-through gate controlled by discrete Open, Close, and Toggle inputs. |
| **`While Loop`** | `FlowIn`, `Condition (Bool)` | `LoopBody (Exec)`, `Completed (Exec)` | `while condition:` | Repeatedly runs `LoopBody` while `Condition` remains true. |
| **`For Each Loop`** | `FlowIn`, `Collection (Array)` | `LoopBody (Exec)`, `Element (T)`, `Completed (Exec)` | `for item in array:` | Iterates through each item in an array or container. |
| **`Delay (Seconds)`** | `FlowIn`, `Seconds (Float)` | `FlowOut (Exec)` | `await get_tree().create_timer(sec).timeout` | Suspends graph coroutine execution for the specified time duration. |
| **`Delay (Frames)`** | `FlowIn`, `Frames (Int)` | `FlowOut (Exec)` | `await get_tree().process_frame` | Suspends graph coroutine execution for N engine frame ticks. |
| **`Return`** | `FlowIn`, `Value (T)` | *Terminal* | `return value` | Exits the active graph or function with an optional return value. |

---

## 4.1 Structs & Vectors (Make / Break)

| KnitNode Name | Input Pins | Output Pins | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Make Vector2`** | `x (Float)`, `y (Float)` | `Vector2 (Vector2)` | `Vector2(x, y)` | Assembles a 2D vector from components. |
| **`Break Vector2`** | `Vector2 (Vector2)` | `x (Float)`, `y (Float)` | `v.x, v.y` | Deconstructs a 2D vector into components. |
| **`Make Vector3`** | `x (Float)`, `y (Float)`, `z (Float)` | `Vector3 (Vector3)` | `Vector3(x, y, z)` | Assembles a 3D vector from components. |
| **`Break Vector3`** | `Vector3 (Vector3)` | `x (Float)`, `y (Float)`, `z (Float)` | `v.x, v.y, v.z` | Deconstructs a 3D vector into components. |
| **`Make Vector4`** | `x`, `y`, `z`, `w` | `Vector4 (Vector4)` | `Vector4(x, y, z, w)` | Assembles a 4D vector from components. |
| **`Break Vector4`** | `Vector4 (Vector4)` | `x`, `y`, `z`, `w` | `v.x, v.y, v.z, v.w` | Deconstructs a 4D vector into components. |
| **`Make Color`** | `r`, `g`, `b`, `a` | `Color (Color)` | `Color(r, g, b, a)` | Assembles an RGBA color from components. |
| **`Break Color`** | `Color (Color)` | `r`, `g`, `b`, `a` | `c.r, c.g, c.b, c.a` | Deconstructs an RGBA color into components. |
| **`Make Rect2`** | `position (Vector2)`, `size (Vector2)` | `Rect2 (Rect2)` | `Rect2(pos, size)` | Assembles a 2D rectangle. |
| **`Break Rect2`** | `Rect2 (Rect2)` | `position (Vector2)`, `size (Vector2)` | `r.position, r.size` | Deconstructs a 2D rectangle. |
| **`Make Transform3D`** | `rotation (Vector3)`, `origin (Vector3)` | `Transform3D (Transform3D)` | `Transform3D(Basis.from_euler(r), o)` | Assembles a 3D coordinate transform. |
| **`Break Transform3D`** | `Transform3D (Transform3D)` | `origin`, `rotation`, `scale` | `t.origin, t.basis.get_euler(), t.basis.get_scale()` | Deconstructs a 3D coordinate transform into origin, rotation, and scale. |

---

## 5. Variables & Memory Nodes

| KnitNode Name | Input Pins | Output Pins | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Get Variable`** | *None* | `Value (T)` | `my_var` | Reads the current value of a script member or local variable. |
| **`Set Variable`** | `FlowIn`, `Value (T)` | `FlowOut`, `Value (T)` | `my_var = value` | Updates variable value and passes execution flow forward. |
| **`Constant Value`** | *Configured in inspector* | `Value (T)` | `const PI = 3.14` | Emits a constant literal without variable storage. |
| **`Get Self`** | *None* | `Self (ObjectRef)` | `self` | Returns reference to the current object instance executing the graph. |
| **`Get Node Shortcut`** | `Path (NodePath)` | `Node (ObjectRef)` | `$Path` / `get_node("Path")` | Resolves node in scene hierarchy relative to current node. |

---

## 6. Operators & Arithmetic Nodes

| KnitNode Name | Input Pins | Output Pins | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Add (+)`** | `a (T)`, `b (T)` | `Result (T)` | `a + b` | Adds numbers, vectors, concatenates strings, or merges arrays. |
| **`Subtract (-)`** | `a (T)`, `b (T)` | `Result (T)` | `a - b` | Subtracts `b` from `a`. |
| **`Multiply (*)`** | `a (T)`, `b (T)` | `Result (T)` | `a * b` | Multiplies numbers, scales vectors, or repeats strings. |
| **`Divide (/)`** | `a (T)`, `b (T)` | `Result (T)` | `a / b` | Divides `a` by `b` with automatic division-by-zero protection. |
| **`Modulo (%)`** | `a (T)`, `b (T)` | `Result (T)` | `a % b` | Computes integer or floating-point remainder. |
| **`Power (**)`** | `Base (Float)`, `Exp (Float)` | `Result (Float)` | `base ** exp` | Computes base raised to power exponent. |
| **`Negate (-)`** | `Value (T)` | `Result (T)` | `-value` | Flips sign of number or vector. |
| **`Bitwise AND (&)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a & b` | Performs bitwise AND. |
| **`Bitwise OR (|)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a | b` | Performs bitwise OR. |
| **`Bitwise XOR (^)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a ^ b` | Performs bitwise XOR. |
| **`Bitwise NOT (~)`** | `Value (Int)` | `Result (Int)` | `~value` | Inverts all bits. |
| **`Bit Shift Left (<<)`**| `Value (Int)`, `Shift (Int)` | `Result (Int)` | `val << shift` | Shifts bits left. |
| **`Bit Shift Right (>>)`**| `Value (Int)`, `Shift (Int)` | `Result (Int)` | `val >> shift` | Shifts bits right. |
| **`Equal (==)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a == b` | Returns `true` if values are equal. |
| **`Not Equal (!=)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a != b` | Returns `true` if values are not equal. |
| **`Less Than (<)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a < b` | Returns `true` if `a` is strictly less than `b`. |
| **`Less Equal (<=)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a <= b` | Returns `true` if `a <= b`. |
| **`Greater Than (>)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a > b` | Returns `true` if `a > b`. |
| **`Greater Equal (>=)`**| `a (T)`, `b (T)` | `Result (Bool)` | `a >= b` | Returns `true` if `a >= b`. |
| **`Logical NOT (!)`** | `Value (Bool)` | `Result (Bool)` | `not value` | Inverts boolean condition. |
| **`Logical AND (&&)`** | `a (Bool)`, `b (Bool)` | `Result (Bool)` | `a and b` | Returns `true` if both conditions are true. |
| **`Logical OR (||)`** | `a (Bool)`, `b (Bool)` | `Result (Bool)` | `a or b` | Returns `true` if either condition is true. |

---

## 7. Math, Trigonometry & Geometry Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Math Expression`** | *Dynamic Variables (e.g. `x`, `y`, `speed`)* | `Result (Float)` | `(x + y) * 2.0 - sin(z)` | Compiles an arbitrary inline mathematical formula directly into high-speed bytecode with automatic input pin generation. |
| **`Sin / Cos / Tan`** | `Angle (Float)` | `Result (Float)` | `sin(x)`, `cos(x)`, `tan(x)` | Trigonometric ratios in radians. |
| **`ASin / ACos / ATan`** | `Value (Float)` | `Angle (Float)` | `asin(x)`, `acos(x)`, `atan(x)` | Inverse trigonometric angles in radians. |
| **`ATan2`** | `Y (Float)`, `X (Float)` | `Angle (Float)` | `atan2(y, x)` | Four-quadrant arc tangent. |
| **`Deg To Rad`** | `Degrees (Float)` | `Radians (Float)` | `deg_to_rad(deg)` | Converts degrees to radians. |
| **`Rad To Deg`** | `Radians (Float)` | `Degrees (Float)` | `rad_to_deg(rad)` | Converts radians to degrees. |
| **`Sqrt`** | `Value (Float)` | `Result (Float)` | `sqrt(x)` | Square root. |
| **`Abs`** | `Value (T)` | `Result (T)` | `abs(x)` | Absolute magnitude. |
| **`Sign`** | `Value (T)` | `Result (T)` | `sign(x)` | `-1` for negative, `0` for zero, `+1` for positive. |
| **`Floor / Ceil / Round`**| `Value (Float)` | `Result (Float)` | `floor(x)`, `ceil(x)`, `round(x)` | Downward, upward, and nearest integer rounding. |
| **`Snap`** | `Value (T)`, `Step (T)` | `Result (T)` | `snapped(x, step)` | Snaps value to nearest grid step. |
| **`Clamp`** | `Value (T)`, `Min (T)`, `Max (T)` | `Result (T)` | `clamp(x, min, max)` | Clamps value within minimum and maximum limits. |
| **`Min / Max`** | `a (T)`, `b (T)` | `Result (T)` | `min(a, b)`, `max(a, b)` | Returns the lesser or greater of two values. |
| **`Wrap`** | `Value (T)`, `Min (T)`, `Max (T)` | `Result (T)` | `wrap(x, min, max)` | Wraps value around boundary limits. |
| **`Ping Pong`** | `Value (Float)`, `Length (Float)` | `Result (Float)` | `pingpong(val, len)` | Bounces value back and forth between 0 and length. |

---

## 8. Interpolation, Curves & Easing Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Lerp`** | `From (T)`, `To (T)`, `Weight (Float)` | `Result (T)` | `lerp(from, to, weight)` | Linear interpolation. `weight=0.0` returns `From`, `weight=1.0` returns `To`. |
| **`Lerp Angle`** | `From (Float)`, `To (Float)`, `Weight (Float)` | `Result (Float)` | `lerp_angle(from, to, w)` | Interpolates angles around the unit circle without 360° flip artifacts. |
| **`Inverse Lerp`** | `From (Float)`, `To (Float)`, `Value (Float)` | `Weight (Float)` | `inverse_lerp(from, to, val)` | Calculates interpolation weight `[0, 1]` for value between bounds. |
| **`Remap Range`** | `Val, IStart, IStop, OStart, OStop` | `Result (Float)` | `remap(val, is, ie, os, oe)` | Converts a value from an input range into an output range. |
| **`SmoothStep`** | `From (Float)`, `To (Float)`, `Value (Float)` | `Result (Float)` | `smoothstep(from, to, val)` | Hermite smoothstep interpolation curve with zero derivative endpoints. |
| **`Move Toward`** | `From (Float)`, `To (Float)`, `Delta (Float)` | `Result (Float)` | `move_toward(from, to, delta)` | Steps value toward target by a maximum constant increment `delta`. |
| **`Ease Curve`** | `Value (Float)`, `Curve (Float)` | `Result (Float)` | `ease(x, curve)` | Easing curve transformation function. |

---

## 9. Random Number Generation Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Random Int (randi)`** | *None* | `Value (Int)` | `randi()` | Generates a random unsigned 32-bit integer. |
| **`Random Float (randf)`** | *None* | `Value (Float)` | `randf()` | Generates a random normalized float in interval `[0.0, 1.0]`. |
| **`Random Int Range`** | `From (Int)`, `To (Int)` | `Value (Int)` | `randi_range(from, to)` | Generates a random integer in closed range `[from, to]`. |
| **`Random Float Range`** | `From (Float)`, `To (Float)` | `Value (Float)` | `randf_range(from, to)` | Generates a random float in range `[from, to]`. |
| **`Random Gaussian`** | `Mean (Float)`, `Deviation (Float)` | `Value (Float)` | `randfn(mean, dev)` | Normally distributed random float. |
| **`Randomize`** | `FlowIn` | `FlowOut` | `randomize()` | Re-seeds global random generator from OS hardware entropy. |

---

## 10. Containers & Strings Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Construct Array`** | `Elem 0..N (T)` | `Array (Array[T])` | `[a, b, c]` | Builds a dynamic or typed array from individual input values. |
| **`Array Append`** | `FlowIn`, `Array`, `Item (T)` | `FlowOut` | `array.append(item)` | Appends element to end of array. |
| **`Array Size (len)`** | `Container` | `Size (Int)` | `array.size()` / `len(c)` | Queries element count in array, string, or dictionary. |
| **`Construct Dictionary`**| `Key0, Val0, ...` | `Dict (Dictionary)` | `{k0: v0, ...}` | Constructs a key-value dictionary. |
| **`Dict Get`** | `Dict`, `Key`, `Default` | `Value (T)` | `dict.get(key, default)` | Retrieves value for key, falling back to default if missing. |
| **`Dict Set`** | `FlowIn`, `Dict`, `Key`, `Val` | `FlowOut` | `dict[key] = val` | Inserts or updates key-value pair in dictionary. |
| **`Dict Has`** | `Dict`, `Key` | `HasKey (Bool)` | `dict.has(key)` | Checks whether dictionary contains key. |
| **`String Format (str)`**| `Values 0..N` | `Result (String)` | `str(...)` | Stringifies and concatenates all input parameters. |
| **`Range Generator`** | `From (0)`, `To`, `Step (1)` | `Array (Array[Int])` | `range(from, to, step)` | Produces integer sequence array `[from..to]`. |
| **`Load Resource`** | `FlowIn`, `Path (String)` | `FlowOut`, `Resource` | `load(path)` | Dynamically loads resource asset from virtual file path. |

---

## 11. Signals & Callables Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Emit Signal`** | `FlowIn`, `Target`, `Signal Name`, *Args...* | `FlowOut` | `signal_name.emit(args)` | Broadcasts signal event to all connected listeners. |
| **`Connect Signal`** | `FlowIn`, `Target`, `Signal Name`, `Callable` | `FlowOut` | `signal_name.connect(callable)` | Subscribes callable method to signal. |
| **`Call Callable`** | `FlowIn`, `Callable`, *Args...* | `FlowOut`, `Result (T)` | `callable.call(args)` | Invokes first-class callable dynamically. |
| **`Await Signal`** | `FlowIn`, `Signal` | `FlowOut`, `Signal Args` | `await signal_name` | Yields coroutine until signal is emitted, capturing parameters. |

---

## 12. Types & Reflection Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Type Of`** | `Value (T)` | `Type ID (Int)` | `typeof(val)` | Returns integer Variant::Type code. |
| **`Is Instance Valid`** | `Object (ObjectRef)` | `Is Valid (Bool)` | `is_instance_valid(obj)` | Checks if object pointer is alive and not queued for deletion. |
| **`Type Test (Is)`** | `Value (T)`, `Type` | `Is Match (Bool)` | `value is MyClass` | Tests if instance matches class name or Variant type. |
| **`Type Cast (As)`** | `Value (T)`, `Target Type` | `Result (Target Type)` | `value as MyClass` | Safe type cast with null fallback if incompatible. |

---

## 13. Actions, Logging & Debugging Nodes

| KnitNode Name | Inputs | Outputs | GDScript Equivalent | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`Print`** | `FlowIn`, `Message (String)` | `FlowOut` | `print(message)` | Prints message to standard engine console. |
| **`Print Rich`** | `FlowIn`, `Message (String)` | `FlowOut` | `print_rich(msg)` | Prints BBCode-formatted colored text to editor console. |
| **`Print Error`** | `FlowIn`, `Message (String)` | `FlowOut` | `printerr(msg)` | Prints error highlighted text to stderr output. |
| **`Push Error`** | `FlowIn`, `Message (String)` | `FlowOut` | `push_error(msg)` | Registers error with stack trace in Engine Debugger. |
| **`Push Warning`** | `FlowIn`, `Message (String)` | `FlowOut` | `push_warning(msg)` | Registers warning in Engine Debugger. |
| **`Assert`** | `FlowIn`, `Condition (Bool)` | `FlowOut` | `assert(condition)` | Halts execution if condition is false in debug builds. |
| **`Move and Slide`** | `FlowIn` | `FlowOut` | `move_and_slide()` | Executes CharacterBody3D physics motion and collision response. |

---

## 14. Annotations & Inspector Decorators

In KnitNodes, script annotations are configured visually in the **Graph Inspector**:

| Annotation | Setting in KnitNodes Inspector | Purpose |
| :--- | :--- | :--- |
| **`@tool`** | `Is Tool Script = true` | Enables running graph execution inside the ZeGFX editor viewport. |
| **`@icon("path.svg")`** | `Script Icon = "res://icon.svg"` | Custom class icon in SceneTree and Creation Dialog. |
| **`@export`** | Variable setting `Exported = true` | Exposes variable to the engine Inspector. |
| **`@export_range(min, max, step)`**| Variable `Hint: Range` | Visual slider editor in Inspector with range limits. |
| **`@export_enum("A", "B", "C")`** | Variable `Hint: Enum` | Dropdown selection list in Inspector. |
| **`@export_file("*.png")`** | Variable `Hint: File` | File browser picker dialog in Inspector. |
| **`@onready`** | Variable `OnReady = true` | Auto-generates node lookup during `_ready` initialization. |
| **`@rpc(mode, sync, transfer)`** | Function `Network RPC Config` | Configures multiplayer network replication. |

---

## 15. Practical Workflow Recipes

### Recipe 1: 3D Character Movement & Gravity
A complete character controller loop executed every physics tick.

```
[ Physics Process Event ]
           |
           +---------------------> [ Get Velocity ]
                                         |
                                         v
   [ Get Delta ] ----> [ Mul (gravity * delta) ] ----> [ Sub (Velocity.y - Gravity) ]
                                                                 |
                                                                 v
                                                        [ Set Velocity ]
                                                                 |
                                                                 v
                                                       [ Move and Slide ]
```

### Recipe 2: Timed Attack Cooldown Coroutine
```
[ Input Event (Attack Action) ]
           |
           v
   [ Branch (Can Attack?) ]
           |
     (True)|
           v
   [ Set Can Attack = False ]
           |
           v
   [ Emit Signal "attack_performed" ]
           |
           v
   [ Delay (1.5 Seconds) ]
           |
           v
   [ Set Can Attack = True ]
```

### Recipe 3: Smooth Camera Follow with Lerp
```
[ Process Event ]
       |
       v
[ Get Camera Position ] ---> [ Lerp ] <--- [ Get Target Position ]
                                ^
[ Delta ] ---> [ Mul (speed) ] -+
                                |
                                v
                      [ Set Camera Position ]
```
