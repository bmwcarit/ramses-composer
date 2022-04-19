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
#include "utils/FileUtils.h"
#include <algorithm>

namespace raco::user_types {


const PropertyInterfaceList& Material::attributes() const {
	return attributes_;
}

void Material::updateFromExternalFile(BaseContext& context) {
	context.errors().removeError(ValueHandle{shared_from_this()});
	bool isUriGeometryValid = false;
	if (uriGeometry_.asString().empty() || (isUriGeometryValid = validateURI(context, ValueHandle{shared_from_this(), &Material::uriGeometry_}))) {
		context.errors().removeError(ValueHandle{shared_from_this(), &Material::uriGeometry_});
	}
	bool isUriDefinesValid = false;
	if (uriDefines_.asString().empty() || (isUriDefinesValid = validateURI(context, ValueHandle{shared_from_this(), &Material::uriDefines_}))) {
		context.errors().removeError(ValueHandle{shared_from_this(), &Material::uriDefines_});
	}

	isShaderValid_ = false;
	PropertyInterfaceList uniforms;
	if (validateURIs<const ValueHandle&, const ValueHandle&>(context, ValueHandle{shared_from_this(), &Material::uriFragment_}, ValueHandle{
		shared_from_this(), &Material::uriVertex_})) {
		std::string vertexShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriVertex_}))};
		std::string fragmentShader{raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriFragment_}))};

		std::string geometryShader = "";
		if (isUriGeometryValid) {
			geometryShader = {raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriGeometry_}))};
			// ramses treats the empty string as no geometry shader should be loaded and only shows errors if the stringcontains at least a single character. 
			// We want to display errors. We want to display errors when the file load was successful but the file is empty.
			if (geometryShader == "") {
				geometryShader = " ";
			}
		}

		std::string shaderDefines = "";
		if (isUriDefinesValid) {
			shaderDefines = {raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriDefines_}))};
			// ramses treats the empty string as no shader defines should be loaded and only shows errors if the string contains at least a single charcater. 
			// We want to display errors when the file load was successful but the file is empty.
			if (shaderDefines == "") {
				shaderDefines = " ";
			}
		}

		std::string error{};
		isShaderValid_ = context.engineInterface().parseShader(vertexShader, geometryShader, fragmentShader, shaderDefines, uniforms, attributes_, error);
		if (error.size() > 0) {
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, ValueHandle{shared_from_this()}, error);
		}		
	}
	if (!isShaderValid_) {
		attributes_.clear();
	}

	syncTableWithEngineInterface(context, uniforms, ValueHandle(shared_from_this(), &Material::uniforms_), cachedUniformValues_, false, true);
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	context.updateBrokenLinkErrors(shared_from_this());
}

void Material::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&Material::objectName_)) {
		context.updateBrokenLinkErrors(shared_from_this());
	}
}

}  // namespace raco::user_types
