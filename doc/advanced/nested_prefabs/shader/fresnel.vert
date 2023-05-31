#version 300 es
precision highp float;

in vec3 a_Position;
in vec3 a_Normal;

uniform mat4 uNormalMatrix;
uniform mat4 u_MVMatrix;
uniform mat4 uWorldViewProjectionMatrix;

out vec3 fNormal;
out vec3 fPosition;

void main()
{
  fNormal = (uNormalMatrix  *  vec4(a_Normal, 1.0)).xyz;
  vec4 pos = u_MVMatrix * vec4(a_Position, 1.0);
  fPosition = pos.xyz;
  gl_Position = uWorldViewProjectionMatrix * vec4(a_Position, 1.0);
}