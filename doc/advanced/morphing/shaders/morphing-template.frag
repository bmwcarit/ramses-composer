#version 300 es

precision highp float;

in float intensity;

out vec4 FragColor;

uniform vec3 u_color;

void main() {
    FragColor = vec4(u_color.rgb * intensity, 1.0);
}
