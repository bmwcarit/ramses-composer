#version 320 es
precision highp float;

in vec3 a_Position;
in vec3 a_Normal;

layout(std140, binding = 0) uniform ModelBlock_t {
	mat4 modelMat;
} uModelBlock;

layout(std140, binding = 1) uniform CameraBlock_t
{
    mat4 projMat;
    mat4 viewMat;
    vec3 camPos;
} uCameraBlock;

layout(std140, binding = 2) uniform CameraModelBlock_t
{
    mat4 mvpMat;
    mat4 mvMat;
    mat4 normalMat;
} uModelCameraBlock;

out vec3 v_NormalWorldSpace;
out vec3 v_VertexWorldSpace;

void main()
{
    vec4 vertWS = uModelCameraBlock.mvMat * vec4(a_Position, 1.0);

    v_NormalWorldSpace = vec3(uModelCameraBlock.normalMat * vec4(a_Normal, 0.0));
    v_VertexWorldSpace = vertWS.xyz / vertWS.w;
    gl_Position = uCameraBlock.projMat * vertWS;
}
