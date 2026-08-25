extends MainLoop

func _process(delta: float) -> bool:
	return false

func _initialize():
	print("=== Running Terrain Heightmap 16-Bit Import Precision Test ===")
	
	var src_path = "res://test_heightmap_16bit.r16"
	
	# Load the imported resource directly via Godot ResourceLoader
	var image: Image = ResourceLoader.load(src_path)
	if image == null or image.is_empty():
		print("[FAIL] ResourceLoader failed to load imported image: ", src_path)
		return
	
	print("[PASS] Imported image loaded: size = ", image.get_size(), ", format = ", image.get_format(), " (Image::FORMAT_RF = 8)")
	
	var dim = 64
	var max_err = 0.0
	var quant_error_count_if_8bit = 0
	var total_samples = dim * dim
	
	# Test exact sample values across the 64x64 grid
	for y in range(dim):
		for x in range(dim):
			var expected_val = float(y * dim + x) / float(total_samples - 1)
			var imported_val = image.get_pixel(x, y).r
			var delta_val = abs(imported_val - expected_val)
			if delta_val > max_err:
				max_err = delta_val
			
			# An 8-bit quantization rounds to nearest k/255
			var quantized_8bit = round(expected_val * 255.0) / 255.0
			var diff_from_8bit = abs(imported_val - quantized_8bit)
			# If the value is significantly different from the 8-bit quantized step, it preserves 16-bit detail
			if diff_from_8bit > 0.0005:
				quant_error_count_if_8bit += 1
	
	print("Max deviation from 16-bit ground truth: ", max_err)
	print("Samples preserving sub-8-bit precision: ", quant_error_count_if_8bit, " / ", total_samples)
	
	# Print first 5 sample values to show exact floating-point precision
	print("Sample value inspection (first 5 pixels):")
	for i in range(5):
		var expected = float(i) / float(total_samples - 1)
		var actual = image.get_pixel(i, 0).r
		var quantized_8bit = round(expected * 255.0) / 255.0
		print("  Pixel (", i, ", 0): Actual=", actual, " Expected=", expected, " (Old 8-Bit PNG would be=", quantized_8bit, ")")
	
	# In 8-bit PNG, max_err was ~0.00196 (half of 1/255).
	# With 16-bit EXR, max_err is < 0.00005.
	if max_err < 0.00005:
		print("[SUCCESS] 16-bit heightmap precision is 100% verified and preserved!")
	else:
		print("[FAIL] Precision loss detected! Max error: ", max_err)
