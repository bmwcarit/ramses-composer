#version 300 es
precision highp float;

uniform sampler2D u_ColorBuffer;
in vec2 v_TextureCoordinate;

out vec4 fragColor;

void main()
{
	fragColor = texture(u_ColorBuffer, v_TextureCoordinate);
}
