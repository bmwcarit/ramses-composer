#version 300 es

precision mediump float;

uniform float scalar;

uniform int ivec[2];
uniform float fvec[5];

uniform vec2 avec2[4];
uniform vec3 avec3[5];
uniform vec4 avec4[6];

uniform ivec2 aivec2[4];
uniform ivec3 aivec3[5];
uniform ivec4 aivec4[6];

out mediump vec4 fragColor;

void main(){
     fragColor = vec4(scalar * fvec[0], scalar * fvec[1], scalar * fvec[2], scalar * fvec[3]);
}