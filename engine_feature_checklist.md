# AAA Engine Feature Checklist — v2 (Upgraded)

*Every original item is kept exactly as written, in its original place. Additions are marked "(new)" on the header line. Six categories at the end didn't exist in the original at all.*

---

## What Changed

The original is a genuinely strong **rendering and core-tech** checklist — the Verify/Modern games format is a good design, and coverage of terrain, lighting, water, and shadows is close to industry-standard.

This pass adds 58 items. Some are folded into existing categories — most importantly Ray Tracing, which had no denoising step (the thing that makes RT usable at all in real time), and Audio, which had two items total. The other 27 sit in six categories that were entirely absent: AI & Navigation, Gameplay Framework & Scripting, Networking & Multiplayer, Input & Camera Systems, UI/UX/Localization/Accessibility, and Platform & Live Services.

The original covers what the player *sees* extremely well. This pass fills in what makes it a *game engine* rather than a renderer: how NPCs think, how a designer scripts an ability without touching engine code, how state gets saved, how a controller gets rebound.

**Total: 137 items across 22 categories, up from 79 across 16.**

---

## Table of Contents

1. Terrain & World
2. Foliage & Vegetation
3. Sky & Atmosphere
4. Water
5. Lighting & Global Illumination
6. Shadows
7. Ray Tracing (if targeting high-end)
8. Materials & Surfaces
9. Distant/Dynamic Object Rendering
10. Animation
11. Particles & VFX
12. Post-Processing / Image Quality
13. Physics
14. AI & Navigation *(new)*
15. Audio
16. Gameplay Framework & Scripting *(new)*
17. Networking & Multiplayer *(new — if targeting online/co-op)*
18. Input & Camera Systems *(new)*
19. UI/UX, Localization & Accessibility *(new)*
20. Platform & Live Services *(new)*
21. Tools & Pipeline
22. Performance & Stability

---

## Terrain & World

### Heightmap/mesh-based terrain system
**Verify:** Import a heightmap, place it, walk on it in-game.
**Modern games:** The ground under everything in any open world.

### Terrain LOD (CDLOD/quadtree/clipmap)
**Verify:** Wireframe view from far away — triangle density drops smoothly with distance, no cracks at LOD seams.
**Modern games:** Mountains stay crisp up close, cheap at the horizon.

### Terrain multi-layer texturing/splatmaps
**Verify:** Paint 2+ materials on one terrain, check smooth blending at boundaries.
**Modern games:** Rock blending into grass, sand into canyon.

### Runtime terrain editing/sculpting
**Verify:** Sculpt in editor or in-game, lighting/collision updates immediately.
**Modern games:** Live landscape tools, destructible terrain.

### World streaming (load/unload by distance)
**Verify:** Walk far from a region, confirm memory usage actually drops.
**Modern games:** Why open worlds don't load the whole map into RAM at once.

### Async asset streaming from disk
**Verify:** Move fast toward unloaded content — pops in without a hard freeze.
**Modern games:** Seamless traversal with no loading screens.

### Large-world coordinate precision (origin rebasing / double precision)
**Verify:** Travel very far from world origin, check for jittering geometry.
**Modern games:** Why huge maps don't get shaky/glitchy far from spawn.

### Virtual texturing / megatexture-style streaming *(new)*
**Verify:** Author large amounts of unique, non-tiling terrain texture data — VRAM usage stays flat regardless of how much of the world is unique.
**Modern games:** Hand-painted detail across an entire map instead of repeating tiled textures.

---

## Foliage & Vegetation

### GPU-instanced grass rendering
**Verify:** Dense grass field (500k+ blades), framerate holds, GPU-bound not CPU-bound.
**Modern games:** Dense grass fields that don't tank performance.

### Per-instance GPU culling (frustum + occlusion) for foliage
**Verify:** Frame-capture — draw calls drop when facing away or occluded.
**Modern games:** Forests not submitting millions of invisible draws.

### Foliage LOD / billboard transition at distance
**Verify:** Walk away from grass — smooth transition to cheaper geometry, no popping.
**Modern games:** Grass simplifying far from camera.

### Global wind system
**Verify:** All trees/grass sway coherently in one direction/strength.
**Modern games:** Coordinated forest sway.

### Procedural vegetation placement / density maps
**Verify:** Paint a density mask, foliage follows it — dense in valleys, sparse on rock.
**Modern games:** Natural biome transitions instead of uniform carpet.

