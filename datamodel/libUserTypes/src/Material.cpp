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
#include "core/MeshCacheInterface.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "utils/FileUtils.h"
#include "utils/ShaderPreprocessor.h"

namespace raco::user_types {

using utils::shader::ShaderPreprocessor;

const PropertyInterfaceList& Material::attributes() const {
	return attributes_;
}

bool validateShaderURI(BaseContext& context, const ValueHandle& uri, bool isNonEmptyRequired) {
	if (uri.asString().empty() && !isNonEmptyRequired) {
		context.errors().removeError(uri);
		return false;
	}

	return validateURI(context, uri);
}

std::string Material::loadShader(const Project& project, const ValueHandle& uri) {
	const std::string shaderPath = PathQueries::resolveUriPropertyToAbsolutePath(project, uri);
	const ShaderPreprocessor preprocessor(shaderPath);
	return preprocessor.getProcessedShader();
}

std::tuple<bool, std::string> Material::validateShader(BaseContext& context, const ValueHandle& uriHandle, bool isNonEmptyUriRequired) {
	// Clear old listeners associated with shader.
	const auto shaderPath{PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), uriHandle)};
	std::set pathsToWatch{shaderPath};
	std::string shaderCode;
	auto isPreprocessedShaderValid = false;

	// Check whether URI is specified and file exists.
	if (validateShaderURI(context, uriHandle, isNonEmptyUriRequired)) {
		// Preprocess and check for erroneous include directives.
		const auto preprocessor = ShaderPreprocessor{shaderPath};
		if (!preprocessor.hasError()) {
			isPreprocessedShaderValid = true;
			shaderCode = preprocessor.getProcessedShader();
			pathsToWatch.insert(preprocessor.getIncludedFiles().begin(), preprocessor.getIncludedFiles().end());
		} else {
			context.errors().addError(ErrorCategory::FILESYSTEM, ErrorLevel::ERROR, uriHandle, preprocessor.getError());
		}
	}

	recreatePropertyFileWatchers(context, uriHandle.getPropName(), pathsToWatch);

	return {isPreprocessedShaderValid, shaderCode};
}

void Material::updateFromExternalFile(BaseContext& context) {
	context.errors().removeError(ValueHandle{shared_from_this()});
	bool isUriDefinesValid = false;
	const auto definesUriHandle = ValueHandle{shared_from_this(), &Material::uriDefines_};
	if (uriDefines_.asString().empty() || (isUriDefinesValid = validateURI(context, definesUriHandle))) {
		context.errors().removeError(definesUriHandle);
	}

	isShaderProgramValid_ = false;
	PropertyInterfaceList uniforms;
	const auto [vertexValid, vertexShader] = validateShader(context, {shared_from_this(), &Material::uriVertex_});
	const auto [fragmentValid, fragmentShader] = validateShader(context, {shared_from_this(), &Material::uriFragment_});

	const ValueHandle geometryUriHandle{shared_from_this(), &Material::uriGeometry_};
	auto [geometryValid, geometryShader] = validateShader(context, geometryUriHandle,false);

	// Fragment and vertex shaders are required. Geometry shader is optional and must be valid only if specified.
	if (fragmentValid && vertexValid && (geometryValid || geometryUriHandle.asString().empty())) {
		if (geometryValid && geometryShader.empty()) {
			// Shader file exists but is empty. Trigger compilation by setting contents to single space.
			geometryShader = " ";
		}

		std::string shaderDefines;
		if (isUriDefinesValid) {
			shaderDefines = {raco::utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Material::uriDefines_}))};
			// ramses treats the empty string as no shader defines should be loaded and only shows errors if the string contains at least a single charcater.
			// We want to display errors when the file load was successful but the file is empty.
			if (shaderDefines.empty()) {
				shaderDefines = " ";
			}
		}

		std::string error{};
		isShaderProgramValid_ = context.engineInterface().parseShader(vertexShader, geometryShader, fragmentShader, shaderDefines, uniforms, attributes_, error);
		if (!error.empty()) {
			context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, ValueHandle{shared_from_this()}, error);
		}
	}
	if (!isShaderProgramValid_) {
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
