float gammaToLinearPrecise(float c) {
	return c <= 0.04045 ? c * 0.077399380804954 : pow((c + 0.055) * 0.9478672985782, 2.4);
}
vec3 gammaToLinearPrecise(vec3 c) {
	bvec3 leq = lessThanEqual(c, vec3(0.04045));
	c.r = leq.r ? c.r * 0.077399380804954 : pow((c.r + 0.055) * 0.9478672985782, 2.4);
	c.g = leq.g ? c.g * 0.077399380804954 : pow((c.g + 0.055) * 0.9478672985782, 2.4);
	c.b = leq.b ? c.b * 0.077399380804954 : pow((c.b + 0.055) * 0.9478672985782, 2.4);
	return c;
}
vec4 gammaToLinearPrecise(vec4 c) { return vec4(gammaToLinearPrecise(c.rgb), c.a); }
float linearToGammaPrecise(float c) {
	return c < 0.0031308 ? c * 12.92 : 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}
vec3 linearToGammaPrecise(vec3 c) {
	bvec3 lt = lessThanEqual(c, vec3(0.0031308));
	c.r = lt.r ? c.r * 12.92 : 1.055 * pow(c.r, 1.0 / 2.4) - 0.055;
	c.g = lt.g ? c.g * 12.92 : 1.055 * pow(c.g, 1.0 / 2.4) - 0.055;
	c.b = lt.b ? c.b * 12.92 : 1.055 * pow(c.b, 1.0 / 2.4) - 0.055;
	return c;
}
vec4 linearToGammaPrecise(vec4 c) { return vec4(linearToGammaPrecise(c.rgb), c.a); }

// pow(x, 2.2) isn't an amazing approximation, but at least it's efficient...
mediump float gammaToLinearFast(mediump float c) { return pow(max(c, 0.0), 2.2); }
mediump vec3 gammaToLinearFast(mediump vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
mediump vec4 gammaToLinearFast(mediump vec4 c) { return vec4(gammaToLinearFast(c.rgb), c.a); }
mediump float linearToGammaFast(mediump float c) { return pow(max(c, 0.0), 1.0 / 2.2); }
mediump vec3 linearToGammaFast(mediump vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }
mediump vec4 linearToGammaFast(mediump vec4 c) { return vec4(linearToGammaFast(c.rgb), c.a); }

#ifdef MYON_PRECISE_GAMMA
#define gammaToLinear gammaToLinearPrecise
#define linearToGamma linearToGammaPrecise
#else
#define gammaToLinear gammaToLinearFast
#define linearToGamma linearToGammaFast
#endif

#ifdef MYON_GAMMA_CORRECT
#define gammaCorrectColor gammaToLinear
#define unGammaCorrectColor linearToGamma
#define gammaCorrectColorPrecise gammaToLinearPrecise
#define unGammaCorrectColorPrecise linearToGammaPrecise
#define gammaCorrectColorFast gammaToLinearFast
#define unGammaCorrectColorFast linearToGammaFast
#else
#define gammaCorrectColor
#define unGammaCorrectColor
#define gammaCorrectColorPrecise
#define unGammaCorrectColorPrecise
#define gammaCorrectColorFast
#define unGammaCorrectColorFast
#endif