### Tree/rock instancing at scale
**Verify:** Thousands of instances, single draw call or GPU-driven indirect draws, not per-object.
**Modern games:** Dense forests rendered efficiently.

### Foliage physics/interaction *(new)*
**Verify:** Walk or drive through grass — it bends and springs back around the collider in real time, distinct from uniform wind sway.
**Modern games:** Grass parting around a character's legs instead of clipping straight through.

---

## Sky & Atmosphere

### Physically based sky (Rayleigh/Mie scattering)
**Verify:** Sky gradient looks physically correct at sunrise/noon/sunset.
**Modern games:** Realistic sky color across time of day.

### Sky-view LUT / precomputed atmosphere
**Verify:** Frame capture — sky rendered from small LUT, near-zero per-pixel cost.
**Modern games:** Cheap, accurate sky technique used industry-wide.

### Aerial perspective / distance haze
**Verify:** Distant mountains visibly desaturate/blue-shift with distance.
**Modern games:** Why far mountains look hazy instead of full contrast.

### Volumetric clouds (3D, raymarched)
**Verify:** Clouds show depth/parallax as camera moves, light scattering on edges facing sun.
**Modern games:** Puffy dimensional clouds, not flat billboards.

### Dynamic time-of-day cycle
**Verify:** Advance day/night, sky/lighting/fog update together automatically.
**Modern games:** Automatic day/night systems.

### Volumetric fog / light shafts
**Verify:** Under a tree canopy or in a valley — visible light shafts and depth haze.
**Modern games:** God-rays through trees, valley fog at dawn.

### Weather system (rain, snow, storms)
**Verify:** Toggle weather state, visuals and audio change, puddles/snow accumulate if applicable.
**Modern games:** Dynamic weather affecting look and gameplay.

### Cloud shadows on terrain *(new)*
**Verify:** Advance time or move clouds — moving shadow patches visibly sweep across distant terrain.
**Modern games:** Drifting cloud-shadow patterns across large vistas instead of flat, uniform lighting.

---

## Water

### Dynamic water surface (Gerstner or FFT ocean)
**Verify:** Wireframe view — surface geometry moves/displaces over time.
**Modern games:** Ocean motion in open-world/pirate games.

### Water reflections (planar or SSR)
**Verify:** Objects near water reflect correctly, updates with camera/object movement.
**Modern games:** Ships/cliffs reflected on water surface.

### Depth-based water color/refraction
**Verify:** Shallow water near shore lighter/transparent, deep water darker/opaque.
**Modern games:** Tropical shallow-water gradient look.

### Shoreline foam & wetness
**Verify:** Water-rock/sand boundary shows foam buildup and darkened wet strip.
**Modern games:** Foam at waterlines on rocky shores.

### Underwater rendering (caustics, fog, distortion)
**Verify:** Submerge camera — color shift, caustic light patterns, visibility falloff.
**Modern games:** Diving/swimming sequences.

### Water-object physical interaction *(new)*
**Verify:** Drop an object or drive a boat into water — displacement, wake, and splash scale with mass/speed, not a fixed shader ripple.
**Modern games:** Buoyant boats, splashes from falling objects, visible wake trails.

---

## Lighting & Global Illumination

### PBR metallic-roughness material pipeline
**Verify:** Material response correct under changing light angle/intensity, energy-conserving.
**Modern games:** Baseline realistic material rendering.

### Image-based lighting (IBL) from sky/probes
**Verify:** Objects show ambient reflections matching the environment, not flat ambient color.
**Modern games:** Correct ambient lighting outdoors.

### Real-time global illumination (probes/DDGI/SSGI/RTGI)
**Verify:** Move a light or object — indirect bounce lighting on nearby surfaces updates live.
**Modern games:** Color bleeding, bounced light in rooms/caves.

### Reflections (SSR, RT reflections, or reflection probes)
**Verify:** Reflective surfaces show scene content, updates as camera/objects move.
**Modern games:** Wet streets, mirrors, glossy floors reflecting the scene.

### Baked lightmaps / lighting for static geometry (fallback path)
**Verify:** Static-only scene lit correctly with no real-time GI enabled.
**Modern games:** Budget-friendly lighting path for lower-end targets.

### Local light types (point, spot, area) with shadows
**Verify:** Place each light type, confirm correct falloff and shadow casting.
**Modern games:** Interior/prop lighting fidelity.

