/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/InfiniteGrid.h"

#include "ramses_adaptor/utilities.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

InfiniteGrid::InfiniteGrid(ramses::Scene* scene)
	: scene_(scene) {
}

static const std::string vertexShader = R"(
#version 300 es

precision highp float;

uniform mat4 u_VMatrix;
uniform mat4 u_PMatrix;

out vec3 ray_near;
out vec3 ray_far;

vec3 unproject(vec3 v) {
	vec4 u = inverse(u_VMatrix) * inverse(u_PMatrix) * vec4(v, 1.0);
	return u.xyz / u.w;
}

void main() {
	// use this with no geometry:
	// - MeshNode::setIndexCount(3)
	// - need to set a GeometryBinding but this can be empty
	vec3 vertices[3] = vec3[](vec3(-1, -1, 0), vec3(-1, 3, 0), vec3(3, -1, 0));

	vec3 p_eye = vertices[gl_VertexID];

	ray_near = unproject(vec3(p_eye.x, p_eye.y, -1.0));
	ray_far = unproject(vec3(p_eye.x, p_eye.y, +1.0));

	gl_Position = vec4(p_eye, 1.0);
}
)";

static const std::string fragmentShader = R"(
#version 300 es

precision highp float;

uniform mat4 u_VMatrix;
uniform mat4 u_PMatrix;

uniform float scale;

uniform int full_grid;

uniform vec3 origin;
uniform vec3 u,v;
uniform int idx_u, idx_v;

uniform vec2 enable;

uniform float axis_color_fac;

in vec3 ray_near;
in vec3 ray_far;

out vec4 fragColor;

void main() {
	vec3 normal = normalize(cross(u, v));

	vec3 ray_dir = ray_far - ray_near;
	float t = (dot(origin, normal) - dot(ray_near, normal)) / dot(ray_dir, normal);
	vec3 p = ray_near + t * ray_dir;

	mat3 m = mat3(u, v, normal);
	mat3 m_inv = inverse(m);
	
	vec2 uv = (m_inv * (p - origin)).xy / scale;
	vec2 fw = fwidth(uv);

	vec2 dist;
	if (full_grid == 1) {
		vec2 uvf = fract(uv);
		dist = min(uvf, 1.0 - uvf) / fw;
	} else {
		dist = abs(uv) / fw;
	}
	
	// interpolate
	//  md=0 -> 1.0
	//  md=n -> 0.0
	
	float lw = 1.5;
	vec2 interp = enable.yx * (1.0 - smoothstep(0.0, lw, dist));
	
	float cc = 0.5;
	float alpha = max(interp.x, interp.y);
	vec4 color = vec4(cc, cc, cc, alpha)* float(t > 0.0);
	
	vec4 color_fac = vec4(axis_color_fac, axis_color_fac, axis_color_fac, 1.0);
	if (abs(uv.x) < lw * fw.x && enable.y == 1.0) {
		color_fac[idx_v] = 2.0;
	} else if (abs(uv.y) < lw * fw.y && enable.x == 1.0) {
		color_fac[idx_u] = 2.0;
	} else if (full_grid == 0) {
		color_fac = vec4(0, 0, 0, 1.0);
	}
    color *= color_fac;

	// Fadeout grid based on distance relative to far plane in eye coordinates:
	vec4 vtmp = inverse(u_PMatrix) * vec4(0.0, 0.0, 1.0, 1.0);
	float fardist = - vtmp.z / vtmp.w;
	vec4 p_eye = u_VMatrix * vec4(p, 1.0);
	float dfrac = length(p_eye.xz) / fardist;
	float fac = max(0.0, 1.0 - 2.0 * dfrac);
	
	fragColor = color * fac;
	
	vec4 p_clip = u_PMatrix * u_VMatrix * vec4(p, 1.0);
	float depth_ndc = p_clip.z/p_clip.w;
	gl_FragDepth = gl_DepthRange.near + gl_DepthRange.diff * (depth_ndc + 1.0)/2.0;
}
)";

void InfiniteGrid::setup(ramses_base::RamsesRenderPass renderPass, bool depthTesting, float axisColorFac, int renderOrder) {
	renderGroup_ = ramsesRenderGroup(scene_, {0, 0});
	renderPass->addRenderGroup(renderGroup_, renderOrder);

	ramses::EffectDescription effectDescription{};
	auto status = effectDescription.setVertexShader(vertexShader.c_str());
	status = effectDescription.setFragmentShader(fragmentShader.c_str());
	status = effectDescription.setUniformSemantic("u_PMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix);
	status = effectDescription.setUniformSemantic("u_VMatrix", ramses::EEffectUniformSemantic::ViewMatrix);
	effect_ = ramsesEffect(scene_, effectDescription, {}, {0, 0});
	appearance_ = ramsesAppearance(scene_, effect_, {0, 0});

	(*appearance_)->setDepthWrite(ramses::EDepthWrite::Disabled);
	if (!depthTesting) {
		(*appearance_)->setDepthFunction(ramses::EDepthFunc::Always);
	}
	(*appearance_)->setCullingMode(ramses::ECullMode::Disabled);
	(*appearance_)->setBlendingOperations(ramses::EBlendOperation::Add, ramses::EBlendOperation::Add);

	ramsesSetUniform(**appearance_, "scale", scale_);
	ramsesSetUniform(**appearance_, "axis_color_fac", axisColorFac);

	geometry_ = ramsesGeometry(scene_, effect_, {0, 0});

	meshNode_ = ramsesMeshNode(scene_, {0, 0});
	meshNode_->setGeometry(geometry_);
	meshNode_->setAppearance(appearance_);
	meshNode_->get()->setIndexCount(3);

	renderGroup_->addMeshNode(meshNode_, 0);
}

void InfiniteGrid::enable(glm::vec3 origin, glm::vec3 u, glm::vec3 v, int idx_u, int idx_v, bool full_grid, glm::vec2 enable) {
	ramsesSetUniform(**appearance_, "full_grid", full_grid ? 1 : 0);
	ramsesSetUniform(**appearance_, "origin", origin);
	ramsesSetUniform(**appearance_, "u", u);
	ramsesSetUniform(**appearance_, "v", v);
	ramsesSetUniform(**appearance_, "idx_u", idx_u);
	ramsesSetUniform(**appearance_, "idx_v", idx_v);
	ramsesSetUniform(**appearance_, "enable", enable);
}

void InfiniteGrid::disable() {
	ramsesSetUniform(**appearance_, "enable", glm::vec2(0.0, 0.0));
}

void InfiniteGrid::setScale(float scale) {
	scale_ = scale;
	ramsesSetUniform(**appearance_, "scale", scale_);
}

}  // namespace raco::ramses_adaptor