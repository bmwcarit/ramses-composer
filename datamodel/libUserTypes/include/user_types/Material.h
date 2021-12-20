/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/EngineInterface.h"
#include "user_types/BaseObject.h"
#include "user_types/DefaultValues.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include <map>

namespace raco::user_types {

class BlendOptions : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = {"BlendOptions", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	BlendOptions() : ClassWithReflectedMembers(getProperties()) {}

	BlendOptions(const BlendOptions& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) : ClassWithReflectedMembers(getProperties()),
		blendOperationColor_(other.blendOperationColor_),
		blendOperationAlpha_(other.blendOperationAlpha_),
		blendFactorSrcColor_(other.blendFactorSrcColor_),
		blendFactorDestColor_(other.blendFactorDestColor_),
		blendFactorSrcAlpha_(other.blendFactorSrcAlpha_),
		blendFactorDestAlpha_(other.blendFactorDestAlpha_),
		blendColor_(other.blendColor_),
		depthwrite_(other.depthwrite_),
		depthFunction_(other.depthFunction_),
		cullmode_(other.cullmode_) {
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
			{"depthwrite", &depthwrite_},
			{"depthFunction", &depthFunction_},
			{"cullmode", &cullmode_}};
	}

	Property<bool, DisplayNameAnnotation> depthwrite_{true, DisplayNameAnnotation("Depth Write")};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> depthFunction_{DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION, DisplayNameAnnotation("Depth Function"), EnumerationAnnotation{EngineEnumeration::DepthFunction}};

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> cullmode_{DEFAULT_VALUE_MATERIAL_CULL_MODE, DisplayNameAnnotation("Cull Mode"), EnumerationAnnotation{EngineEnumeration::CullMode}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationColor_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EngineEnumeration::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EngineEnumeration::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EngineEnumeration::BlendFactor}};
	Property<Vec4f, DisplayNameAnnotation> blendColor_{{}, {"Blend Color"}};
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

	Property<Table, ArraySemanticAnnotation, TagContainerAnnotation, DisplayNameAnnotation> tags_{{}, {}, {}, {"Tags"}};

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriVertex_{std::string(), {"Vertex shader files(*.glsl *.vert)"}, DisplayNameAnnotation("Vertex URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriGeometry_{std::string(), {"Geometry shader files(*.glsl *.geom)"}, DisplayNameAnnotation("Geometry URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriFragment_{std::string(), {"Fragment shader files(*.glsl *.frag)"}, DisplayNameAnnotation("Fragment URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriDefines_{std::string(), {"Shader define files(*.def)"}, DisplayNameAnnotation("Defines URI")};

	Property<BlendOptions, DisplayNameAnnotation> options_{{}, {"Options"}};

	Property<Table, DisplayNameAnnotation> uniforms_{{}, DisplayNameAnnotation("Uniforms")};

	bool isShaderValid() const {
		return isShaderValid_;
	}

	const PropertyInterfaceList& attributes() const;

private:
	void syncUniforms(BaseContext& context);

	bool isShaderValid_ = false;

	PropertyInterfaceList attributes_;

	// Cached values of outdated uniforms.
	OutdatedPropertiesStore cachedUniformValues_;
};

using SMaterial = std::shared_ptr<Material>;

}  // namespace raco::user_types
