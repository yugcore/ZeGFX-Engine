/**************************************************************************/
/*  resource_importer_terrain_heightmap.cpp                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "resource_importer_terrain_heightmap.h"

#include "core/io/file_access.h"
#include "core/io/image.h"

String ResourceImporterTerrainHeightmap::get_importer_name() const {
	return "terrain_heightmap";
}

String ResourceImporterTerrainHeightmap::get_visible_name() const {
	return "Terrain Heightmap (16-bit Linear)";
}

void ResourceImporterTerrainHeightmap::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("r16");
	p_extensions->push_back("raw");
	p_extensions->push_back("r8");
	p_extensions->push_back("hdr");
}

String ResourceImporterTerrainHeightmap::get_save_extension() const {
	return "image";
}

String ResourceImporterTerrainHeightmap::get_resource_type() const {
	return "Image";
}

bool ResourceImporterTerrainHeightmap::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

int ResourceImporterTerrainHeightmap::get_preset_count() const {
	return 0;
}

String ResourceImporterTerrainHeightmap::get_preset_name(int p_idx) const {
	return String();
}

void ResourceImporterTerrainHeightmap::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "bit_depth", PROPERTY_HINT_ENUM, "Auto-Detect,8-Bit (R8),16-Bit (R16),32-Bit Float"), BIT_DEPTH_AUTO));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "raw_width", PROPERTY_HINT_RANGE, "2,8192,1"), 513));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "raw_height", PROPERTY_HINT_RANGE, "2,8192,1"), 513));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "normalize_heights"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "invert_y"), false));
}

Error ResourceImporterTerrainHeightmap::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Ref<FileAccess> f = FileAccess::open(p_source_file, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, "Cannot open heightmap file '" + p_source_file + "'.");

	uint64_t file_len = f->get_length();
	String ext = p_source_file.get_extension().to_lower();

	int bit_depth_opt = p_options["bit_depth"];
	int width_opt = p_options["raw_width"];
	int height_opt = p_options["raw_height"];
	bool normalize = p_options["normalize_heights"];
	bool invert_y = p_options["invert_y"];

	int bytes_per_sample = 2;
	if (bit_depth_opt == BIT_DEPTH_8 || ext == "r8") {
		bytes_per_sample = 1;
	} else if (bit_depth_opt == BIT_DEPTH_32F || ext == "hdr") {
		bytes_per_sample = 4;
	} else if (bit_depth_opt == BIT_DEPTH_16 || ext == "r16") {
		bytes_per_sample = 2;
	} else {
		// Auto-detect
		if (file_len % 2 == 0) {
			bytes_per_sample = 2;
		} else {
			bytes_per_sample = 1;
		}
	}

	int sample_count = file_len / bytes_per_sample;
	int width = width_opt;
	int height = height_opt;

	int square_dim = (int)Math::sqrt((double)sample_count);
	if (square_dim * square_dim == sample_count) {
		width = square_dim;
		height = square_dim;
	}

	Ref<Image> image = Image::create_empty(width, height, false, Image::FORMAT_RF);
	ERR_FAIL_COND_V_MSG(image.is_null() || image->is_empty(), ERR_OUT_OF_MEMORY, "Failed to allocate image for heightmap.");

	Vector<uint8_t> img_data = image->get_data();
	float *dst_floats = (float *)img_data.ptrw();

	float min_v = 1e9f;
	float max_v = -1e9f;

	for (int y = 0; y < height; ++y) {
		int target_y = invert_y ? (height - 1 - y) : y;
		for (int x = 0; x < width; ++x) {
			float val = 0.0f;
			if (bytes_per_sample == 1) {
				val = (float)f->get_8() / 255.0f;
			} else if (bytes_per_sample == 2) {
				val = (float)f->get_16() / 65535.0f;
			} else {
				val = f->get_float();
			}

			dst_floats[target_y * width + x] = val;
			min_v = MIN(min_v, val);
			max_v = MAX(max_v, val);
		}
	}

	if (normalize && (max_v > min_v)) {
		float range = max_v - min_v;
		for (int i = 0; i < width * height; ++i) {
			dst_floats[i] = (dst_floats[i] - min_v) / range;
		}
	}

	image->set_data(width, height, false, Image::FORMAT_RF, img_data);

	// Save to GDIM image format
	Ref<FileAccess> out = FileAccess::open(p_save_path + ".image", FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(out.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_save_path + ".image'.");

	const uint8_t header[4] = { 'G', 'D', 'I', 'M' };
	out->store_buffer(header, 4);
	out->store_pascal_string("png"); // save PNG extension format
	Vector<uint8_t> png_bytes = image->save_png_to_buffer();
	out->store_buffer(png_bytes.ptr(), png_bytes.size());

	return OK;
}
