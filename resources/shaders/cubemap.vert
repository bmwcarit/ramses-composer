#version 300 es
precision mediump float;

in vec3 a_Position;

out vec3 vTC0;

uniform mat4 u_MVPMatrix;

void main() {
	vTC0 = a_Position;
    gl_Position = u_MVPMatrix * vec4(a_Position.xyz, 1.0);
}

