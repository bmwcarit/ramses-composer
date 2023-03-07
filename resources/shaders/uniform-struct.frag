#version 310 es

precision mediump float;

struct struct_prim {
	int i;
	float f;
	
	vec2 v2;
	vec3 v3;
	vec4 v4;
	
	ivec2 iv2;
	ivec3 iv3;
	ivec4 iv4;
};

struct struct_sampler {
	sampler2D s_texture;
	samplerCube s_cubemap;
	sampler2D s_buffer;
	mediump sampler2DMS s_buffer_ms;
};

struct struct_array_prim {
	int ivec[2];
	float fvec[5];

	vec2 avec2[4];
	vec3 avec3[5];
	vec4 avec4[6];

	ivec2 aivec2[4];
	ivec3 aivec3[5];
	ivec4 aivec4[6];
};

struct struct_array_struct_prim {
	struct_prim prims[2];
};
	
struct struct_array_struct_samplers {
	struct_sampler samplers[2];
};

struct struct_nested {
	struct_prim prims;
	struct_sampler samplers;
};

uniform struct_prim s_prims;

uniform struct_sampler s_samplers;

uniform struct_array_prim s_a_prims;

uniform struct_nested nested;

uniform struct_array_struct_prim s_a_struct_prim;

uniform struct_array_struct_samplers s_a_struct_samplers;

uniform struct_prim a_s_prims[2];

uniform struct_sampler a_s_samplers[2];

uniform struct_array_struct_prim a_s_a_struct_prim[2];

uniform struct_array_struct_samplers a_s_a_struct_samplers[2];

out mediump vec4 fragColor;

void main(){
	fragColor = vec4(0.2, 0.3, 0.4, 1.0);
}