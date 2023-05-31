#version 300 es
precision highp float;

uniform mat4 uWorldViewProjectionMatrix;
uniform bool u_FlipUV;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

void main() {
	if (!u_FlipUV)
		v_TextureCoordinate = a_TextureCoordinate;
	else
		// Flip V coordinate, because Blender exports quads with top-left origin rather than bottom-left
		v_TextureCoordinate = vec2(a_TextureCoordinate.x, 1.0 - a_TextureCoordinate.y);

	gl_Position = uWorldViewProjectionMatrix*vec4(a_Position.xyz, 1.0);
}
