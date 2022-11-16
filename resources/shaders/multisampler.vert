#version 310 es

uniform mat4 uWorldViewProjectionMatrix;

in vec3 a_Position;
in vec2 a_TextureCoordinate;
out vec2 v_TextureCoordinate;

void main()
{
    gl_Position = uWorldViewProjectionMatrix * vec4(a_Position, 1.0);
    v_TextureCoordinate = a_TextureCoordinate;
}