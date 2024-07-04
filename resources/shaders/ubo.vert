#version 320 es

precision mediump float;

in vec3 a_Position;

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

void main() {
    float offset = float(gl_InstanceID) * 0.2;
	
    gl_Position = uCameraBlock.projMat * uCameraBlock.viewMat * uModelBlock.modelMat * vec4(a_Position.x + offset, a_Position.yz, 1.0);
}