### Emissive surfaces contributing to GI *(new)*
**Verify:** Place a bright emissive material (neon sign, lava) near a wall — it visibly tints/bounces colored light onto nearby surfaces rather than just glowing itself.
**Modern games:** Neon-lit alleys and lava caves that actually color the space around them.

---

## Shadows

### Cascaded shadow maps (CSM) for directional light
**Verify:** Shadow resolution consistent from near to far camera range, no visible cascade seams.
**Modern games:** Sharp sun shadows at all distances.

### Shadow filtering (PCF/PCSS) for soft edges
**Verify:** Shadow edges are soft, not hard-aliased pixel steps.
**Modern games:** Natural-looking soft shadow penumbra.

### Point/spot light shadows
**Verify:** Local lights cast correct shadows in enclosed spaces.
**Modern games:** Torch/lamp shadows indoors.

### Contact shadows / screen-space shadow detail
**Verify:** Small objects near ground show correct close-contact shadowing.
**Modern games:** Fine-grained shadow detail missed by shadow maps.

### Virtual shadow maps *(new)*
**Verify:** Compare close-up shadow crispness to standard CSM at the same camera distance — no cascade-transition softening or resolution falloff near the viewer.
**Modern games:** High-resolution shadows that keep pace with dense, high-poly modern geometry.

---

## Ray Tracing (if targeting high-end)

### Hardware RT API integration (DXR/VK_KHR_ray_tracing)
**Verify:** BLAS/TLAS build without CPU stalls, RT pass executes without crash.
**Modern games:** Foundation for any RT feature.

### RT shadows
**Verify:** Compare to shadow-map shadows — sharper contact, no cascade artifacts.
**Modern games:** Pin-sharp accurate shadows.

