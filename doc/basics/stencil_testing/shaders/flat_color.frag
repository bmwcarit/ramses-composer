#version 300 es
precision highp float;

out vec4 FragColor;
uniform vec3 u_color;

void main() {
    FragColor = vec4(u_color.r, u_color.g, u_color.b, 1.0);
}
