/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/EngineInterface.h"
#include "user_types/BaseCamera.h"
#include "user_types/BaseObject.h"
#include "user_types/DefaultValues.h"
#include "user_types/Enumerations.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include <map>

namespace raco::user_types {

class ColorWriteMask : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"ColorWriteMask", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	ColorWriteMask() : StructBase(getProperties()) {}

	ColorWriteMask(const ColorWriteMask& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  red_(other.red_),
		  green_(other.green_),
		  blue_(other.blue_),
		  alpha_(other.alpha_) {
	}

	ColorWriteMask& operator=(const ColorWriteMask& other) {
		red_ = other.red_;
		green_ = other.green_;
		blue_ = other.blue_;
		alpha_ = other.alpha_;
		return *this;
	}

	void copyAnnotationData(const ColorWriteMask& other) {
		red_.copyAnnotationData(other.red_);
		green_.copyAnnotationData(other.green_);
		blue_.copyAnnotationData(other.blue_);
		alpha_.copyAnnotationData(other.alpha_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"red", &red_},
			{"green", &green_},
			{"blue", &blue_},
			{"alpha", &alpha_}};
	}

	Property<bool, DisplayNameAnnotation> red_{true, {"Red"}};
	Property<bool, DisplayNameAnnotation> green_{true, {"Green"}};
	Property<bool, DisplayNameAnnotation> blue_{true, {"Blue"}};
	Property<bool, DisplayNameAnnotation> alpha_{true, {"Alpha"}};
};

