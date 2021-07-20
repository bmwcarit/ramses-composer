#version 300 es

in vec3 a_Position;
in vec2 a_TextureCoordinate;

uniform mat4 u_MVPMatrix;

out vec2 vTC0;

void main() {
    float offset = float(gl_InstanceID) * 0.2;
	vTC0 = a_TextureCoordinate;
    gl_Position = u_MVPMatrix * vec4(a_Position.x + offset, a_Position.yz, 1.0);
}
