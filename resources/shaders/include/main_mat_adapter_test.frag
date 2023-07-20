#version 320 es

precision mediump float;

#include "uniforms_frag.glsl"

out mediump vec4 fragColor;

void main(){
     fragColor = vec4(f, f, f, 1.0);
}