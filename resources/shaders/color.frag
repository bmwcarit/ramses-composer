
#version 300 es
precision mediump float;

in vec4 vColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(vColor.rgb, 1.0);
}
