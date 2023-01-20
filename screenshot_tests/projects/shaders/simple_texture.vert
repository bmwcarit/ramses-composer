#version 300 es
precision highp float;

uniform mat4 uWorldViewProjectionMatrix;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

void main() {
	v_TextureCoordinate = a_TextureCoordinate;
	
	gl_Position = uWorldViewProjectionMatrix*vec4(a_Position.xyz, 1.0);
}
