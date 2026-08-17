/* clang-format off */
#[vertex]

#version 450

#VERSION_DEFINES

#include "blur_raster_inc.glsl"

layout(location = 0) out vec2 uv_interp;
/* clang-format on */

void main() {
	// old code, ARM driver bug on Mali-GXXx GPUs and Vulkan API 1.3.xxx
	// https://github.com/godotengine/godot/pull/92817#issuecomment-2168625982
	//vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	//gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
	//uv_interp = clamp(gl_Position.xy, vec2(0.0, 0.0), vec2(1.0, 1.0)) * 2.0; // saturate(x) * 2.0

	vec2 vertex_base;
	if (gl_VertexIndex == 0) {
		vertex_base = vec2(-1.0, -1.0);
	} else if (gl_VertexIndex == 1) {
		vertex_base = vec2(-1.0, 3.0);
	} else {
		vertex_base = vec2(3.0, -1.0);
	}
	gl_Position = vec4(vertex_base, 0.0, 1.0);
	uv_interp = clamp(vertex_base, vec2(0.0, 0.0), vec2(1.0, 1.0)) * 2.0; // saturate(x) * 2.0
}

/* clang-format off */
#[fragment]

#version 450

#VERSION_DEFINES

#include "blur_raster_inc.glsl"

layout(location = 0) in vec2 uv_interp;
/* clang-format on */

layout(set = 0, binding = 0) uniform sampler2D source_color;

#ifdef MODE_GLOW_UPSAMPLE
// When upsampling this is original downsampled texture, not the blended upsampled texture.
layout(set = 1, binding = 0) uniform sampler2D blend_color;
layout(constant_id = 0) const bool use_debanding = false;
layout(constant_id = 1) const bool use_blend_color = false;

// From https://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
// and https://www.shadertoy.com/view/MslGR8 (5th one starting from the bottom)
// NOTE: `frag_coord` is in pixels (i.e. not normalized UV).
// This dithering must be applied after encoding changes (linear/nonlinear) have been applied
// as the final step before quantization from floating point to integer values.
vec3 screen_space_dither(vec2 frag_coord, float bit_alignment_diviser) {
	// Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR.
	// Removed the time component to avoid passing time into this shader.
	vec3 dither = vec3(dot(vec2(171.0, 231.0), frag_coord));
	dither.rgb = fract(dither.rgb / vec3(103.0, 71.0, 97.0));

	// Subtract 0.5 to avoid slightly brightening the whole viewport.
	// Use a dither strength of 100% rather than the 37.5% suggested by the original source.
	return (dither.rgb - 0.5) / bit_alignment_diviser;
}
#endif

layout(location = 0) out vec4 frag_color;

#ifdef MODE_GLOW_DOWNSAMPLE