### RT reflections
**Verify:** Reflections show geometry not visible in screen-space (SSR fails at screen edges, RT doesn't).
**Modern games:** Accurate reflections regardless of camera angle.

### RT/path-traced global illumination
**Verify:** Fully dynamic bounced lighting, no light leaking through walls.
**Modern games:** Cyberpunk 2077 Overdrive-tier lighting.

### RT denoising (temporal + spatial) *(new)*
**Verify:** Freeze camera and lighting — the RT image converges from noisy to clean within a handful of frames instead of staying speckled.
**Modern games:** The part that makes every feature above actually shippable — low sample-count RT is unusable noise without it.

### RT ambient occlusion (RTAO) *(new)*
**Verify:** Compare contact darkening in corners/crevices against SSAO — no screen-edge cutoff or missing off-screen occluders.
**Modern games:** More accurate, stable alternative to screen-space AO where the budget allows.

---

## Materials & Surfaces

### Clear coat (car paint, lacquer)
**Verify:** Object shows a distinct secondary specular layer over base material.
**Modern games:** Vehicle paint, wet varnished wood.

### Subsurface scattering
**Verify:** Light passing through thin geometry (ears, leaves) shows translucent glow.
**Modern games:** Realistic skin, foliage backlighting.

### Anisotropic specular
**Verify:** Brushed metal shows elongated, direction-dependent highlights.
**Modern games:** Brushed metal, hair highlights.

### Decal system
**Verify:** Place a decal on a surface — projects correctly onto geometry curvature.
**Modern games:** Bullet holes, blood, graffiti, grime.

### Parallax/tessellated displacement mapping
**Verify:** Surface shows real depth from shallow angle, not flat normal-map faking.
**Modern games:** Cobblestone, brick with real depth.

### Triplanar/procedural texturing *(new)*
**Verify:** Apply a material to a cliff face or sculpted rock with no manual UVs — no stretching or seams at steep angles.
**Modern games:** Clean texturing on procedural terrain and rock formations without an unwrap pass.

### Material LOD *(new)*
**Verify:** Profile shader cost — distant instances fall back to a cheaper shader variant automatically.
**Modern games:** Full complex material up close, cheap approximation at the horizon.

---

## Distant/Dynamic Object Rendering

### Impostor/billboard system for distant dynamic actors
**Verify:** Place 20+ animated actors far away — stay readable, framerate doesn't collapse.
**Modern games:** Crowds, herds, armies visible at range.

### Skeletal animation LOD (reduced bones/update rate at distance)
**Verify:** Profile CPU skinning cost — drops as actors move farther away.
**Modern games:** Large-scale crowd/battle scenes.

### GPU-driven instanced rendering / indirect draws
**Verify:** Frame-capture — draw calls issued via GPU compute culling, not per-object CPU submission.
**Modern games:** Rendering thousands of objects without CPU bottleneck.

### Occlusion culling (HZB-based or portal-based)
**Verify:** Objects behind walls/terrain don't get drawn — check GPU frame stats.
**Modern games:** Dense cities/interiors rendering efficiently.

### Static mesh LOD generation & switching *(new)*
**Verify:** Walk away from a detailed prop — it swaps to progressively simpler geometry at set distances, no visible pop if authored well.
**Modern games:** One of the most basic, load-bearing optimizations in any 3D engine — easy to overlook once instancing and culling are already on the list.

### Virtualized/nanite-style geometry *(new)*
**Verify:** Import film-quality source meshes (millions of triangles) with no manual LOD authoring, framerate holds.
**Modern games:** UE5 Nanite-class rendering — near-unlimited geometric detail without a traditional per-asset LOD pipeline.

---

## Animation

### Skeletal animation blending/state machines
**Verify:** Transition between animations (walk to run) smoothly with no pops.
**Modern games:** Fluid character locomotion.

### Inverse kinematics (foot IK, look-at IK)
**Verify:** Character feet adapt to uneven terrain, don't clip through slopes.
**Modern games:** Feet planted correctly on stairs/slopes.

### Facial animation / blendshapes
**Verify:** Character face shows distinct expressions, lip sync matches dialogue.
**Modern games:** Cutscene facial performance.

### Cloth & hair simulation
**Verify:** Cape/hair reacts to movement and wind in real time, no clipping through body.
**Modern games:** Dynamic cloaks, hair physics.

### Ragdoll / physics-based animation blending
**Verify:** Character death/impact blends animation into physics-driven ragdoll smoothly.
**Modern games:** Believable death/impact reactions.

### Motion matching / data-driven locomotion *(new)*
**Verify:** Character movement searches a motion database for the best-matching pose each frame, rather than blending a fixed set of hand-authored clips.
**Modern games:** The foot-planted, natural-feeling locomotion in recent sports and open-world titles, replacing older blend-tree movement.

### Root motion support *(new)*
**Verify:** An animation drives root-bone displacement directly — travel distance matches the authored clip exactly, not a separately-tuned capsule velocity.
**Modern games:** Precise traversal and attack animations (ledge climbs, heavy swings) that need to match the clip exactly.

### Animation retargeting *(new)*
**Verify:** Apply one mocap clip to multiple differently-proportioned skeletons — motion adapts correctly with no manual re-keying.
**Modern games:** Reusing a mocap library across an entire cast instead of recapturing per character.

---

## Particles & VFX

### GPU particle simulation
**Verify:** Thousands of particles (fire, smoke, debris) at stable framerate.
**Modern games:** Large-scale explosions, weather effects.

### Particle collision with world geometry
**Verify:** Particles bounce/settle on ground/surfaces correctly, not passing through.
**Modern games:** Debris settling, sparks bouncing off walls.

### VFX-mesh interaction (e.g., wind/explosion affecting foliage)
**Verify:** Trigger an explosion near foliage — grass/branches react physically.
**Modern games:** Environmental reactivity to combat/explosions.

### Ribbon/trail renderers *(new)*
**Verify:** A fast-moving effect (weapon swing, projectile) leaves a continuous connected mesh trail, not a chain of separate sprites.
**Modern games:** Sword-swing trails, magic streaks, jet contrails.

### Particle LOD *(new)*
**Verify:** Move far from a dense effect — particle count/complexity drops automatically, framerate cost shrinks with distance.
**Modern games:** Battles and explosions stay affordable when seen from far away.

---

## Post-Processing / Image Quality

### Temporal anti-aliasing (TAA)
**Verify:** Edges clean in motion, minimal ghosting on thin/moving geometry.
**Modern games:** Standard AA solution in nearly all modern titles.

### Tonemapping (ACES/AgX/filmic)
**Verify:** Bright/dark scenes stay balanced, no blown highlights or crushed blacks.
**Modern games:** Cinematic, natural color grading.

### Bloom
**Verify:** Bright light sources show soft glow bleeding into surrounding pixels.
**Modern games:** Sun glare, neon light bloom.

### Depth of field
**Verify:** Out-of-focus background/foreground blurs correctly relative to focus point.
**Modern games:** Cinematic camera focus in cutscenes/photo mode.

### Motion blur
**Verify:** Fast camera/object movement shows directional blur streaks.
**Modern games:** Smooths perceived motion at lower framerates.

### Color grading / LUTs
**Verify:** Apply a LUT, scene mood/color shifts globally and consistently.
**Modern games:** Distinct visual identity per game/area.

### Screen-space ambient occlusion (SSAO/GTAO)
**Verify:** Contact points (corners, crevices) show subtle darkening.
**Modern games:** Grounded, non-flat lighting in tight spaces.

### Upscaling (DLSS/FSR/XeSS/TSR)
**Verify:** Render at lower internal resolution, output matches native quality closely.
**Modern games:** Performance/quality tradeoff standard in current titles.

### Auto-exposure / eye adaptation *(new)*
**Verify:** Move from a dark interior to bright daylight — perceived brightness gradually adapts over roughly a second instead of snapping instantly.
**Modern games:** Simulates eyes adjusting; avoids blown-out or crushed scenes on sudden lighting changes. Distinct from tonemapping's fixed response curve.

### Lens/camera artifacts (chromatic aberration, flares, vignette, grain) *(new)*
**Verify:** Toggle each independently — visible but subtle at default settings, doesn't dominate the image.
**Modern games:** Deliberate cinematic "imperfections" used for mood and style.

---

## Physics

### Rigid body dynamics
**Verify:** Objects fall, collide, and come to rest realistically.
**Modern games:** Basic physical object interaction.

### Character controller (capsule collision, slopes, steps)
**Verify:** Character moves smoothly over stairs/slopes without getting stuck.
**Modern games:** Core movement feel.

### Vehicle physics (if applicable)
**Verify:** Vehicle handles suspension, traction, and collision believably.
**Modern games:** Driving/riding mechanics.

### Destructible/breakable objects
**Verify:** Trigger destruction — pieces separate and simulate physically, not just disappear.
**Modern games:** Breakable crates, walls, props.

### Constraint/joint systems *(new)*
**Verify:** Rig a rope, hinge, or chain — connected objects move together correctly and break under a defined force threshold.
**Modern games:** Swinging bridges, hinged doors, breakable chains.

### Networked/deterministic physics *(new)*
**Verify:** Run identical simulation input on two machines — results match (bit-for-bit, or within an agreed tolerance) frame after frame.
**Modern games:** Required for lockstep/rollback multiplayer and reliable replay systems.

---

## AI & Navigation *(new category)*

### Navmesh generation & pathfinding
**Verify:** Bake a navmesh over complex terrain/props, an agent paths around obstacles to a distant goal without getting stuck.
**Modern games:** NPCs navigating open worlds and dungeons intelligently.

### Behavior trees / utility AI
**Verify:** An NPC switches behavior (patrol → alert → combat → flee) based on world state, not a fixed scripted sequence.
**Modern games:** Enemies that react believably instead of following one canned pattern.

### Perception system (sight/hearing/awareness)
**Verify:** NPC only reacts to the player within a line-of-sight cone and noise radius, not omniscient tracking through walls.
**Modern games:** Stealth mechanics, guard alert states.

### Crowd/flocking simulation
**Verify:** A large group of agents moves as a cohesive group, avoiding collisions with each other and the player.
**Modern games:** City crowds, battle formations, animal herds.

### Dynamic/runtime navmesh updates
**Verify:** Destroy a bridge or drop an obstacle mid-session — agents reroute without a manual navmesh rebake.
**Modern games:** AI that keeps functioning correctly in destructible environments.

---

## Audio

### 3D positional audio with occlusion/reverb zones
**Verify:** Sound source direction/distance perceivable, muffled correctly behind walls.
**Modern games:** Spatial audio immersion.

### Dynamic music system (layered/adaptive)
**Verify:** Music intensity changes with gameplay state (combat vs. exploration) without hard cuts.
**Modern games:** Adaptive game scoring.

### Procedural/surface-aware footstep & foley *(new)*
**Verify:** Walk across different surfaces (stone, grass, metal, water) — footstep and movement sounds change automatically to match.
**Modern games:** Grounded, material-correct movement audio instead of one generic loop.

### Dialogue/VO system with lip-sync *(new)*
**Verify:** Play a VO line — mouth shapes match phonemes closely enough to read as speech, not generic flapping.
**Modern games:** Cutscene and in-world dialogue that reads as performed, not dubbed over.

### Dynamic mixing & voice prioritization *(new)*
**Verify:** Trigger far more simultaneous sound sources than output channels allow — the engine culls/prioritizes by distance and importance instead of clipping or distorting.
**Modern games:** Large battles stay clear instead of turning into audio mush.

### Audio streaming & compression pipeline *(new)*
**Verify:** Profile memory — VO/music for the whole game isn't resident in RAM at once, streams in alongside level content.
**Modern games:** The same streaming logic already applied to textures and meshes, applied to audio.

---

## Gameplay Framework & Scripting *(new category)*

### Entity/actor-component architecture
**Verify:** Assemble a new gameplay object from existing reusable components, no engine recompile needed.
**Modern games:** Fast iteration on enemy types, items, and interactable objects.

### Designer-facing scripting layer
**Verify:** A non-programmer wires up a quest trigger or ability using visual scripting or an embedded language, without touching engine source.
**Modern games:** Blueprint-style or Lua/Python-driven gameplay logic layered over the core engine.

### Event/messaging system
**Verify:** One system (a switch, a death) triggers reactions in unrelated systems (doors, AI, audio, UI) with no direct references between them.
**Modern games:** Gameplay logic that stays maintainable as systems multiply.

### Save/load & game-state serialization
**Verify:** Save mid-session, reload — world state (positions, quest flags, inventory, AI state) restores exactly.
**Modern games:** Checkpoint saves, quicksave/quickload without state corruption.

---

## Networking & Multiplayer *(new category — if targeting online/co-op)*

### Client-server replication
**Verify:** Change object state on the server — it propagates correctly to all connected clients within a frame or two.
**Modern games:** Foundation for any online mode, co-op or competitive.

### Client-side prediction & server reconciliation
**Verify:** Local input feels instant; server disagreement corrects silently without visible rubber-banding under normal latency.
**Modern games:** Responsive-feeling movement despite real network delay.

### Network interest management / relevancy
**Verify:** Profile bandwidth per client — only nearby/relevant entities sync, not full world state to everyone.
**Modern games:** Large-player-count sessions without bandwidth collapse.

### Lag compensation / rollback
**Verify:** Hit registration favors what the attacking client actually saw on their screen, accounting for their ping.
**Modern games:** Shooters and fighters that feel fair across a range of connection qualities.

### Server-authoritative validation
**Verify:** Feed the server an impossible client-reported state (teleport, damage spike) — it's rejected or corrected, not trusted blindly.
**Modern games:** Baseline cheat resistance in any competitive online title.

---

## Input & Camera Systems *(new category)*

### Action-based input mapping with rebinding
**Verify:** Reassign a control in settings — gameplay responds to the new binding immediately, nothing hardcoded to a specific key/button.
**Modern games:** Full control customization expected at launch, especially on PC.

### Multi-device support with runtime switching
**Verify:** Swap between gamepad and keyboard/mouse mid-session — UI prompts and control scheme update automatically.
**Modern games:** Seamless cross-input play without a restart or menu dive.

### Third-person camera collision & follow logic
**Verify:** Move into a tight space or corner — camera pulls in smoothly without clipping through geometry or losing the character.
**Modern games:** One of the hardest-to-get-right systems in any third-person AAA title.

### Cinematic camera/sequencer blending
**Verify:** Cut or blend from gameplay camera into a scripted cutscene camera with no jarring pop or frame skip.
**Modern games:** In-engine cutscenes that read as directed, not raw gameplay footage.

---

## UI/UX, Localization & Accessibility *(new category)*

### In-game UI framework
**Verify:** Build a new HUD element or menu from reusable widgets, it updates reactively when underlying game state changes.
**Modern games:** Data-driven UI instead of hand-wired, hardcoded screens.

### UI scaling across resolutions & aspect ratios
**Verify:** Same UI on ultrawide, standard 16:9, and split-screen — no clipped, overlapping, or misaligned elements.
**Modern games:** Ship-ready across PC's variable displays and console split-screen.

### Subtitle & closed-caption system
**Verify:** Dialogue and key non-dialogue cues (footsteps, off-screen gunfire direction) surface as readable, positioned captions.
**Modern games:** Increasingly a platform certification requirement, not an optional extra.

### Accessibility options
**Verify:** Toggle a colorblind filter or a difficulty/assist setting — gameplay-critical information stays readable and the game stays completable.
**Modern games:** Expected baseline for major releases: remappable controls, readable UI, adjustable assists.

### Text & audio localization pipeline
**Verify:** Switch language setting — UI text, subtitles, and VO swap together, layout doesn't break on longer translated strings.
**Modern games:** Simultaneous worldwide launch across a dozen-plus languages.

---

## Platform & Live Services *(new category)*

### Platform SDK integration
**Verify:** Unlock an achievement/trophy and save progress — both show correctly in the platform's own overlay/UI, cloud save restores on a different device.
**Modern games:** Baseline certification requirement on Steam/PlayStation/Xbox.

### Storefront/DRM integration
**Verify:** Build passes platform entitlement/licensing checks in a certification/compliance test pass.
**Modern games:** Required to ship on any major storefront.

### Patch & live content delivery
**Verify:** Push a hotfix or seasonal content update — client downloads a delta patch, not a full reinstall.
**Modern games:** Live-service update cadence without forcing a full redownload every time.

### Analytics/telemetry hooks
**Verify:** Gameplay events (deaths, drop-off points, purchases) stream to a dashboard usable for real balance decisions.
**Modern games:** Data-informed balance patches and content decisions post-launch.

---

## Tools & Pipeline

### In-engine editor with live preview
**Verify:** Edit a scene, see changes without restarting the game.
**Modern games:** Core dev workflow for any studio.

### Shader/asset hot-reload
**Verify:** Edit a shader/asset file, see it update in-game without a rebuild.
**Modern games:** Fast iteration for artists/engineers.

### GPU profiling integration (PIX/RenderDoc/Nsight markers)
**Verify:** Capture a frame, see named passes/markers in the profiler, not opaque draw calls.
**Modern games:** Essential for performance tuning at scale.

### Asset import pipeline (gltf/fbx, texture compression, mesh optimization)
**Verify:** Import a raw asset, confirm compressed textures (BCn) and optimized mesh/LODs generated automatically.
**Modern games:** Efficient content pipeline from DCC tools to runtime.

### Build system supporting multiple platforms
**Verify:** Build succeeds and runs on each target platform without manual per-platform hacks.
**Modern games:** Cross-platform shipping requirement.

### Version control for large binary assets *(new)*
**Verify:** Multiple artists lock/check out large binary files (textures, meshes) concurrently without silent overwrites.
**Modern games:** Perforce-style workflows standard at AAA scale — git alone struggles on multi-terabyte asset repos.

### Automated build & regression testing (CI) *(new)*
**Verify:** A code change triggers an automatic build plus smoke test; breakages get caught before they reach the whole team.
**Modern games:** Keeps a large, fast-moving codebase shippable day to day.

### Cinematic/sequencer tool *(new)*
**Verify:** Author a multi-camera cutscene on a timeline in-editor, scrub and preview without a separate export step.
**Modern games:** In-engine cutscene direction — camera cuts, animation tracks, audio, effects — authored by hand instead of baked video.

### Node-based VFX/shader authoring tool *(new)*
**Verify:** An artist builds or edits a particle effect or shader graph visually with a live preview, no code compile step required.
**Modern games:** Artist-driven iteration speed (Niagara/shader-graph-style tools).

---

## Performance & Stability

### PSO/shader pre-warming
**Verify:** New material/shader variants entering view cause no stutter.
**Modern games:** Avoids the infamous "shader compilation stutter" on PC.

### Async compute utilization
**Verify:** GPU timeline shows compute and graphics queues overlapping, not serialized.
**Modern games:** Extra performance headroom on modern GPUs.

### Multithreaded render command recording
**Verify:** CPU frame time scales across cores, not bottlenecked on a single render thread.
**Modern games:** CPU-side scalability for complex scenes.

### Stable frame pacing under dynamic load
**Verify:** Spawn many entities/effects at once — frame time stays consistent, no spikes.
**Modern games:** Smooth feel during combat/crowd-heavy moments.

### Crash reporting & telemetry pipeline *(new)*
**Verify:** Force a crash — a symbolicated report with stack trace and repro context reaches the team automatically.
**Modern games:** How live titles actually find and fix the crashes players hit, not just the ones QA reproduces.

### Memory budgeting & tracking tools *(new)*
**Verify:** Per-system memory budgets (textures, audio, animation, etc.) are visible live and flag when a system exceeds its budget.
**Modern games:** Catching memory bloat before it becomes a cert-failing footprint or a console crash.
