#version 320 es

precision mediump float;

#include "uniforms_geom.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 5) out;

void main() {
    ivec2 test = iv2;
    vec4 positionIn = gl_in[0].gl_Position;
    
    gl_Position = positionIn;
    EmitVertex();
    
    gl_Position = positionIn;
    EmitVertex();
    
    gl_Position = positionIn;
    EmitVertex();
    EndPrimitive();
}