float luma(vec3 color) {
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float karis_weight(vec3 color) {
	return 1.0 / (1.0 + luma(color));
}

// 13-Tap Dual-Filter Downsampling (Karis average with sub-pixel firefly suppression)
vec4 BloomDownKernel13(sampler2D Tex, vec2 uv0) {
	vec2 RcpSrcTexRes = blur.source_pixel_size;
	vec2 uv = (uv0 * 2.0 + 1.0) * RcpSrcTexRes;

	// 13-tap layout: 4 corners, 4 cardinals, 4 inner diagonals, 1 center
	vec4 A = textureLod(Tex, uv + vec2(-2.0, -2.0) * RcpSrcTexRes, 0.0);
	vec4 B = textureLod(Tex, uv + vec2( 0.0, -2.0) * RcpSrcTexRes, 0.0);
	vec4 C = textureLod(Tex, uv + vec2( 2.0, -2.0) * RcpSrcTexRes, 0.0);

	vec4 D = textureLod(Tex, uv + vec2(-1.0, -1.0) * RcpSrcTexRes, 0.0);
	vec4 E = textureLod(Tex, uv + vec2( 1.0, -1.0) * RcpSrcTexRes, 0.0);

	vec4 F = textureLod(Tex, uv + vec2(-2.0,  0.0) * RcpSrcTexRes, 0.0);
	vec4 G = textureLod(Tex, uv, 0.0);
	vec4 H = textureLod(Tex, uv + vec2( 2.0,  0.0) * RcpSrcTexRes, 0.0);

	vec4 I = textureLod(Tex, uv + vec2(-1.0,  1.0) * RcpSrcTexRes, 0.0);
	vec4 J = textureLod(Tex, uv + vec2( 1.0,  1.0) * RcpSrcTexRes, 0.0);

	vec4 K = textureLod(Tex, uv + vec2(-2.0,  2.0) * RcpSrcTexRes, 0.0);
	vec4 L = textureLod(Tex, uv + vec2( 0.0,  2.0) * RcpSrcTexRes, 0.0);
	vec4 M = textureLod(Tex, uv + vec2( 2.0,  2.0) * RcpSrcTexRes, 0.0);

	// 5 overlapping 2x2 bilinear groups with Karis weighting
	vec4 quad1 = (A + B + F + G) * 0.25;
	vec4 quad2 = (B + C + G + H) * 0.25;
	vec4 quad3 = (F + G + K + L) * 0.25;
	vec4 quad4 = (G + H + L + M) * 0.25;
	vec4 quad5 = (D + E + I + J) * 0.25;

	float w1 = karis_weight(quad1.rgb) * 0.125;
	float w2 = karis_weight(quad2.rgb) * 0.125;
	float w3 = karis_weight(quad3.rgb) * 0.125;
	float w4 = karis_weight(quad4.rgb) * 0.125;
	float w5 = karis_weight(quad5.rgb) * 0.5;

	float total_weight = w1 + w2 + w3 + w4 + w5;
	return (quad1 * w1 + quad2 * w2 + quad3 * w3 + quad4 * w4 + quad5 * w5) / max(1e-5, total_weight);
}

#endif

#ifdef MODE_GLOW_UPSAMPLE

// 9-Tap 3x3 Tent Upsampling Filter (Smooth continuous energy dispersion)
vec4 BloomUpKernel9(sampler2D Tex, vec2 uv0) {
	vec2 RcpSrcTexRes = blur.source_pixel_size;
	vec2 uv = (uv0 * 0.5 + 0.5) * blur.dest_pixel_size;

	vec2 d = RcpSrcTexRes * 1.0;

	vec4 c = vec4(0.0);
	// 4 corner samples (weight 1/16)
	c += textureLod(Tex, uv + vec2(-d.x, -d.y), 0.0) * (1.0 / 16.0);
	c += textureLod(Tex, uv + vec2( d.x, -d.y), 0.0) * (1.0 / 16.0);
	c += textureLod(Tex, uv + vec2(-d.x,  d.y), 0.0) * (1.0 / 16.0);
	c += textureLod(Tex, uv + vec2( d.x,  d.y), 0.0) * (1.0 / 16.0);

	// 4 cardinal samples (weight 2/16)
	c += textureLod(Tex, uv + vec2(  0.0, -d.y), 0.0) * (2.0 / 16.0);
	c += textureLod(Tex, uv + vec2(-d.x,   0.0), 0.0) * (2.0 / 16.0);
	c += textureLod(Tex, uv + vec2( d.x,   0.0), 0.0) * (2.0 / 16.0);
	c += textureLod(Tex, uv + vec2(  0.0,  d.y), 0.0) * (2.0 / 16.0);

	// Center sample (weight 4/16)
	c += textureLod(Tex, uv, 0.0) * (4.0 / 16.0);

	return c;
}

#endif // MODE_GLOW_UPSAMPLE

void main() {
	// We do not apply our color scale for our mobile renderer here, we'll leave our colors at half brightness and apply scale in the tonemap raster.

#ifdef MODE_MIPMAP

	vec2 pix_size = blur.dest_pixel_size;
	vec4 color = texture(source_color, uv_interp + vec2(-0.5, -0.5) * pix_size);
	color += texture(source_color, uv_interp + vec2(0.5, -0.5) * pix_size);
	color += texture(source_color, uv_interp + vec2(0.5, 0.5) * pix_size);
	color += texture(source_color, uv_interp + vec2(-0.5, 0.5) * pix_size);
	frag_color = color / 4.0;

#endif

#ifdef MODE_GAUSSIAN_BLUR

	// For Gaussian Blur we use 13 taps in a single pass instead of 12 taps over 2 passes.
	// This minimizes the number of times we change framebuffers which is very important for mobile.
	// Source: http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
	vec4 A = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(-1.0, -1.0));
	vec4 B = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(0.0, -1.0));
	vec4 C = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(1.0, -1.0));
	vec4 D = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(-0.5, -0.5));
	vec4 E = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(0.5, -0.5));
	vec4 F = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(-1.0, 0.0));
	vec4 G = texture(source_color, uv_interp);
	vec4 H = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(1.0, 0.0));
	vec4 I = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(-0.5, 0.5));
	vec4 J = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(0.5, 0.5));
	vec4 K = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(-1.0, 1.0));
	vec4 L = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(0.0, 1.0));
	vec4 M = texture(source_color, uv_interp + blur.dest_pixel_size * vec2(1.0, 1.0));

	float base_weight = 0.5 / 4.0;
	float lesser_weight = 0.125 / 4.0;

	frag_color = (D + E + I + J) * base_weight;
	frag_color += (A + B + G + F) * lesser_weight;
	frag_color += (B + C + H + G) * lesser_weight;
	frag_color += (F + G + L + K) * lesser_weight;
	frag_color += (G + H + M + L) * lesser_weight;
