#version 300 es
precision highp float;

in vec3 a_Position;
in vec3 a_Normal;

uniform mat4 u_MVMatrix;
uniform mat4 u_PMatrix;
uniform mat4 uNormalMatrix;

out vec3 v_NormalWorldSpace;
out vec3 v_VertexWorldSpace;

void main()
{
    vec4 vertWS = u_MVMatrix * vec4(a_Position, 1.0);

    v_NormalWorldSpace = vec3(uNormalMatrix * vec4(a_Normal, 0.0));
    v_VertexWorldSpace = vertWS.xyz / vertWS.w;
    gl_Position = u_PMatrix * vertWS;
}
