#version 310 es
precision highp float;

uniform mat4 uWorldViewProjectionMatrix;

in vec3 a_Position;
in vec2 a_TextureCoordinate;
out vec2 v_TextureCoordinate;

void main()
{
	// Flip V coordinate, because Blender exports quads with top-left origin rather than bottom-left
	v_TextureCoordinate = vec2(a_TextureCoordinate.x, 1.0 - a_TextureCoordinate.y);

    gl_Position = uWorldViewProjectionMatrix * vec4(a_Position, 1.0);
}
