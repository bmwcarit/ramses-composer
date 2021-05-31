/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Material.h"

#include "Validation.h"
#include "core/Context.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "log_system/log.h"
#include "utils/FileUtils.h"
#include <algorithm>

namespace raco::user_types {

void Material::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	vertexListener_.reset();
	geometryListener_.reset();
	fragmentListener_.reset();
	definesListener_.reset();
}

const PropertyInterfaceList& Material::attributes() const {
	return attributes_;
}

void Material::syncUniforms(BaseContext& context) {
	context.errors().removeError(ValueHandle{shared_from_this()});
	if (uriGeometry_.asString().empty() || validateURI(context, ValueHandle{shared_from_this(), {"uriGeometry"}})) {
		context.errors().removeError(ValueHandle{shared_from_this(), {"uriGeometry"}});
	}
	if (uriDefines_.asString().empty() || validateURI(context, ValueHandle{shared_from_this(), {"uriDefines"}})) {
		context.errors().removeError(ValueHandle{shared_from_this(), {"uriDefines"}});
	}

	isShaderValid_ = false;
	PropertyInterfaceList uniforms;
	if (validateURIs<const ValueHandle&, const ValueHandle&>(context, ValueHandle{shared_from_this(), {"uriFragment"}}, ValueHandle{shared_from_this(), {"uriVertex"}})) {
		std::string vertexShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uriVertex"}}))};
		std::string geometryShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uriGeometry"}}))};
		std::string fragmentShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uriFragment"}}))};
		std::string shaderDefines{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uriDefines"}}))};
		if (!vertexShader.empty() && !fragmentShader.empty()) {
			std::string error{};
			isShaderValid_ = context.engineInterface().parseShader(vertexShader, geometryShader, fragmentShader, shaderDefines, uniforms, attributes_, error);
			if (error.size() > 0) {
				context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, ValueHandle{shared_from_this()}, error);
			}
		}
	}
	if (!isShaderValid_) {
		attributes_.clear();
	}

	syncTableWithEngineInterface(context, uniforms, ValueHandle(shared_from_this(), {"uniforms"}), cachedUniformValues_, false, false);
	context.changeMultiplexer().recordValueChanged(ValueHandle(shared_from_this(), {"uniforms"}));
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void Material::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	ValueHandle vertextUriHandle(shared_from_this(), {"uriVertex"});
	if (vertextUriHandle == value) {
		vertexListener_ = registerFileChangedHandler(context, vertextUriHandle, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});
		syncUniforms(context);
	}
	ValueHandle fragmentUriHandle(shared_from_this(), {"uriFragment"});
	if (fragmentUriHandle == value) {
		fragmentListener_ = registerFileChangedHandler(context, fragmentUriHandle, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});
		syncUniforms(context);
	}
	ValueHandle geometryUriHandle(shared_from_this(), {"uriGeometry"});
	if (geometryUriHandle == value) {
		geometryListener_ = registerFileChangedHandler(context, geometryUriHandle, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});
		syncUniforms(context);
	}
	ValueHandle definesUriHandle(shared_from_this(), {"uriDefines"});
	if (definesUriHandle == value) {
		geometryListener_ = registerFileChangedHandler(context, definesUriHandle, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});
		syncUniforms(context);
	}
}

void Material::onAfterContextActivated(BaseContext& context) {
	vertexListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uriVertex"}}, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});

	fragmentListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uriFragment"}}, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});

	geometryListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uriGeometry"}}, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});

	definesListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uriDefines"}}, {shared_from_this(),
			[this](BaseContext& context) { syncUniforms(context); }});

	syncUniforms(context);
}

}  // namespace raco::user_types
