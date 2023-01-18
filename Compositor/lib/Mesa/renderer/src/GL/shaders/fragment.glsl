
/* enum class GLES::GlesShaderVariant */
#define VARIANT_SINGLE_COLOR = 1,
#define VARIANT_TEXTURE_EXTERNAL = 2,
#define VARIANT_TEXTURE_RGBA = 3,
#define VARIANT_TEXTURE_RGBX = 4,
#define VARIANT_TEXTURE_Y_U_V = 5,
#define VARIANT_TEXTURE_Y_UV = 6,
#define VARIANT_TEXTURE_Y_XUXV = 7,
#define VARIANT_TEXTURE_XYUV = 8,

#if !defined(VARIANT)
#error "Missing shader preamble"
#endif

#if VARIANT == VARIANT_TEXTURE_EXTERNAL
#extension GL_OES_EGL_image_external : require
#endif

precision mediump float;

varying vec2 v_texcoord;

vec4 yuva2rgba(vec4 yuva)
{
	vec4 color_out;
	float Y, su, sv;

	/* ITU-R BT.601 & BT.709 quantization (limited range) */

	/* Y = 255/219 * (x - 16/256) */
	Y = 1.16438356 * (yuva.x - 0.0625);

	/* Remove offset 128/256, but the 255/224 multiplier comes later */
	su = yuva.y - 0.5;
	sv = yuva.z - 0.5;

	/*
	 * ITU-R BT.601 encoding coefficients (inverse), with the
	 * 255/224 limited range multiplier already included in the
	 * factors for su (Cb) and sv (Cr).
	 */
	color_out.r = Y                   + 1.59602678 * sv;
	color_out.g = Y - 0.39176229 * su - 0.81296764 * sv;
	color_out.b = Y + 2.01723214 * su;

	color_out.a = yuva.w;

	return color_out;
}

#if VARIANT == VARIANT_TEXTURE_EXTERNAL
uniform samplerExternalOES tex0;
#elif VARIANT == SHADER_VARIANT_Y_U_V \
   || VARIANT == SHADER_VARIANT_Y_UV \
   || VARIANT == SHADER_VARIANT_Y_XUXV 
uniform sampler2D tex0;
#endif

#if VARIANT == SHADER_VARIANT_Y_U_V \
 || VARIANT == SHADER_VARIANT_Y_UV \
 || VARIANT == SHADER_VARIANT_Y_XUXV \
 || VARIANT == SHADER_VARIANT_XYUV
uniform sampler2D tex1;
#endif

#if VARIANT == SHADER_VARIANT_Y_U_V)
uniform sampler2D tex2;
#endif


#if VARIANT != VARIANT_SINGLE_COLOR
uniform float alpha;
#else
const float alpha = 1.0;
uniform vec4 color;
#endif

vec4 sample_texture() {
#if VARIANT == VARIANT_TEXTURE_RGBA || VARIANT == VARIANT_TEXTURE_EXTERNAL
	return texture2D(tex0, v_texcoord);
#elif VARIANT == VARIANT_TEXTURE_RGBX
	return vec4(texture2D(tex0, v_texcoord).rgb, 1.0);
#elif VARIANT == VARIANT_SINGLE_COLOR
	return color;
#else
	// RGB conversion
    vec4 yuva = vec4(0.0, 0.0, 0.0, 1.0);

	#if VARIANT == SHADER_VARIANT_Y_U_V) {
			yuva.x = texture2D(tex0, v_texcoord).x;
			yuva.y = texture2D(tex1, v_texcoord).x;
			yuva.z = texture2D(tex2, v_texcoord).x;
	#elif VARIANT == SHADER_VARIANT_Y_UV) {
			yuva.x = texture2D(tex0, v_texcoord).x;
			yuva.yz = texture2D(tex1, v_texcoord).rg;
	#elif VARIANT == SHADER_VARIANT_Y_XUXV) {
			yuva.x = texture2D(tex0, v_texcoord).x;
			yuva.yz = texture2D(tex1, v_texcoord).ga;
	#elif VARIANT == SHADER_VARIANT_XYUV) {
			yuva.xyz = texture2D(tex0, v_texcoord).bgr;
	#endif
	
	return yuva2rgba(yuva);
#endif
}

void main() {
	vec4 color = sample_input_texture();

	color *= alpha;
	
	gl_FragColor = color;
}