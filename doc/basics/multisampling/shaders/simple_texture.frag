#version 300 es
precision highp float;

uniform sampler2D u_Tex;

in vec2 v_TextureCoordinate;

out vec4 FragColor;

void main(){
	FragColor = texture(u_Tex, v_TextureCoordinate);
}
