#version 300 es

precision mediump float;

uniform bool b;
uniform int i;
uniform float f;

uniform vec2 v2;
uniform vec3 v3;
uniform vec4 v4;

uniform ivec2 iv2;
uniform ivec3 iv3;
uniform ivec4 iv4;

out mediump vec4 fragColor;

void main(){
     fragColor = vec4(f, f, f, 1.0);
}