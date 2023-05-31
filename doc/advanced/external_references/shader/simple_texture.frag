#version 300 es
precision highp float;

uniform sampler2D u_Tex;

in vec2 v_TextureCoordinate;

out vec4 FragColor;

void main(){
	vec4 clr0 = texture(u_Tex, v_TextureCoordinate);
	if (clr0.a == 0.0)
		discard;
	FragColor = clr0;
}