class StencilOptions : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"StencilOptions", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	StencilOptions() : StructBase(getProperties()) {}

	StencilOptions(const StencilOptions& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  stencilFunc_(other.stencilFunc_),
		  stencilRef_(other.stencilRef_),
		  stencilMask_(other.stencilMask_),
		  stencilOpStencilFail_(other.stencilOpStencilFail_),
		  stencilOpDepthFail_(other.stencilOpDepthFail_),
		  stencilOpDepthSucc_(other.stencilOpDepthSucc_) {
	}

	StencilOptions& operator=(const StencilOptions& other) {
		stencilFunc_ = other.stencilFunc_;
		stencilRef_ = other.stencilRef_;
		stencilMask_ = other.stencilMask_;
		stencilOpStencilFail_ = other.stencilOpStencilFail_;
		stencilOpDepthFail_ = other.stencilOpDepthFail_;
		stencilOpDepthSucc_ = other.stencilOpDepthSucc_;
		return *this;
	}

	void copyAnnotationData(const StencilOptions& other) {
		stencilFunc_.copyAnnotationData(other.stencilFunc_);
		stencilRef_.copyAnnotationData(other.stencilRef_);
		stencilMask_.copyAnnotationData(other.stencilMask_);
		stencilOpStencilFail_.copyAnnotationData(other.stencilOpStencilFail_);
		stencilOpDepthFail_.copyAnnotationData(other.stencilOpDepthFail_);
		stencilOpDepthSucc_.copyAnnotationData(other.stencilOpDepthSucc_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"stencilFunc", &stencilFunc_},
			{"stencilRef", &stencilRef_},
			{"stencilMask", &stencilMask_},
			{"stencilOpStencilFail", &stencilOpStencilFail_},
			{"stencilOpDepthFail", &stencilOpDepthFail_},
			{"stencilOpDepthSucc", &stencilOpDepthSucc_}};
	}

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> stencilFunc_{static_cast<int>(EStencilFunc::Disabled), {"Stencil Function"}, {EUserTypeEnumerations::StencilFunction}};
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> stencilRef_{1, {"Stencil Ref Value"}, {0, 255}};
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> stencilMask_{255, {"Stencil Mask Value"}, {0, 255}};

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> stencilOpStencilFail_{static_cast<int>(EStencilOperation::Keep), {"Stencil Operation Stencil Fail"}, {EUserTypeEnumerations::StencilOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> stencilOpDepthFail_{static_cast<int>(EStencilOperation::Keep), {"Stencil Operation Depth Fail"}, {EUserTypeEnumerations::StencilOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> stencilOpDepthSucc_{static_cast<int>(EStencilOperation::Keep), {"Stencil Operation Depth Succeed"}, {EUserTypeEnumerations::StencilOperation}};
};

class ScissorOptions : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"ScissorOptions", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	ScissorOptions() : StructBase(getProperties()) {}

	ScissorOptions(const ScissorOptions& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  scissorEnable_(other.scissorEnable_),
		  scissorRegion_(other.scissorRegion_) {
	}

	ScissorOptions& operator=(const ScissorOptions& other) {
		scissorEnable_ = other.scissorEnable_;
		scissorRegion_ = other.scissorRegion_;
		return *this;
	}

	void copyAnnotationData(const ScissorOptions& other) {
		scissorEnable_.copyAnnotationData(other.scissorEnable_);
		scissorRegion_.copyAnnotationData(other.scissorRegion_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"scissorEnable", &scissorEnable_},
			{"scissorRegion", &scissorRegion_}};
	}

	Property<bool, DisplayNameAnnotation> scissorEnable_{false, {"Scissor Enabled"}};
	Property<CameraViewport, DisplayNameAnnotation> scissorRegion_{{}, {"Scissor Region"}};
};

class BlendOptions : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"BlendOptions", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	BlendOptions() : StructBase(getProperties()) {}

	BlendOptions(const BlendOptions& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()),
		  blendOperationColor_(other.blendOperationColor_),
		  blendOperationAlpha_(other.blendOperationAlpha_),
		  blendFactorSrcColor_(other.blendFactorSrcColor_),
		  blendFactorDestColor_(other.blendFactorDestColor_),
		  blendFactorSrcAlpha_(other.blendFactorSrcAlpha_),
		  blendFactorDestAlpha_(other.blendFactorDestAlpha_),
		  blendColor_(other.blendColor_),
		  depthwrite_(other.depthwrite_),
		  depthFunction_(other.depthFunction_),
		  cullmode_(other.cullmode_),
		  colorWriteMask_(other.colorWriteMask_),
		  scissorOptions_(other.scissorOptions_),
		  stencilOptions_(other.stencilOptions_) {
	}

	BlendOptions& operator=(const BlendOptions& other) {
		blendOperationColor_ = other.blendOperationColor_;
		blendOperationAlpha_ = other.blendOperationAlpha_;
		blendFactorSrcColor_ = other.blendFactorSrcColor_;
		blendFactorDestColor_ = other.blendFactorDestColor_;
		blendFactorSrcAlpha_ = other.blendFactorSrcAlpha_;
		blendFactorDestAlpha_ = other.blendFactorDestAlpha_;
		blendColor_ = other.blendColor_;
		depthwrite_ = other.depthwrite_;
		depthFunction_ = other.depthFunction_;
		cullmode_ = other.cullmode_;
		colorWriteMask_ = other.colorWriteMask_;
		scissorOptions_ = other.scissorOptions_;
		stencilOptions_ = other.stencilOptions_;
		return *this;
	}

	void copyAnnotationData(const BlendOptions& other) {
		blendOperationColor_.copyAnnotationData(other.blendOperationColor_);
		blendOperationAlpha_.copyAnnotationData(other.blendOperationAlpha_);
		blendFactorSrcColor_.copyAnnotationData(other.blendFactorSrcColor_);
		blendFactorDestColor_.copyAnnotationData(other.blendFactorDestColor_);
		blendFactorSrcAlpha_.copyAnnotationData(other.blendFactorSrcAlpha_);
		blendFactorDestAlpha_.copyAnnotationData(other.blendFactorDestAlpha_);
		blendColor_.copyAnnotationData(other.blendColor_);
		depthwrite_.copyAnnotationData(other.depthwrite_);
		depthFunction_.copyAnnotationData(other.depthFunction_);
		cullmode_.copyAnnotationData(other.cullmode_);
		colorWriteMask_.copyAnnotationData(other.colorWriteMask_);
		scissorOptions_.copyAnnotationData(other.scissorOptions_);
		stencilOptions_.copyAnnotationData(other.stencilOptions_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"blendOperationColor", &blendOperationColor_},
			{"blendOperationAlpha", &blendOperationAlpha_},
			{"blendFactorSrcColor", &blendFactorSrcColor_},
			{"blendFactorDestColor", &blendFactorDestColor_},
			{"blendFactorSrcAlpha", &blendFactorSrcAlpha_},
			{"blendFactorDestAlpha", &blendFactorDestAlpha_},
			{"blendColor", &blendColor_},
			{"colorWriteMask", &colorWriteMask_},
			{"cullmode", &cullmode_},
			{"depthwrite", &depthwrite_},
			{"depthFunction", &depthFunction_},
			{"scissorOptions", &scissorOptions_},
			{"stencilOptions", &stencilOptions_}};
	}

	Property<bool, DisplayNameAnnotation> depthwrite_{true, DisplayNameAnnotation("Depth Write")};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> depthFunction_{DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION, DisplayNameAnnotation("Depth Function"), EnumerationAnnotation{EUserTypeEnumerations::DepthFunction}};

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> cullmode_{DEFAULT_VALUE_MATERIAL_CULL_MODE, DisplayNameAnnotation("Cull Mode"), EnumerationAnnotation{EUserTypeEnumerations::CullMode}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationColor_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EUserTypeEnumerations::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EUserTypeEnumerations::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EUserTypeEnumerations::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EUserTypeEnumerations::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EUserTypeEnumerations::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EUserTypeEnumerations::BlendFactor}};
	Property<Vec4f, DisplayNameAnnotation> blendColor_{{}, {"Blend Color"}};

	Property<ColorWriteMask, DisplayNameAnnotation> colorWriteMask_{{}, {"Color Write Mask"}};

	Property<ScissorOptions, DisplayNameAnnotation> scissorOptions_{{}, {"Scissor Options"}};

	Property<StencilOptions, DisplayNameAnnotation> stencilOptions_{{}, {"Stencil Options"}};
};

