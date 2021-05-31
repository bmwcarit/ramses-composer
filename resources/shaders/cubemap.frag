#version 300 es
precision mediump float; 

in vec3 vTC0;

uniform samplerCube uTex;

out vec4 FragColor;

void main() {    
    FragColor = texture(uTex, vTC0).rgba;
}