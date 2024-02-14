/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/ClearDepthObject.h"

#include "ramses_adaptor/utilities.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

ClearDepthObject::ClearDepthObject(ramses::Scene* scene)
	: scene_(scene) {
}

static const std::string vertexShader = R"(
#version 300 es

precision highp float;

void main() {
	// use this with no geometry:
	// - MeshNode::setIndexCount(3)
	// - need to set a GeometryBinding but this can be empty
	vec3 vertices[3] = vec3[](vec3(-1, -1, 0), vec3(-1, 3, 0), vec3(3, -1, 0));

	vec3 p_eye = vertices[gl_VertexID];

	gl_Position = vec4(p_eye, 1.0);
}
)";

static const std::string fragmentShader = R"(
#version 300 es

precision highp float;

void main() {
	gl_FragDepth = gl_DepthRange.far;
}
)";

void ClearDepthObject::setup(ramses_base::RamsesRenderPass renderPass, int renderOrder) {
	renderGroup_ = ramsesRenderGroup(scene_, {0, 0});
	renderPass->addRenderGroup(renderGroup_, renderOrder);

	ramses::EffectDescription effectDescription{};
	auto status = effectDescription.setVertexShader(vertexShader.c_str());
	status = effectDescription.setFragmentShader(fragmentShader.c_str());
	effect_ = ramsesEffect(scene_, effectDescription, {}, {0, 0});
	appearance_ = ramsesAppearance(scene_, effect_, {0, 0});

	(*appearance_)->setDepthWrite(ramses::EDepthWrite::Enabled);
	(*appearance_)->setDepthFunction(ramses::EDepthFunc::Always);
	(*appearance_)->setCullingMode(ramses::ECullMode::Disabled);
	(*appearance_)->setColorWriteMask(false, false, false, false);

	geometry_ = ramsesGeometry(scene_, effect_, {0, 0});

	meshNode_ = ramsesMeshNode(scene_, {0, 0});
	meshNode_->setGeometry(geometry_);
	meshNode_->setAppearance(appearance_);
	meshNode_->get()->setIndexCount(3);

	renderGroup_->addMeshNode(meshNode_, 0);
}

}  // namespace raco::ramses_adaptor