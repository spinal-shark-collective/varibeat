$input a_position, a_color0, a_normal
$output v_color0, v_normal

#include "bgfx_shader.sh"

#define MYON_GAMMA_CORRECT
#include "gamma.sh"

void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
	v_color0 = gammaCorrectColor(a_color0 / 255.0);
	v_normal = a_normal;
}
