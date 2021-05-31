/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/MaterialAdaptor.h"

#include "utils/FileUtils.h"
#include "log_system/log.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/Utils.h"
#include "user_types/Material.h"

namespace raco::ramses_adaptor {

constexpr const char* emptyVertexShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

constexpr const char* emptyFragmentShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

raco::ramses_base::RamsesEffect MaterialAdaptor::createEffect(SceneAdaptor* sceneAdaptor) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(emptyVertexShader);
	effectDescription.setFragmentShader(emptyFragmentShader);
	return raco::ramses_base::ramsesEffect(sceneAdaptor->scene(), effectDescription);
}

MaterialAdaptor::MaterialAdaptor(SceneAdaptor* sceneAdaptor, user_types::SMaterial material)
	: TypedObjectAdaptor{sceneAdaptor, material, createEffect(sceneAdaptor)},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject(), [this]() {
		  tagDirty();
	  })} {
}

bool MaterialAdaptor::isValid() {
	return editorObject()->isShaderValid();
}

bool MaterialAdaptor::sync(core::Errors* errors) {
	bool status = TypedObjectAdaptor<user_types::Material, ramses::Effect>::sync(errors);
	LOG_TRACE(raco::log_system::RAMSES_ADAPTOR, "valid: {}", isValid());
	if (editorObject()->isShaderValid()) {
		std::string const vertexShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {"uriVertex"}}));
		std::string const fragmentShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {"uriFragment"}}));
		std::string const geometryShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {"uriGeometry"}}));
		std::string const shaderDefines = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {"uriDefines"}}));
		auto const effectDescription = raco::ramses_base::createEffectDescription(vertexShader, geometryShader, fragmentShader, shaderDefines);
		reset(raco::ramses_base::ramsesEffect(sceneAdaptor_->scene(), *effectDescription));
	} else {
		reset(createEffect(sceneAdaptor_));
	}
	tagDirty(false);
	return true;
}

};	// namespace raco::ramses_adaptor
