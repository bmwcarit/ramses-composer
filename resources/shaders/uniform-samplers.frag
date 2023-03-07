#version 310 es

precision mediump float;

uniform sampler2D s_texture;
uniform samplerCube s_cubemap;
uniform sampler2D s_buffer;
uniform mediump sampler2DMS s_buffer_ms;

out mediump vec4 fragColor;

void main(){
     fragColor = vec4(0.2, 0.4, 0.6, 1.0);
}