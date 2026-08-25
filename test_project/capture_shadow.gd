extends SceneTree

var frame_count = 0
var camera: Camera3D
var light: DirectionalLight3D
var ground_mesh: MeshInstance3D
var box_mesh: MeshInstance3D
var sphere_mesh: MeshInstance3D
var root_node: Node3D

func _init():
	root_node = Node3D.new()
	root.add_child(root_node)
	
	# Camera
	camera = Camera3D.new()
	camera.position = Vector3(0, 4, 8)
	camera.look_at(Vector3(0, 1, 0), Vector3.UP)
	camera.current = true
	root_node.add_child(camera)
	
	# Environment
	var env = Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.6, 0.7, 0.8)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.2, 0.2, 0.25)
	var world_env = WorldEnvironment.new()
	world_env.environment = env
	root_node.add_child(world_env)
	
	# Light
	light = DirectionalLight3D.new()
	light.position = Vector3(5, 10, 5)
	light.rotation_degrees = Vector3(-45, 45, 0)
	light.shadow_enabled = true
	light.light_energy = 1.2
	root_node.add_child(light)
	
	# White ground plane
	ground_mesh = MeshInstance3D.new()
	var plane = PlaneMesh.new()
	plane.size = Vector2(30, 30)
	ground_mesh.mesh = plane
	var mat_ground = StandardMaterial3D.new()
	mat_ground.albedo_color = Color(0.9, 0.9, 0.9)
	mat_ground.roughness = 0.5
	ground_mesh.material_override = mat_ground
	root_node.add_child(ground_mesh)
	
	# Box casting shadow
	box_mesh = MeshInstance3D.new()
	var box = BoxMesh.new()
	box.size = Vector3(2, 2, 2)
	box_mesh.mesh = box
	box_mesh.position = Vector3(-2, 1, 0)
	var mat_box = StandardMaterial3D.new()
	mat_box.albedo_color = Color(0.8, 0.2, 0.2)
	box_mesh.material_override = mat_box
	box_mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	root_node.add_child(box_mesh)
	
	# Sphere casting shadow
	sphere_mesh = MeshInstance3D.new()
	var sphere = SphereMesh.new()
	sphere.radius = 1.0
	sphere.height = 2.0
	sphere_mesh.mesh = sphere
	sphere_mesh.position = Vector3(2, 1, 0)
	var mat_sphere = StandardMaterial3D.new()
	mat_sphere.albedo_color = Color(0.2, 0.4, 0.8)
	sphere_mesh.material_override = mat_sphere
	sphere_mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	root_node.add_child(sphere_mesh)
	
	print("[Capture] Scene setup complete.")

func _process(delta: float) -> bool:
	frame_count += 1
	if frame_count == 10:
		var img = root.get_viewport().get_texture().get_image()
		if img != null and not img.is_empty():
			var out_path = "res://shadow_render.png"
			img.save_png(out_path)
			print("[Capture] Successfully saved screenshot to: ", out_path)
		else:
			print("[Capture] Error: Viewport image was null/empty.")
		quit(0)
		return true
	return false
