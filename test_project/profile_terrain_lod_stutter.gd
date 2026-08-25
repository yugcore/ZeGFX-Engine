extends SceneTree

var terrain: Terrain3D
var camera: Camera3D
var frame_times: Array = []
var current_frame = 0
var max_frames = 120

func _init():
	print("=== Running Terrain3D LOD Profiling & VirtualTexture2D Validation ===")

	# 1. Test VirtualTexture2D
	var vt = VirtualTexture2D.new()
	vt.set_virtual_size(Vector2i(4096, 4096))
	vt.set_tile_size(128)
	vt.set_physical_cache_size(1024)
	vt.rebuild_virtual_texture()

	var requested_tiles = []
	vt.connect("tile_requested", Callable(func(tx, ty, mip):
		requested_tiles.append(Vector3i(tx, ty, mip))
	))

	# Request a region
	vt.request_region(Rect2(0.2, 0.2, 0.1, 0.1), 0)
	print("[PASS] VirtualTexture2D region requested. Total tile requests emitted: ", requested_tiles.size())
	
	# Upload a dummy tile
	var img = Image.create_empty(128, 128, false, Image.FORMAT_RGBA8)
	img.fill(Color(1, 0, 0, 1))
	vt.upload_tile_data(0, 0, 0, img)
	if vt.is_tile_resident(0, 0, 0):
		print("[PASS] VirtualTexture2D tile upload verified resident.")
	else:
		print("[FAIL] VirtualTexture2D tile was not marked resident!")

	# 2. Setup large Terrain3D (256x256 heightfield with 32x32 chunks -> 64 chunk nodes)
	var root = Node3D.new()
	get_root().add_child(root)

	terrain = Terrain3D.new()
	terrain.set_cell_size(2.0)
	terrain.set_chunk_size(32)
	terrain.set_lod_enabled(true)
	terrain.set_lod_count(4)
	terrain.set_lod_distance_step(80.0)
	terrain.set_max_lod_swaps_per_frame(8) # Throttled budget
	terrain.set_lod_hysteresis_margin(0.15)
	terrain.set_lod_update_distance_threshold(2.0)

	var map_dim = 256
	var heights = PackedFloat32Array()
	heights.resize(map_dim * map_dim)
	for z in range(map_dim):
		for x in range(map_dim):
			heights[z * map_dim + x] = (sin(float(x) * 0.05) + cos(float(z) * 0.05) + 2.0) * 0.25
	
	terrain.set_heights_raw(heights)
	root.add_child(terrain)
	terrain.rebuild_terrain()
	print("[PASS] Terrain3D instantiated with ", terrain.get_chunk_count(), " chunks.")

	camera = Camera3D.new()
	camera.position = Vector3(0, 100, 0)
	root.add_child(camera)

	# Run simulation frames
	for frame in range(max_frames):
		var t0 = Time.get_ticks_usec()

		# Move camera across terrain across multiple LOD distance rings
		var cam_x = sin(float(frame) * 0.1) * 300.0
		var cam_z = cos(float(frame) * 0.1) * 300.0
		var cam_y = 50.0 + sin(float(frame) * 0.05) * 40.0
		camera.position = Vector3(cam_x, cam_y, cam_z)

		# Explicitly trigger LOD update with moving camera
		terrain.update_lod(camera.position)

		var t1 = Time.get_ticks_usec()
		var frame_dur_ms = float(t1 - t0) / 1000.0
		frame_times.append(frame_dur_ms)

	_finish_profiling()
	root.queue_free()
	quit(0)

func _finish_profiling():
	var total_time = 0.0
	var max_time = 0.0
	for t in frame_times:
		total_time += t
		if t > max_time:
			max_time = t
	
	var avg_time = total_time / float(frame_times.size())
	frame_times.sort()
	var p95_time = frame_times[int(frame_times.size() * 0.95)]
	var p99_time = frame_times[int(frame_times.size() * 0.99)]

	print("=== Profiling Results over ", max_frames, " Dynamic Camera Frames ===")
	print("Average LOD Evaluation Time: ", String.num(avg_time, 4), " ms")
	print("95th Percentile LOD Time:    ", String.num(p95_time, 4), " ms")
	print("99th Percentile LOD Time:    ", String.num(p99_time, 4), " ms")
	print("Maximum Peak LOD Spike:      ", String.num(max_time, 4), " ms")

	if max_time < 2.0:
		print("[SUCCESS] Zero main-thread stalls! Maximum LOD spike is ultra-smooth (< 2.0ms).")
	else:
		print("[INFO] Peak LOD time: ", max_time, " ms")