class Material : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"Material", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Material(Material const& other) : BaseObject(other),
									  tags_(other.tags_),
									  uriVertex_(other.uriVertex_),
									  uriGeometry_(other.uriGeometry_),
									  uriFragment_(other.uriFragment_),
									  uriDefines_(other.uriDefines_),
									  uniforms_(other.uniforms_) {
		fillPropertyDescription();
	}

	Material(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("tags", &tags_);
		properties_.emplace_back("uriVertex", &uriVertex_);
		properties_.emplace_back("uriGeometry", &uriGeometry_);
		properties_.emplace_back("uriFragment", &uriFragment_);
		properties_.emplace_back("uriDefines", &uriDefines_);
		properties_.emplace_back("options", &options_);
		properties_.emplace_back("uniforms", &uniforms_);
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void updateFromExternalFile(BaseContext& context) override;

	Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation> tags_{{}, {}, {}, {}, {"Tags"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriVertex_{std::string(), {"Vertex shader files(*.glsl *.vert);; All files (*.*)", core::PathManager::FolderTypeKeys::Shader}, DisplayNameAnnotation("Vertex URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriGeometry_{std::string(), {"Geometry shader files(*.glsl *.geom);; All files (*.*)", core::PathManager::FolderTypeKeys::Shader}, DisplayNameAnnotation("Geometry URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriFragment_{std::string(), {"Fragment shader files(*.glsl *.frag);; All files (*.*)", core::PathManager::FolderTypeKeys::Shader}, DisplayNameAnnotation("Fragment URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriDefines_{std::string(), {"Shader define files(*.def);; All files (*.*)", core::PathManager::FolderTypeKeys::Shader}, DisplayNameAnnotation("Defines URI")};

	Property<BlendOptions, DisplayNameAnnotation> options_{{}, {"Options"}};

	Property<Table, DisplayNameAnnotation> uniforms_{{}, DisplayNameAnnotation("Uniforms")};

	bool isShaderValid() const {
		return isShaderProgramValid_;
	}

	// Utility method to get preprocessed shader.
	static std::string loadShader(const Project& project, const ValueHandle& uri);

	const PropertyInterfaceList& attributes() const;

private:
	// Validity of shader program containing all shader stages.
	bool isShaderProgramValid_ = false;

	PropertyInterfaceList attributes_;

	// Cached values of outdated uniforms.
	OutdatedPropertiesStore cachedUniformValues_;

	enum class ShaderFileType {
		Vertex,
		Geometry,
		Fragment,
		Defines,
	};

	void autofillShaderIfFileExists(BaseContext& context, const std::string& fileNameWithoutEnding, bool endsWithDotGlsl, ShaderFileType shaderType);
	void autofillShaderFiles(BaseContext& context, const std::string& fileName, ShaderFileType setShaderProperty);
	bool areAllOtherShadersEmpty(ShaderFileType fileType);
	std::string getShaderFileEnding(ShaderFileType shader, bool endsWithDotGlsl);

	// Read, validate and update subscriptions.
	std::tuple<bool, std::string> validateShader(BaseContext& context, const ValueHandle& uriHandle, bool isNonEmptyUriRequired = true);
};

using SMaterial = std::shared_ptr<Material>;

}  // namespace raco::user_types
