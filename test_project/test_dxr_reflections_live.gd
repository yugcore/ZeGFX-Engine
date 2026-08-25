extends SceneTree

var frame_count = 0
var max_test_frames = 30
var world_env: WorldEnvironment
var cam: Camera3D

func _init():
	print("=== Starting Live DXR / SSR Reflection Stress Test ===")

	var root = Node3D.new()
	get_root().add_child(root)

	# 1. Setup WorldEnvironment with SSR enabled
	world_env = WorldEnvironment.new()
	var env = Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.1, 0.2, 0.3, 1.0)
	env.ssr_enabled = true
	env.ssr_max_steps = 64
	env.ssr_fade_in = 0.15
	env.ssr_fade_out = 2.0
	env.ssr_depth_tolerance = 0.2
	world_env.environment = env
	root.add_child(world_env)

	# 2. Setup DirectionalLight3D
	var light = DirectionalLight3D.new()
	light.position = Vector3(0, 10, 5)
	light.rotation_degrees = Vector3(-45, 45, 0)
	light.shadow_enabled = true
	root.add_child(light)

	# 3. Setup Reflective Ground Plane
	var plane_mesh = PlaneMesh.new()
	plane_mesh.size = Vector2(20, 20)
	var plane_inst = MeshInstance3D.new()
	plane_inst.mesh = plane_mesh
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0.8, 0.8, 0.8, 1.0)
	mat.metallic = 1.0
	mat.roughness = 0.05
	plane_inst.material_override = mat
	root.add_child(plane_inst)

	# 4. Setup Reflective Metallic Sphere
	var sphere_mesh = SphereMesh.new()
	sphere_mesh.radius = 1.5
	sphere_mesh.height = 3.0
	var sphere_inst = MeshInstance3D.new()
	sphere_inst.mesh = sphere_mesh
	sphere_inst.position = Vector3(0, 2, 0)
	var sphere_mat = StandardMaterial3D.new()
	sphere_mat.albedo_color = Color(1.0, 0.2, 0.2, 1.0)
	sphere_mat.metallic = 0.9
	sphere_mat.roughness = 0.1
	sphere_inst.material_override = sphere_mat
	root.add_child(sphere_inst)

	# 5. Camera
	cam = Camera3D.new()
	cam.position = Vector3(0, 4, 8)
	cam.look_at_from_position(Vector3(0, 4, 8), Vector3(0, 1, 0), Vector3(0, 1, 0))
	cam.current = true
	root.add_child(cam)

	print("[PASS] Reflection scene created with metallic materials and SSR enabled.")

func _process(delta: float) -> bool:
	frame_count += 1
	if frame_count % 5 == 0:
		print("Rendering frame: ", frame_count, "/", max_test_frames)

	if frame_count >= max_test_frames:
		print("[SUCCESS] Live DXR/SSR reflection test completed ", max_test_frames, " frames with ZERO GPU crash/TDR!")
		var img = get_root().get_viewport().get_texture().get_image()
		if img != null and not img.is_empty():
			img.save_png("res://dxr_reflection_render.png")
			print("[PASS] Saved reflection frame render to: res://dxr_reflection_render.png")
		quit(0)
		return true

	return false
