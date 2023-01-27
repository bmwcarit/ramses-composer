#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require

precision mediump float;

uniform samplerExternalOES utex;

uniform bool enableFlatColor;
uniform vec4 color;

in vec2 v_TextureCoordinate;

out vec4 fragColor;

void main() {
	if (enableFlatColor) {
		fragColor = color;
	} else {
		fragColor = texture(utex, v_TextureCoordinate);
	}
}