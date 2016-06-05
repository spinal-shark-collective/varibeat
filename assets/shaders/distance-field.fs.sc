$input v_texcoord0

#include "bgfx_shader.sh"

SAMPLER2D(s_tex_color, 0);

const float distancethreshold = 0.5;

float aastep(float threshold, float dist)
{
	float afwidth = 0.5 * length(vec2(dFdx(dist), dFdy(dist)));
	return smoothstep(threshold - afwidth, threshold + afwidth, dist);
}

void main()
{
	float dist = texture2D(s_tex_color, v_texcoord0).a;
	float alpha = aastep(distancethreshold, dist);
	gl_FragColor = vec4(vec3_splat(1.0), alpha);
}
