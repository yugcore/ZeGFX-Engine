# KnitNodes Complete User Reference Manual

Welcome to the **KnitNodes Visual Scripting System** comprehensive reference manual. KnitNodes is the next-generation, high-performance visual scripting system embedded natively in the **ZeGFX / Velvet Engine**.

KnitNodes graphs compile directly into a lightweight, register-based virtual machine bytecode. This delivers near-C++ execution speed with zero runtime reflection overhead, while offering 100% API parity with GDScript and ClassDB.

---

## Table of Contents
1. [Visual Language & Pin Conventions](#1-visual-language--pin-conventions)
2. [Node Categories & Color Palette](#2-node-categories--color-palette)
3. [Events & Engine Lifecycle Nodes](#3-events--engine-lifecycle-nodes)
4. [Flow Control & Gate Nodes](#4-flow-control--gate-nodes)
5. [Gameplay & Physics Power Nodes](#5-gameplay--physics-power-nodes)
6. [Workflow & Productivity Nodes (Reroutes & Formulas)](#6-workflow--productivity-nodes)
7. [Struct Construction & Deconstruction (Make / Break)](#7-struct-construction--deconstruction-make--break)
8. [Variables & Memory Nodes](#8-variables--memory-nodes)
9. [Operators & Arithmetic Nodes](#9-operators--arithmetic-nodes)
10. [Math, Trigonometry & Geometry Nodes](#10-math-trigonometry--geometry-nodes)
11. [Interpolation, Curves & Easing Nodes](#11-interpolation-curves--easing-nodes)
12. [Random Number Generation Nodes](#12-random-number-generation-nodes)
13. [Containers & Strings Nodes](#13-containers--strings-nodes)
14. [Signals & Callables Nodes](#14-signals--callables-nodes)
15. [Types & Dynamic ClassDB Reflection Nodes](#15-types--dynamic-classdb-reflection-nodes)
16. [Actions, Logging & Debugging Nodes](#16-actions-logging--debugging-nodes)
17. [1-Click Bidirectional GDScript $\leftrightarrow$ KnitNodes Transpiler](#17-1-click-bidirectional-gdscript--knitnodes-transpiler)
18. [Annotations & Inspector Decorators](#18-annotations--inspector-decorators)
19. [Production Recipes & Architectures](#19-production-recipes--architectures)

---

## 1. Visual Language & Pin Conventions

Every node in KnitNodes communicates via **Pins** connected by **Wires**.

```
  +-------------------------------------------------------------------------+
  |                           KnitNode Anatomy                              |
  |                                                                         |
  |  [ > Flow In ]                                             [ > Flow Out ]
  |  (● Target Object )       [ Character Move & Jump 3D ]     (● Velocity  )
  |  (● Input Dir (Vec2) )                                     (● On Floor  )
  |  (● Jump (Bool) )                                                       |
  +-------------------------------------------------------------------------+
```

### Pin Kinds
* **Execution Pin (`>`)**: White chevron/arrow that specifies the sequential order of execution.
* **Data Pin (`●`)**: Color-coded circle that passes typed data values between nodes. Data pins support inline default editing widgets when unconnected.

### Data Pin Color Palette
| Data Type | Color | Hex Code | Description |
| :--- | :--- | :--- | :--- |
| **Execution Flow** | **Pure White** | `#FFFFFF` | Sequence impulse wire |
| **Bool** | **Coral Red** | `#F25959` | `true` or `false` |
| **Int (Int32 / Int64)** | **Sea Green** | `#40D999` | Whole numbers (`0, 1, -42`) |
| **Float / Double** | **Pale Cyan** | `#59CCFF` | Decimal numbers (`3.14159, -0.5`) |
| **String / StringName** | **Magenta** | `#E673E6` | Text strings (`"Player"`) |
| **Vector2 / Vector2i** | **Bright Gold** | `#F2CC59` | 2D coordinates `(X, Y)` |
| **Vector3 / Vector3i** | **Sun Yellow** | `#FFD940` | 3D coordinates `(X, Y, Z)` |
| **Vector4 / Vector4i** | **Peach** | `#FFBF73` | 4D coordinates `(X, Y, Z, W)` |
| **Color** | **Lavender** | `#F2B3F2` | RGBA color channels |
| **Transform2D / 3D** | **Orange** | `#D98C4D` | Matrix transform, rotation & translation |
| **Basis / Quaternion** | **Terracotta** | `#CC8066` | 3D rotation representation |
| **NodePath** | **Sky Blue** | `#73BFD9` | Scene tree node path references |
| **RID** | **Pale Green** | `#99D999` | Low-level server resource identifier |
| **ObjectRef** | **Bright Blue** | `#59A6FF` | Node / Object instance pointer |
| **Callable** | **Aqua Cyan** | `#4DE6E6` | First-class callable method handle |
| **Signal** | **Gold** | `#F2D933` | Event broadcast emitter |
| **Array / Containers** | **Ochre** | `#D9A666` | Ordered list of elements |
| **Dictionary** | **Turquoise** | `#66D9D9` | Key-value mapping table |
| **Packed Arrays** | **Tan** | `#BF9959` | Contiguous memory data buffers |
| **Wildcard (`T`)** | **Light Gray** | `#CCCCCC` | Generic polymorphic pin |

---

## 2. Node Categories & Color Palette

KnitNodes organizes all nodes into distinct visual categories with colored header banners:

1. **`Event` (Crimson `#CC3347`)**: Lifecycle and input entry points that initiate graph execution.
2. **`FlowControl` (Amber `#D98C26`)**: Directs execution branching, gates, loops, and coroutines.
3. **`PureFunction` (Emerald `#26B366`)**: Deterministic calculations without side effects. Evaluated lazily on-demand when pulled by execution nodes.
4. **`ImpureAction` (Cobalt `#337ACC`)**: Operations with stateful side effects (physics, node tree changes, audio).
5. **`VariableGet` / `VariableSet` (Teal `#269999`)**: Reads and writes graph variables or script properties.
6. **`SubGraph` (Purple `#8C4DD9`)**: Reusable nested graphs and macro instances.
7. **`Reroute` (Neutral `#808080`)**: Zero-cost visual routing knots to keep complex graphs organized.

---

## 3. Events & Engine Lifecycle Nodes

Events are entry points called automatically by the engine or triggered by custom game signals.

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Ready Event`** | *None* | `FlowOut` | `func _ready():` | Executes once when the node and all of its children have entered the active SceneTree. Ideal for setup and initialization. |
| **`Process Event`** | *None* | `FlowOut`, `delta (Float)` | `func _process(delta):` | Executes every rendered frame. `delta` provides the elapsed time in seconds since the previous frame. |
| **`Physics Process Event`** | *None* | `FlowOut`, `delta (Float)` | `func _physics_process(delta):` | Executes on a fixed physics timestep (typically 60Hz). Used for movement, raycasting, and physics interactions. |
| **`Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _input(event):` | Intercepts raw input events from the OS (keyboard, mouse, gamepad) before they reach the UI. |
| **`Unhandled Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _unhandled_input(event):` | Fires for gameplay input events that were not consumed by UI control nodes. |
| **`GUI Input Event`** | *None* | `FlowOut`, `event (InputEvent)` | `func _gui_input(event):` | Fires when a user interacts directly with a UI `Control` node. |
| **`Draw Event`** | *None* | `FlowOut` | `func _draw():` | CanvasItem 2D custom rendering hook. Used to draw custom vectors, lines, and textures. |
| **`Custom Signal Event`** | *None* | `FlowOut`, *Signal Params...* | `signal my_signal; func _on_my_signal():` | Triggered whenever a connected script or node signal is broadcast. Automatically exposes the signal's arguments as output data pins. |

---

## 4. Flow Control & Gate Nodes

Control flow nodes govern how impulses travel through your graph, enabling branching, loops, gates, and asynchronous delays.

```
       [ Do Once ]                     [ Flip Flop ]                      [ Gate ]
  [In]  -----> [Out]              [In]  -----> [A (Exec)]           [In]    -----> [Out]
  [Reset]                         (Is A) ----> [B (Exec)]           [Open]
                                                                    [Close]
                                                                    [Toggle]
```

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Branch (If / Else)`** | `FlowIn`, `Condition (Bool)` | `True (Exec)`, `False (Exec)` | `if condition: ... else: ...` | Routes execution to `True` if `Condition` is true, otherwise routes to `False`. |
| **`Sequence`** | `FlowIn` | `Then 0`, `Then 1`, `... (Exec)` | Sequential lines of code | Executes multiple execution flow branches sequentially in a single frame. Expandable with interactive `+ Pin` / `- Pin` buttons. |
| **`Do Once`** | `In (Exec)`, `Reset (Exec)`, `Start Closed (Bool)` | `Out (Exec)` | Stateful single-trigger gate | Executes its `Out` flow branch exactly once upon receiving an impulse. Ignores subsequent impulses until a reset impulse arrives on `Reset`. |
| **`Flip Flop`** | `In (Exec)` | `A (Exec)`, `B (Exec)`, `Is A (Bool)` | Alternating toggle | Alternates execution between output `A` and output `B` on consecutive impulses. The boolean output `Is A` indicates whether branch `A` just executed. |
| **`Gate`** | `In`, `Open`, `Close`, `Toggle`, `Start Closed` | `Out (Exec)`, `Is Open (Bool)` | Controlled flow gate | When open, impulses on `In` pass directly through to `Out`. When closed, impulses are blocked. State is changed by triggering `Open`, `Close`, or `Toggle`. |
| **`Multi-Gate`** | `In (Exec)`, `Reset (Exec)`, `Is Random (Bool)`, `Loop (Bool)` | `Out 0`, `Out 1`, `... (Exec)` | Multi-way router | Cycles impulses sequentially or randomly across multiple output pins. Can be expanded dynamically with `+ Pin`. |
| **`While Loop`** | `FlowIn`, `Condition (Bool)` | `LoopBody (Exec)`, `Completed (Exec)` | `while condition:` | Repeatedly fires `LoopBody` as long as `Condition` evaluates to `true`. When false, fires `Completed`. |
| **`For Each Loop`** | `FlowIn`, `Collection (Array)` | `LoopBody (Exec)`, `Element (T)`, `Index (Int)`, `Completed (Exec)` | `for item in array:` | Iterates through each element in an array, outputting the current element and its index, then triggers `Completed`. |
| **`Delay (Seconds)`** | `FlowIn`, `Seconds (Float)` | `FlowOut (Exec)` | `await get_tree().create_timer(sec).timeout` | Suspends graph coroutine execution for the specified duration in seconds without blocking the main thread. |
| **`Delay (Frames)`** | `FlowIn`, `Frames (Int)` | `FlowOut (Exec)` | `await get_tree().process_frame` | Suspends graph coroutine execution for N engine frame ticks. |
| **`Return`** | `FlowIn`, `Value (T)` | *Terminal* | `return value` | Immediately terminates the current graph execution or function, returning an optional output value. |

---

## 5. Gameplay & Physics Power Nodes

High-level nodes that encapsulate multi-line boilerplate physics and gameplay systems into unified, single-node solutions.

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Character Move & Jump 3D`** | `FlowIn`, `Character (Object)`, `Input Dir (Vec2)`, `Speed (Float)`, `Jump (Bool)`, `Jump Velocity (Float)`, `Gravity (Float)` | `FlowOut (Exec)`, `Velocity (Vec3)`, `On Floor (Bool)` | Unified CharacterBody3D controller | Performs floor checks, gravity accumulation, jump impulse application, horizontal direction scaling, and calls `move_and_slide()` automatically in a single node. |
| **`Raycast Query 3D`** | `FlowIn`, `Origin (Vec3)`, `Target (Vec3)`, `Collision Mask (Int)` | `FlowOut (Exec)`, `Hit (Bool)`, `Hit Position (Vec3)`, `Hit Normal (Vec3)`, `Hit Collider (Object)` | Direct `PhysicsDirectSpaceState3D.intersect_ray()` | Directly queries 3D physics space along a line segment, returning whether a collision occurred and providing hit details. |
| **`Tween Property`** | `FlowIn`, `Target (Object)`, `Property (String)`, `Final Value (T)`, `Duration (Float)`, `Ease Type (Int)`, `Trans Type (Int)` | `FlowOut (Exec)`, `Tween (Object)` | `create_tween().tween_property(...)` | Automatically constructs and starts a SceneTree `Tween` to smoothly interpolate any property on a target object to a target value. |
| **`Play Sound 3D`** | `FlowIn`, `Sound (AudioStream)`, `Position (Vec3)`, `Volume dB (Float)`, `Pitch (Float)` | `FlowOut (Exec)` | `AudioStreamPlayer3D` one-shot | Spawns a temporary positional 3D audio emitter that plays a sound stream at the specified coordinates and automatically frees itself. |

---

## 6. Workflow & Productivity Nodes

Ergonomic nodes designed to eliminate graph clutter and speed up visual development.

```
       [ Reroute Knot ]                        [ Math Expression ]
     (●) ------------> (●)                    f = (speed * delta) + sin(t)
                                              (● Speed)
                                              (● Delta) -------> (● Result)
                                              (● t)
```

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Reroute Knot`** | `Input (T)` | `Output (T)` | Visual pass-through | Zero-overhead pass-through knot that compiles via direct register aliasing (0 bytecode cost). Organizes long wires and prevents wire overlap. |
| **`Math Expression`** | *Dynamic Variables (e.g. `a`, `b`, `speed`)* | `Result (Float)` | `(a + b) * speed - sin(t)` | Contains an inline formula editor `f = ...` with a recursive-descent compiler. Dynamically creates typed data input pins matching variable names in the formula. |

---

## 7. Struct Construction & Deconstruction (Make / Break)

Deconstruct and assemble Godot core mathematical data structures with dedicated zero-overhead opcodes.

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Make Vector2`** | `x (Float)`, `y (Float)` | `Vector2` | `Vector2(x, y)` | Assembles a 2D vector from individual X and Y components. |
| **`Break Vector2`** | `Vector2` | `x (Float)`, `y (Float)` | `v.x, v.y` | Extracts X and Y floating-point components from a 2D vector. |
| **`Make Vector3`** | `x (Float)`, `y (Float)`, `z (Float)` | `Vector3` | `Vector3(x, y, z)` | Assembles a 3D vector from individual X, Y, and Z components. |
| **`Break Vector3`** | `Vector3` | `x (Float)`, `y (Float)`, `z (Float)` | `v.x, v.y, v.z` | Extracts X, Y, and Z components from a 3D vector. |
| **`Make Vector4`** | `x`, `y`, `z`, `w (Float)` | `Vector4` | `Vector4(x, y, z, w)` | Assembles a 4D vector from X, Y, Z, and W components. |
| **`Break Vector4`** | `Vector4` | `x`, `y`, `z`, `w (Float)` | `v.x, v.y, v.z, v.w` | Extracts X, Y, Z, and W components from a 4D vector. |
| **`Make Color`** | `r`, `g`, `b`, `a (Float)` | `Color` | `Color(r, g, b, a)` | Assembles an RGBA color from normalized `[0, 1]` channel components. |
| **`Break Color`** | `Color` | `r`, `g`, `b`, `a (Float)` | `c.r, c.g, c.b, c.a` | Extracts normalized Red, Green, Blue, and Alpha components. |
| **`Make Rect2`** | `position (Vector2)`, `size (Vector2)` | `Rect2` | `Rect2(pos, size)` | Assembles a 2D bounding rectangle from position and size vectors. |
| **`Break Rect2`** | `Rect2` | `position (Vector2)`, `size (Vector2)` | `r.position, r.size` | Extracts position and size from a 2D rectangle. |
| **`Make Transform3D`** | `rotation (Vector3)`, `origin (Vector3)` | `Transform3D` | `Transform3D(Basis.from_euler(r), o)` | Assembles a 3D transform matrix from Euler rotation angles and translation origin. |
| **`Break Transform3D`** | `Transform3D` | `origin (Vector3)`, `rotation (Vector3)`, `scale (Vector3)` | `t.origin, t.basis.get_euler(), t.basis.get_scale()` | Extracts 3D translation origin, Euler rotation angles, and scale vector. |

---

## 8. Variables & Memory Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Get Variable`** | *None* | `Value (T)` | `my_var` | Reads the current value of a script member or graph-local variable. |
| **`Set Variable`** | `FlowIn`, `Value (T)` | `FlowOut`, `Value (T)` | `my_var = value` | Updates the stored value of a variable and passes execution flow forward. |
| **`Constant Value`** | *Configured in inspector* | `Value (T)` | `const PI = 3.14159` | Emits a constant literal value with zero memory allocation. |
| **`Get Self`** | *None* | `Self (ObjectRef)` | `self` | Returns an object reference to the current instance executing the graph. |
| **`Get Node Shortcut`** | `Path (NodePath)` | `Node (ObjectRef)` | `$Path` / `get_node("Path")` | Resolves and returns a node in the scene tree relative to the current script instance. |

---

## 9. Operators & Arithmetic Nodes

All binary arithmetic nodes support dynamic type deduction (operating on scalars, vectors, and matrices).

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Add (+)`** | `a (T)`, `b (T)` | `Result (T)` | `a + b` | Adds numbers, adds vectors component-wise, concatenates strings, or merges arrays. Expandable with `+ Pin`. |
| **`Subtract (-)`** | `a (T)`, `b (T)` | `Result (T)` | `a - b` | Subtracts `b` from `a`. |
| **`Multiply (*)`** | `a (T)`, `b (T)` | `Result (T)` | `a * b` | Multiplies numbers, scales vectors, or transforms vectors by matrices. |
| **`Divide (/)`** | `a (T)`, `b (T)` | `Result (T)` | `a / b` | Divides `a` by `b` with automatic VM division-by-zero protection. |
| **`Modulo (%)`** | `a (T)`, `b (T)` | `Result (T)` | `a % b` | Computes integer or floating-point remainder of division. |
| **`Power (**)`** | `Base (Float)`, `Exp (Float)` | `Result (Float)` | `base ** exp` | Computes `Base` raised to the power of `Exp`. |
| **`Negate (-)`** | `Value (T)` | `Result (T)` | `-value` | Inverts the mathematical sign of a number or vector. |
| **`Bitwise AND (&)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a & b` | Performs bitwise AND masking. |
| **`Bitwise OR (|)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a | b` | Performs bitwise OR combination. |
| **`Bitwise XOR (^)`** | `a (Int)`, `b (Int)` | `Result (Int)` | `a ^ b` | Performs bitwise exclusive OR. |
| **`Bitwise NOT (~)`** | `Value (Int)` | `Result (Int)` | `~value` | Inverts all bit flags. |
| **`Bit Shift Left (<<)`**| `Value (Int)`, `Shift (Int)` | `Result (Int)` | `val << shift` | Shifts bit representations left. |
| **`Bit Shift Right (>>)`**| `Value (Int)`, `Shift (Int)` | `Result (Int)` | `val >> shift` | Shifts bit representations right. |
| **`Equal (==)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a == b` | Returns `true` if `a` and `b` have equal values. |
| **`Not Equal (!=)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a != b` | Returns `true` if `a` and `b` are not equal. |
| **`Less Than (<)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a < b` | Returns `true` if `a` is strictly less than `b`. |
| **`Less Equal (<=)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a <= b` | Returns `true` if `a <= b`. |
| **`Greater Than (>)`** | `a (T)`, `b (T)` | `Result (Bool)` | `a > b` | Returns `true` if `a > b`. |
| **`Greater Equal (>=)`**| `a (T)`, `b (T)` | `Result (Bool)` | `a >= b` | Returns `true` if `a >= b`. |
| **`Logical NOT (!)`** | `Value (Bool)` | `Result (Bool)` | `not value` | Inverts a boolean condition (`true` becomes `false`). |
| **`Logical AND (&&)`** | `a (Bool)`, `b (Bool)` | `Result (Bool)` | `a and b` | Returns `true` if both boolean inputs are true. |
| **`Logical OR (||)`** | `a (Bool)`, `b (Bool)` | `Result (Bool)` | `a or b` | Returns `true` if at least one boolean input is true. |

---

## 10. Math, Trigonometry & Geometry Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Sin / Cos / Tan`** | `Angle (Float)` | `Result (Float)` | `sin(x), cos(x), tan(x)` | Computes basic trigonometric ratios for an angle in radians. |
| **`ASin / ACos / ATan`** | `Value (Float)` | `Angle (Float)` | `asin(x), acos(x), atan(x)` | Computes inverse trigonometric arc values in radians. |
| **`ATan2`** | `Y (Float)`, `X (Float)` | `Angle (Float)` | `atan2(y, x)` | Computes the four-quadrant arc tangent of Y / X in radians. |
| **`Deg To Rad`** | `Degrees (Float)` | `Radians (Float)` | `deg_to_rad(deg)` | Converts degrees to radians. |
| **`Rad To Deg`** | `Radians (Float)` | `Degrees (Float)` | `rad_to_deg(rad)` | Converts radians to degrees. |
| **`Sqrt`** | `Value (Float)` | `Result (Float)` | `sqrt(x)` | Computes square root. |
| **`Abs`** | `Value (T)` | `Result (T)` | `abs(x)` | Computes absolute non-negative magnitude of a scalar or vector. |
| **`Sign`** | `Value (T)` | `Result (T)` | `sign(x)` | Returns `-1.0` for negative, `0.0` for zero, and `+1.0` for positive numbers. |
| **`Floor / Ceil / Round`**| `Value (Float)` | `Result (Float)` | `floor(x), ceil(x), round(x)` | Rounds down, rounds up, or rounds to the nearest whole integer. |
| **`Snap`** | `Value (T)`, `Step (T)` | `Result (T)` | `snapped(x, step)` | Snaps a value or vector to the nearest grid increment `step`. |
| **`Clamp`** | `Value (T)`, `Min (T)`, `Max (T)` | `Result (T)` | `clamp(x, min, max)` | Constrains a value within the `[Min, Max]` boundary interval. |
| **`Min / Max`** | `a (T)`, `b (T)` | `Result (T)` | `min(a, b), max(a, b)` | Returns the lesser or greater of two values. Expandable with `+ Pin`. |
| **`Wrap`** | `Value (T)`, `Min (T)`, `Max (T)` | `Result (T)` | `wrap(x, min, max)` | Wraps a value around boundaries (e.g. wrapping angles in range `[0, TAU]`). |
| **`Ping Pong`** | `Value (Float)`, `Length (Float)` | `Result (Float)` | `pingpong(val, len)` | Bounces a value back and forth between 0 and `Length`. |

---

## 11. Interpolation, Curves & Easing Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Lerp`** | `From (T)`, `To (T)`, `Weight (Float)` | `Result (T)` | `lerp(from, to, weight)` | Performs linear interpolation. `weight = 0.0` returns `From`, `weight = 1.0` returns `To`. |
| **`Lerp Angle`** | `From (Float)`, `To (Float)`, `Weight (Float)` | `Result (Float)` | `lerp_angle(from, to, w)` | Interpolates rotational angles around the unit circle without 360° wrapping artifacts. |
| **`Inverse Lerp`** | `From (Float)`, `To (Float)`, `Value (Float)` | `Weight (Float)` | `inverse_lerp(from, to, val)` | Calculates the normalized interpolation factor `[0, 1]` of `Value` between `From` and `To`. |
| **`Remap Range`** | `Val, InMin, InMax, OutMin, OutMax` | `Result (Float)` | `remap(val, is, ie, os, oe)` | Re-maps a value from one input range to a corresponding output range. |
| **`SmoothStep`** | `From (Float)`, `To (Float)`, `Value (Float)` | `Result (Float)` | `smoothstep(from, to, val)` | Hermite smoothstep S-curve interpolation with zero derivative at boundaries. |
| **`Move Toward`** | `From (Float)`, `To (Float)`, `Delta (Float)` | `Result (Float)` | `move_toward(from, to, delta)` | Steps a value towards a target by a maximum constant increment `delta`. |
| **`Ease Curve`** | `Value (Float)`, `Curve (Float)` | `Result (Float)` | `ease(x, curve)` | Applies non-linear easing curvature transformation. |

---

## 12. Random Number Generation Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Random Int (randi)`** | *None* | `Value (Int)` | `randi()` | Returns a random unsigned 32-bit integer. |
| **`Random Float (randf)`** | *None* | `Value (Float)` | `randf()` | Returns a random normalized floating-point number in range `[0.0, 1.0]`. |
| **`Random Int Range`** | `From (Int)`, `To (Int)` | `Value (Int)` | `randi_range(from, to)` | Generates a random integer within the closed range `[From, To]`. |
| **`Random Float Range`** | `From (Float)`, `To (Float)` | `Value (Float)` | `randf_range(from, to)` | Generates a random float within the range `[From, To]`. |
| **`Random Gaussian`** | `Mean (Float)`, `Deviation (Float)` | `Value (Float)` | `randfn(mean, dev)` | Generates a normally distributed random float matching standard Gaussian distribution. |
| **`Randomize`** | `FlowIn` | `FlowOut` | `randomize()` | Re-seeds the global pseudo-random number generator from hardware entropy. |

---

## 13. Containers & Strings Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Construct Array`** | `Elem 0..N (T)` | `Array (Array[T])` | `[a, b, c]` | Constructs an array from individual inputs. Expandable via `+ Pin`. |
| **`Array Append`** | `FlowIn`, `Array`, `Item (T)` | `FlowOut` | `array.append(item)` | Appends an element to the end of an array. |
| **`Array Size (len)`** | `Container` | `Size (Int)` | `array.size()` / `len(c)` | Returns the element count of an array, string, or dictionary. |
| **`Construct Dictionary`**| `Key0, Val0, ...` | `Dict (Dictionary)` | `{k0: v0, ...}` | Constructs a key-value dictionary. Expandable via `+ Pin`. |
| **`Dict Get`** | `Dict`, `Key`, `Default` | `Value (T)` | `dict.get(key, default)` | Retrieves value for `Key`, returning `Default` if key is missing. |
| **`Dict Set`** | `FlowIn`, `Dict`, `Key`, `Val` | `FlowOut` | `dict[key] = val` | Inserts or updates a key-value pair in a dictionary. |
| **`Dict Has`** | `Dict`, `Key` | `HasKey (Bool)` | `dict.has(key)` | Checks whether a dictionary contains the specified key. |
| **`String Format (str)`**| `Values 0..N` | `Result (String)` | `str(...)` | Stringifies and concatenates all input parameters. Expandable via `+ Pin`. |
| **`Range Generator`** | `From (0)`, `To`, `Step (1)` | `Array (Array[Int])` | `range(from, to, step)` | Generates a sequential integer array `[From..To]`. |
| **`Load Resource`** | `FlowIn`, `Path (String)` | `FlowOut`, `Resource` | `load(path)` | Dynamically loads a resource asset from a virtual resource path (`res://...`). |

---

## 14. Signals & Callables Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Emit Signal`** | `FlowIn`, `Target`, `Signal Name`, *Args...* | `FlowOut` | `signal_name.emit(args)` | Broadcasts a signal event to all connected listeners. |
| **`Connect Signal`** | `FlowIn`, `Target`, `Signal Name`, `Callable` | `FlowOut` | `signal_name.connect(callable)` | Subscribes a callable method to a signal. |
| **`Call Callable`** | `FlowIn`, `Callable`, *Args...* | `FlowOut`, `Result (T)` | `callable.call(args)` | Invokes a first-class `Callable` object dynamically. |
| **`Await Signal`** | `FlowIn`, `Signal` | `FlowOut`, `Signal Args...` | `await signal_name` | Suspends the coroutine until the designated signal is fired, outputting signal payload arguments. |

---

## 15. Types & Dynamic ClassDB Reflection Nodes

KnitNodes features dynamic discovery that automatically reflects all engine methods, properties, and signals directly from `ClassDB`.

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Type Of`** | `Value (T)` | `Type ID (Int)` | `typeof(val)` | Returns the integer `Variant::Type` identifier of a value. |
| **`Is Instance Valid`** | `Object (ObjectRef)` | `Is Valid (Bool)` | `is_instance_valid(obj)` | Verifies that an object pointer is alive and not queued for deletion (`queue_free`). |
| **`Type Test (Is)`** | `Value (T)`, `Type` | `Is Match (Bool)` | `value is MyClass` | Tests whether an instance is an instance of a specific class or Variant type. |
| **`Type Cast (As)`** | `Value (T)`, `Target Type` | `Result (Target Type)` | `value as MyClass` | Performs a safe type cast, returning `null` if the object is incompatible. |
| **`ClassDB Dynamic Action`**| `FlowIn`, `Target`, *Args...* | `FlowOut`, `Result (T)` | `target.method(args)` | Dynamically discovers and invokes any C++ engine method on any engine class without manual wrappers. |

---

## 16. Actions, Logging & Debugging Nodes

| Node Name | Inputs | Outputs | GDScript Equivalent | Detailed Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **`Print`** | `FlowIn`, `Message (String)` | `FlowOut` | `print(message)` | Prints a formatted text message to the engine output console. |
| **`Print Rich`** | `FlowIn`, `Message (String)` | `FlowOut` | `print_rich(msg)` | Prints BBCode-formatted colored text and markup to the editor console. |
| **`Print Error`** | `FlowIn`, `Message (String)` | `FlowOut` | `printerr(msg)` | Outputs an error-highlighted message directly to standard error output. |
| **`Push Error`** | `FlowIn`, `Message (String)` | `FlowOut` | `push_error(msg)` | Registers an error with full callstack in the Engine Debugger. |
| **`Push Warning`** | `FlowIn`, `Message (String)` | `FlowOut` | `push_warning(msg)` | Registers a warning in the Engine Debugger. |
| **`Assert`** | `FlowIn`, `Condition (Bool)` | `FlowOut` | `assert(condition)` | Halts execution if `Condition` is `false` during debug builds. |
| **`Move and Slide`** | `FlowIn` | `FlowOut` | `move_and_slide()` | Calls kinematic motion and collision solver on `CharacterBody3D`. |

---

## 17. 1-Click Bidirectional GDScript $\leftrightarrow$ KnitNodes Transpiler

ZeGFX KnitNodes includes a built-in C++ AST transpiler (`KnitsGDScriptTranspiler`) that bridges GDScript code and visual KnitGraphs with zero lock-in.

```
                     BIDIRECTIONAL CONVERTER ARCHITECTURE
                     
     ┌─────────────────┐       GDScriptParser AST       ┌─────────────────┐
     │  GDScript Code  │ ─────────────────────────────> │  KnitNodes Graph│
     │    (.gd file)   │ <───────────────────────────── │  (.knits graph) │
     └─────────────────┘    Code Generation Emitter     └─────────────────┘
```

### Features
1. **GDScript $\to$ KnitGraph (`Import GDScript`):**
   * Click **"Import GDScript"** on the KnitNodes editor toolbar.
   * Paste any `.gd` source code.
   * Leverages Godot's built-in `GDScriptParser` to parse the script into an Abstract Syntax Tree.
   * Reconstitutes lifecycle events, custom functions, branches (`if`/`else`), loops (`while`/`for`), returns, math expressions, and action calls, wiring them with an automatic topological layout.
2. **KnitGraph $\to$ GDScript (`Export GDScript`):**
   * Click **"Export GDScript"** on the KnitNodes editor toolbar.
   * Generates clean, PEP8-compliant, formatted GDScript code with 1-click clipboard copy.

---

## 18. Annotations & Inspector Decorators

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

## 19. Production Recipes & Architectures

### Recipe 1: 3D Character Controller with Move & Jump 3D Node
```
[ Physics Process Event ]
           |
           v
 [ Character Move & Jump 3D ]
    ( Character: self )
    ( Input Dir: (Get Vector "move_left", "move_right", ...) )
    ( Speed: 6.0 )
    ( Jump: (Is Action Just Pressed "jump") )
    ( Jump Velocity: 5.5 )
    ( Gravity: 9.8 )
```

### Recipe 2: Stateful Combat Ability with Do Once & Cooldown
```
[ Input Event (Attack) ]
           |
           v
      [ Do Once ] -----------------> [ Play Attack Animation ]
       ^      |                               |
       |      +-------------------------> [ Delay (1.2s) ]
       |                                      |
       +--------------------------------------+ (Reset)
```

### Recipe 3: Raycast Ground Detection & Surface Alignment
```
[ Physics Process Event ]
           |
           v
  [ Raycast Query 3D ]
    ( Origin: Position )
    ( Target: Position - Vector3(0, 2, 0) )
           |
      (Hit)|
           v
      [ Branch ]
      (True) ----> [ Align Transform with Hit Normal ]
```
