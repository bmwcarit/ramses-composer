#version 300 es
precision highp float;
precision highp int;
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

#define alpha 2.0
#define brightnes 1.0
const vec3 color = vec3(1.0);
uniform float opacity;

in vec3 fPosition;
in vec3 fNormal;

out vec4 FragColor;

void main()
{
	vec3 L_normal = normalize(fNormal);
	vec3 weiv_view = normalize(fPosition.xyz);
	float gradient = (-sqrt( cos(0.5 * PI * ( dot(L_normal, weiv_view) ))) + 1.0);
	FragColor = vec4( clamp((( ( clamp( gradient, 0.0, 1.0) -1.0 ) / brightnes ) + 1.0), 0.0, 1.0) * alpha * color, opacity ) ; 
}
