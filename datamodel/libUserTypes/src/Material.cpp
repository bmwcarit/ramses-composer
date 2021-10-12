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
	if (uriGeometry_.asString().empty() || validateURI(context, ValueHandle{shared_from_this(), &Material::uriGeometry_})) {
		context.errors().removeError(ValueHandle{shared_from_this(), &Material::uriGeometry_});
	}
	if (uriDefines_.asString().empty() || validateURI(context, ValueHandle{shared_from_this(), &Material::uriDefines_})) {
		context.errors().removeError(ValueHandle{shared_from_this(), &Material::uriDefines_});
	}

	isShaderValid_ = false;
	PropertyInterfaceList uniforms;
	if (validateURIs<const ValueHandle&, const ValueHandle&>(context, ValueHandle{shared_from_this(), &Material::uriFragment_}, ValueHandle{
		shared_from_this(), &Material::uriVertex_})) {
		std::string vertexShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriVertex_}))};
		std::string geometryShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriGeometry_}))};
		std::string fragmentShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriFragment_}))};
		std::string shaderDefines{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriDefines_}))};
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

	syncTableWithEngineInterface(context, uniforms, ValueHandle(shared_from_this(), &Material::uniforms_), cachedUniformValues_, false, true);
	context.changeMultiplexer().recordValueChanged(ValueHandle(shared_from_this(), &Material::uniforms_));
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	context.updateBrokenLinkErrors(shared_from_this());
}

void Material::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&Material::uriVertex_)) {
		vertexListener_ = registerFileChangedHandler(context, value, [this, &context]() { syncUniforms(context); });
		syncUniforms(context);
	}
	if (value.isRefToProp(&Material::uriFragment_)) {
		fragmentListener_ = registerFileChangedHandler(context, value, [this, &context]() { syncUniforms(context); });
		syncUniforms(context);
	}
	if (value.isRefToProp(&Material::uriGeometry_)) {
		geometryListener_ = registerFileChangedHandler(context, value,  [this, &context]() { syncUniforms(context); });
		syncUniforms(context);
	}
	if (value.isRefToProp(&Material::uriDefines_)) {
			definesListener_ = registerFileChangedHandler(context, value, [this, &context]() { syncUniforms(context); });
		syncUniforms(context);
	}

	if (value.isRefToProp(&Material::objectName_)) {
		context.updateBrokenLinkErrors(shared_from_this());
	}
}

void Material::onAfterContextActivated(BaseContext& context) {
	vertexListener_ = registerFileChangedHandler(context, {shared_from_this(), &Material::uriVertex_},  [this, &context]() { syncUniforms(context); });

	fragmentListener_ = registerFileChangedHandler(context, {shared_from_this(), &Material::uriFragment_},  [this, &context]() { syncUniforms(context); });

	geometryListener_ = registerFileChangedHandler(context, {shared_from_this(), &Material::uriGeometry_}, [this, &context]() { syncUniforms(context); });

	definesListener_ = registerFileChangedHandler(context, {shared_from_this(), &Material::uriDefines_}, 
			[this, &context]() { syncUniforms(context); });

	syncUniforms(context);
}

}  // namespace raco::user_types
