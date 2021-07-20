#version 300 es

precision mediump float;

in vec2 vTC0;

uniform sampler2D uTex0;
uniform samplerCube cubeTex;

uniform float scalar;
uniform vec2 v2;
uniform vec3 vec;
uniform vec4 ambient;
uniform int count_;

uniform ivec2 iv2;
uniform ivec3 iv3;
uniform ivec4 iv4;

out mediump vec4 fragColor;

void main(){
    vec3 clr0 = texture(uTex0, vTC0).rgb;
    if (scalar > 0.5) {
        fragColor = vec4(scalar, float(count_)/100.0, scalar, 0.0);
    }
    else {
        fragColor = vec4(vec + clr0, scalar) + ambient;
    }
}
