$input v_texcoord0

#include "bgfx_shader.sh"

SAMPLER2D(s_tex_color, 0);

const float distancethreshold = 0.5;

float aastep(float threshold, float dist)
{
	float afwidth = 0.5 * length(vec2(dFdx(dist), dFdy(dist)));
	return smoothstep(threshold - afwidth, threshold + afwidth, dist);
}

float sample(vec2 offset, float width) {
	float dist = texture2D(s_tex_color, v_texcoord0 - offset).a;
	return aastep(distancethreshold - width, dist);
}

void main() {
	vec4 out_color = vec4_splat(0.0);
	out_color += vec4(0.0, 0.0, 0.0, 0.5) * sample(vec2(0.0025, 0.0025), 0.25);
	out_color += vec4_splat(1.0) * sample(vec2(0.0, 0.0), 0.0);

	gl_FragColor = out_color;
}
