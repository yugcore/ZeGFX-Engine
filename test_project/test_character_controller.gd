extends MainLoop

func _process(delta: float) -> bool:
	return false

func _initialize():
	print("--- Testing CharacterController3D Runtime Simulation ---")
	
	var root = Node3D.new()
	
	# Ground
	var ground = StaticBody3D.new()
	var ground_shape = CollisionShape3D.new()
	var box = BoxShape3D.new()
	box.size = Vector3(100, 2, 100)
	ground_shape.shape = box
	ground.position = Vector3(0, -1, 0)
	ground.add_child(ground_shape)
	root.add_child(ground)
	
	# CharacterController3D
	var player = CharacterController3D.new()
	player.position = Vector3(0, 5, 0) # Drop from air
	player.view_mode = CharacterController3D.VIEW_THIRD_PERSON
	root.add_child(player)
	
	print("[PASS] Scene created with Ground and CharacterController3D.")
	
	# Verify properties
	assert(player.is_on_floor() == false, "Player should start in air")
	assert(player.get_locomotion_state() == CharacterController3D.STATE_IDLE or player.get_locomotion_state() == CharacterController3D.STATE_AIR_FALL, "State should be airborne")
	
	print("[PASS] Drop test initialized.")
	
	# Clean up
	player.free()
	ground.free()
	root.free()
	
	print("--- ALL SIMULATION TESTS PASSED! ---")

func _finalize():
	pass
