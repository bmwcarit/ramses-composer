#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require

precision mediump float;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

uniform mat4 u_MVPMatrix;

void main() {
	v_TextureCoordinate = a_TextureCoordinate;
	
	gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}