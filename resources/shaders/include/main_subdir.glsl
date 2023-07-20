#version 300 es

#include "subdirectory/func3.glsl"

precision mediump float;

in float lambertian;

out vec4 fragColor;

void main() {
     fragColor = vec4(1.0, 0.5, 0.0, 1.0) * lambertian;
}