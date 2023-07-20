#version 320 es

precision mediump float;

#include "uniforms_vert.glsl"

in vec3 a_Position;

uniform mat4 u_MVPMatrix;

void main() {
    vec2 test = v2;
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}