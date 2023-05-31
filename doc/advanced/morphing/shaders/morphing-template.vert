#version 300 es

precision highp float;

in vec3 a_Position;
in vec3 a_Position_Morph_0;
in vec3 a_Position_Morph_1;

in vec3 a_Normal;
in vec3 a_Normal_Morph_0;
in vec3 a_Normal_Morph_1;

uniform mat4 u_MVPMatrix;
uniform mat4 u_NMatrix;

uniform float weights[2];

out float intensity;

void main() {
	vec3 lightdir = (vec4(normalize(vec3(1.5, -2.4, -3.0)), 0.0)).xyz;

	vec3 m_Normal = a_Normal
		+ weights[0] * a_Normal_Morph_0
		+ weights[1] * a_Normal_Morph_1;
	vec3 normal = normalize((u_NMatrix * vec4(m_Normal, 0.0)).xyz);

	intensity = max(dot(-lightdir, normal), 0.0);

    gl_Position = u_MVPMatrix * vec4(a_Position 
		+ weights[0] * a_Position_Morph_0
		+ weights[1] * a_Position_Morph_1,
	1.0);
}