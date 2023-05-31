#version 300 es
precision highp float;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

uniform mat4 u_MVPMatrix;

void main() {
    // Flit V coordinate, because Blender exports quads with top-left origin rather than bottom-left
	v_TextureCoordinate = vec2(a_TextureCoordinate.x, 1.0 - a_TextureCoordinate.y);
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}
