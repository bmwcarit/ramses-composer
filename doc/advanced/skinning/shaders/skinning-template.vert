#version 300 es
precision highp float;

in vec3 a_Position;
//in vec3 a_Normal;

uniform mat4 u_VMatrix;
uniform mat4 u_PMatrix;

in vec4 a_Joints0;
in vec4 a_Weights0;

// Adjust array size to actual number of joints
uniform mat4 u_jointMat[2];

//out float lambertian;

void main() {
	//lambertian = mix(0.4, 0.8, max(abs(dot(vec3(1.5,2.4,1.0),a_Normal)), 0.0));

	mat4 skinMat = 
		a_Weights0.x * u_jointMat[int(a_Joints0.x)] +
		a_Weights0.y * u_jointMat[int(a_Joints0.y)] +
		a_Weights0.z * u_jointMat[int(a_Joints0.z)] +
		a_Weights0.w * u_jointMat[int(a_Joints0.w)];

    gl_Position = u_PMatrix * u_VMatrix * skinMat * vec4(a_Position, 1.0);
}