#endif

#ifdef MODE_GLOW_GATHER
	// First step, go straight to quarter resolution.
	// Don't apply blur, but include thresholding.

	vec2 block_pos = floor(gl_FragCoord.xy) * 4.0;
	vec2 end = max(1.0 / blur.source_pixel_size - vec2(4.0), vec2(0.0));
	block_pos = clamp(block_pos, vec2(0.0), end);

	// We skipped a level, so gather 16 closest samples now.

	vec4 color = textureLod(source_color, (block_pos + vec2(1.0, 1.0)) * blur.source_pixel_size, 0.0);
	color += textureLod(source_color, (block_pos + vec2(1.0, 3.0)) * blur.source_pixel_size, 0.0);
	color += textureLod(source_color, (block_pos + vec2(3.0, 1.0)) * blur.source_pixel_size, 0.0);
	color += textureLod(source_color, (block_pos + vec2(3.0, 3.0)) * blur.source_pixel_size, 0.0);
	frag_color = color * 0.25;

	// Apply strength a second time since it usually gets added at each level.
	frag_color *= blur.glow_strength;
	frag_color *= blur.glow_strength;

	// In the first pass bring back to correct color range else we're applying the wrong threshold
	// in subsequent passes we can use it as is as we'd just be undoing it right after.
	frag_color *= blur.luminance_multiplier;
	frag_color *= blur.glow_exposure;

	float max_value = max(frag_color.r, max(frag_color.g, frag_color.b));
	float feedback = max(smoothstep(blur.glow_hdr_threshold, blur.glow_hdr_threshold + blur.glow_hdr_scale, max_value), blur.glow_bloom);

	frag_color = min(frag_color * feedback, vec4(blur.glow_luminance_cap)) / blur.luminance_multiplier;
#endif // MODE_GLOW_GATHER_WIDE

#ifdef MODE_GLOW_DOWNSAMPLE
	// 13-tap Karis dual-filter downsample with firefly suppression
	frag_color = BloomDownKernel13(source_color, floor(gl_FragCoord.xy));
	frag_color *= blur.glow_strength;
#endif // MODE_GLOW_DOWNSAMPLE

#ifdef MODE_GLOW_UPSAMPLE
	// 9-tap 3x3 tent upsample for smooth optical dispersion
	frag_color = BloomUpKernel9(source_color, floor(gl_FragCoord.xy)) * blur.glow_strength; // "glow_strength" here is actually the glow level. It is always 1.0, except for the first upsample where we need to apply the level to two textures at once.
	if (use_blend_color) {
		vec2 uv = floor(gl_FragCoord.xy) + 0.5;
		frag_color += textureLod(blend_color, uv * blur.dest_pixel_size, 0.0) * blur.glow_level;
	}

	if (use_debanding) {
		frag_color.rgb += screen_space_dither(gl_FragCoord.xy, 1023.0);
	}
#endif // MODE_GLOW_UPSAMPLE

#ifdef MODE_COPY
	vec4 color = textureLod(source_color, uv_interp, 0.0);
	frag_color = color;
#endif
}
