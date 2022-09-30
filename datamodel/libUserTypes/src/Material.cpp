/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
	if (validateURIs<const ValueHandle&, const ValueHandle&>(context, ValueHandle{shared_from_this(), &Material::uriFragment_}, ValueHandle{shared_from_this(), &Material::uriVertex_})) {
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
			context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, ValueHandle{shared_from_this()}, error);
		}
	}
	if (!isShaderValid_) {
		attributes_.clear();
	}

	syncTableWithEngineInterface(context, uniforms, ValueHandle(shared_from_this(), &Material::uniforms_), cachedUniformValues_, false, true);
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());

	context.updateBrokenLinkErrors(shared_from_this());
}

std::string Material::getShaderFileEnding(ShaderFileType shader, bool endsWithDotGlsl) {
	if (endsWithDotGlsl) {
		switch (shader) {
			case ShaderFileType::Vertex:
				return "vert.glsl";
			case ShaderFileType::Geometry:
				return "geom.glsl";
			case ShaderFileType::Fragment:
				return "frag.glsl";
			case ShaderFileType::Defines:
				return ".def";
		}
	}

	switch (shader) {
		case ShaderFileType::Vertex:
			return ".vert";
		case ShaderFileType::Geometry:
			return ".geom";
		case ShaderFileType::Fragment:
			return ".frag";
		case ShaderFileType::Defines:
			return ".def";
	}

	return "???";
}

void Material::autofillShaderIfFileExists(BaseContext& context, const std::string& fileNameWithoutEnding, bool endsWithDotGlsl, ShaderFileType shaderType) {
	auto fileName = fileNameWithoutEnding + getShaderFileEnding(shaderType, endsWithDotGlsl);
	if (raco::utils::u8path(fileName).normalizedAbsolutePath(context.project()->currentFolder()).existsFile()) {
		ValueHandle handle;
		switch (shaderType) {
			case ShaderFileType::Vertex:
				handle = ValueHandle{shared_from_this(), &Material::uriVertex_};
				break;
			case ShaderFileType::Geometry:
				handle = ValueHandle{shared_from_this(), &Material::uriGeometry_};
				break;
			case ShaderFileType::Fragment:
				handle = ValueHandle{shared_from_this(), &Material::uriFragment_};
				break;
			case ShaderFileType::Defines:
				handle = ValueHandle{shared_from_this(), &Material::uriDefines_};
				break;
		}

		context.set(handle, fileName);
	}
}

bool Material::areAllOtherShadersEmpty(ShaderFileType fileType) {
	switch (fileType) {
		case ShaderFileType::Vertex:
			return uriFragment_->empty() && uriGeometry_->empty() && uriDefines_->empty();
		case ShaderFileType::Geometry:
			return uriVertex_->empty() && uriFragment_->empty() && uriDefines_->empty();
		case ShaderFileType::Fragment:
			return uriVertex_->empty() && uriGeometry_->empty() && uriDefines_->empty();
		case ShaderFileType::Defines:
			return uriVertex_->empty() && uriFragment_->empty() && uriGeometry_->empty();
	}

	return false;
}

void Material::autofillShaderFiles(BaseContext& context, const std::string& fileName, ShaderFileType setShaderProperty) {
	if (fileName.empty() || !areAllOtherShadersEmpty(setShaderProperty)) {
		return;
	}

	auto endsWithDotGlsl = false;
	auto fileEnding = getShaderFileEnding(setShaderProperty, endsWithDotGlsl);
	if (fileName.length() < fileEnding.length() ||  0 != fileName.compare(fileName.length() - fileEnding.length(), fileEnding.length(), fileEnding)) {
		endsWithDotGlsl = true;
		fileEnding = getShaderFileEnding(setShaderProperty, endsWithDotGlsl);
		if (fileName.length() < fileEnding.length() || 0 != fileName.compare(fileName.length() - fileEnding.length(), fileEnding.length(), fileEnding)) {
			return;
		}
	}

	auto fileNameWithoutEnding = fileName.substr(0, fileName.length() - fileEnding.length());
	switch (setShaderProperty) {
		case ShaderFileType::Vertex:
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Geometry);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Fragment);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Defines);
			break;
		case ShaderFileType::Geometry:
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Vertex);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Fragment);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Defines);
			break;
		case ShaderFileType::Fragment:
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Vertex);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Geometry);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Defines);
			break;
		case ShaderFileType::Defines:
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Vertex);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Geometry);
			autofillShaderIfFileExists(context, fileNameWithoutEnding, endsWithDotGlsl, ShaderFileType::Fragment);
			break;
	}
}

void Material::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&Material::objectName_)) {
		context.updateBrokenLinkErrors(shared_from_this());
	}

	if (value.isRefToProp(&Material::uriVertex_)) {
		autofillShaderFiles(context, value.asString(), ShaderFileType::Vertex);
	}

	if (value.isRefToProp(&Material::uriGeometry_)) {
		autofillShaderFiles(context, value.asString(), ShaderFileType::Geometry);
	}

	if (value.isRefToProp(&Material::uriFragment_)) {
		autofillShaderFiles(context, value.asString(), ShaderFileType::Fragment);
	}

	if (value.isRefToProp(&Material::uriDefines_)) {
		autofillShaderFiles(context, value.asString(), ShaderFileType::Defines);
	}
}

}  // namespace raco::user_types
