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
#include "core/FileChangeMonitor.h"
#include "user_types/BaseObject.h"
#include "user_types/DefaultValues.h"
#include "user_types/SyncTableWithEngineInterface.h"

#include <map>

namespace raco::user_types {

class Material : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"Material", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Material(Material const& other) : BaseObject(other),
									  uriVertex_(other.uriVertex_),
									  uriGeometry_(other.uriGeometry_),
									  uriFragment_(other.uriFragment_),
									  uriDefines_(other.uriDefines_),
									  blendOperationColor_(other.blendOperationColor_),
									  blendOperationAlpha_(other.blendOperationAlpha_),
									  blendFactorSrcColor_(other.blendFactorSrcColor_),
									  blendFactorDestColor_(other.blendFactorDestColor_),
									  blendFactorSrcAlpha_(other.blendFactorSrcAlpha_),
									  blendFactorDestAlpha_(other.blendFactorDestAlpha_),
									  depthwrite_(other.depthwrite_),
									  cullmode_(other.cullmode_),
									  uniforms_(other.uniforms_) {
		fillPropertyDescription();
	}

	Material(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("uriVertex", &uriVertex_);
		properties_.emplace_back("uriGeometry", &uriGeometry_);
		properties_.emplace_back("uriFragment", &uriFragment_);
		properties_.emplace_back("uriDefines", &uriDefines_);
		properties_.emplace_back("blendOperationColor", &blendOperationColor_);
		properties_.emplace_back("blendOperationAlpha", &blendOperationAlpha_);
		properties_.emplace_back("blendFactorSrcColor", &blendFactorSrcColor_);
		properties_.emplace_back("blendFactorDestColor", &blendFactorDestColor_);
		properties_.emplace_back("blendFactorSrcAlpha", &blendFactorSrcAlpha_);
		properties_.emplace_back("blendFactorDestAlpha", &blendFactorDestAlpha_);
		properties_.emplace_back("blendColor", &blendColor_);
		properties_.emplace_back("depthwrite", &depthwrite_);
		properties_.emplace_back("depthFunction", &depthFunction_);
		properties_.emplace_back("cullmode", &cullmode_);
		properties_.emplace_back("uniforms", &uniforms_);
	}

	void onBeforeDeleteObject(Errors& errors) const override;

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void onAfterContextActivated(BaseContext& context) override;

	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriVertex_{std::string(), {"Vertex shader files(*.glsl *.vert)"}, DisplayNameAnnotation("Vertex URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriGeometry_{std::string(), {"Geometry shader files(*.glsl *.geom)"}, DisplayNameAnnotation("Geometry URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriFragment_{std::string(), {"Fragment shader files(*.glsl *.frag)"}, DisplayNameAnnotation("Fragment URI")};
	Property<std::string, URIAnnotation, DisplayNameAnnotation> uriDefines_{std::string(), {"Shader define files(*.def)"}, DisplayNameAnnotation("Defines URI")};

	Property<bool, DisplayNameAnnotation> depthwrite_{true, DisplayNameAnnotation("Depth Write")};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> depthFunction_{DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION, DisplayNameAnnotation("Depth Function"), EnumerationAnnotation{EngineEnumeration::DepthFunction}};
	

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> cullmode_{DEFAULT_VALUE_MATERIAL_CULL_MODE, DisplayNameAnnotation("Cull Mode"), EnumerationAnnotation{EngineEnumeration::CullMode}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationColor_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR, {"Blend Operation Color"}, {EngineEnumeration::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendOperationAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA, {"Blend Operation Alpha"}, {EngineEnumeration::BlendOperation}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR, {"Blend Factor Src Color"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestColor_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR, {"Blend Factor Dest Color"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorSrcAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA, {"Blend Factor Src Alpha"}, {EngineEnumeration::BlendFactor}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> blendFactorDestAlpha_{DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA, {"Blend Factor Dest Alpha"}, {EngineEnumeration::BlendFactor}};
	Property<Vec4f, DisplayNameAnnotation> blendColor_ { {}, { "Blend Color"} };

	Property<Table, DisplayNameAnnotation> uniforms_{{}, DisplayNameAnnotation("Uniforms")};

	bool isShaderValid() const {
		return isShaderValid_;
	}

	const PropertyInterfaceList& attributes() const;

private:
	void syncUniforms(BaseContext& context);

	mutable FileChangeMonitor::UniqueListener vertexListener_;
	mutable FileChangeMonitor::UniqueListener geometryListener_;
	mutable FileChangeMonitor::UniqueListener fragmentListener_;
	mutable FileChangeMonitor::UniqueListener definesListener_;

	bool isShaderValid_ = false;

	PropertyInterfaceList attributes_;

	// Cached values of outdated uniforms.
	OutdatedPropertiesStore cachedUniformValues_;
};

using SMaterial = std::shared_ptr<Material>;

}  // namespace raco::user_types
