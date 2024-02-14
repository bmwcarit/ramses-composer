
#version 300 es
precision mediump float;
in vec3 a_Position;

uniform mat4 u_MVPMatrix;
void main() {
    float offset = float(gl_InstanceID) * 0.2;
    gl_Position = u_MVPMatrix * vec4(a_Position.x + offset, a_Position.yz, 1.0);
}
