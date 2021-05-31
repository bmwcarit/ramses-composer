#version 300 es

precision mediump float;

in vec3 a_Position;
in vec3 a_Normal;

out float lambertian;

uniform mat4 u_MVPMatrix;

void main() {
	lambertian = mix(0.4, 0.8, max(abs(dot(vec3(1.5,2.4,1.0),a_Normal)), 0.0));
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}