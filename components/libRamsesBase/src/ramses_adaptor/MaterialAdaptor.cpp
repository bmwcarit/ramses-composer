/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <utility>

#include "ramses_adaptor/MaterialAdaptor.h"

#include "ramses_adaptor/CubeMapAdaptor.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_adaptor/RenderBufferMSAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TextureExternalAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/Utils.h"
#include "user_types/Material.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "utils/FileUtils.h"

#include "user_types/EngineTypeAnnotation.h"

namespace raco::ramses_adaptor {

constexpr const char* emptyVertexShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

constexpr const char* emptyFragmentShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

ramses_base::RamsesEffect MaterialAdaptor::createEffect(SceneAdaptor* sceneAdaptor) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(emptyVertexShader);
	effectDescription.setFragmentShader(emptyFragmentShader);
	return ramses_base::ramsesEffect(sceneAdaptor->scene(), effectDescription, {}, {0, 0});
}

MaterialAdaptor::MaterialAdaptor(SceneAdaptor* sceneAdaptor, user_types::SMaterial material)
	: TypedObjectAdaptor{sceneAdaptor, material, createEffect(sceneAdaptor)},
	  subscription_{sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject(), [this]() {
		  tagDirty();
	  })},
	  optionsSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject(), &user_types::Material::options_}, [this](auto) {
		  tagDirty();
	  })},
	  uniformSubscription_{sceneAdaptor_->dispatcher()->registerOnChildren({editorObject(), &user_types::Material::uniforms_}, [this](auto) {
		  tagDirty();
	  })} {
}

bool MaterialAdaptor::isValid() {
	return editorObject()->isShaderValid();
}

bool MaterialAdaptor::sync(core::Errors* errors) {
	errors->removeIf([this](core::ErrorItem const& error) {
		auto handle = error.valueHandle();
		auto uniformsHandle = core::ValueHandle(editorObject(), &user_types::Material::uniforms_);
		return uniformsHandle.contains(handle);
	});

	TypedObjectAdaptor::sync(errors);

	appearanceBinding_.reset();
	appearance_.reset();

	if (editorObject()->isShaderValid()) {
		std::string const vertexShader{user_types::Material::loadShader(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriVertex_})};
		std::string const fragmentShader{user_types::Material::loadShader(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriFragment_})};
		std::string const geometryShader{user_types::Material::loadShader(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriGeometry_})};
		std::string const shaderDefines = utils::file::read(core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriDefines_}));
		auto const effectDescription = ramses_base::createEffectDescription(vertexShader, geometryShader, fragmentShader, shaderDefines);
		reset(ramses_base::ramsesEffect(sceneAdaptor_->scene(), *effectDescription, {}, editorObject_->objectIDAsRamsesLogicID()));
	} else {
		reset(createEffect(sceneAdaptor_));
	}

	appearance_ = ramses_base::ramsesAppearance(sceneAdaptor_->scene(), getRamsesObjectPointer(), editorObject_->objectIDAsRamsesLogicID());
	(*appearance_)->setName(std::string(this->editorObject()->objectName() + "_Appearance").c_str());

	// Only create appearance binding and set uniforms & blend options if we are using a valid shader but not if
	// we are using the empty default shaders.
	if (editorObject()->isShaderValid()) {
		appearanceBinding_ = ramses_base::ramsesAppearanceBinding(*appearance_->get(), &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_AppearanceBinding", editorObject_->objectIDAsRamsesLogicID());

		core::ValueHandle optionsHandle = {editorObject(), &user_types::Material::options_};
		core::ValueHandle uniformsHandle = {editorObject(), &user_types::Material::uniforms_};
		updateAppearance(errors, sceneAdaptor_, appearance_, *editorObject()->options_, optionsHandle, uniformsHandle);
	}

	tagDirty(false);
	return true;
}

void MaterialAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	if (appearanceBinding_) {
		logicNodes.push_back(appearanceBinding_.get());
	}
}

ramses::Property* MaterialAdaptor::getProperty(const std::vector<std::string_view>& names) {
	if (appearanceBinding_ && names.size() > 1 && names[0] == "uniforms") {
		auto ramsesPropNames = ramses_base::getRamsesUniformPropertyNames(core::ValueHandle(editorObject(), &user_types::Material::uniforms_), names, 1);
		return ILogicPropertyProvider::getPropertyRecursive(appearanceBinding_->getInputs(), {ramsesPropNames.begin(), ramsesPropNames.end()}, 0);
	}
	return nullptr;
}

void MaterialAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject()};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

std::vector<ExportInformation> MaterialAdaptor::getExportInformation() const {
	std::vector<ExportInformation> result = {};
	if (appearance_ != nullptr) {
		if (appearance_->effect() != nullptr) {
			result.emplace_back(appearance_->effect()->getType(), appearance_->effect()->getName());
		}
		result.emplace_back(appearance_->get()->getType(), appearance_->get()->getName());
	}

	if (appearanceBinding_ != nullptr) {
		result.emplace_back("AppearanceBinding", appearanceBinding_->getName());
	}

	return result;
}

template <typename RamsesType>
std::vector<RamsesType> getArrayData(const core::ValueHandle& handle) {
	std::vector<RamsesType> result;
	for (size_t index = 0; index < handle.size(); index++) {
		result.emplace_back(RamsesType(handle[index].as<RamsesType>()));
	}
	return result;
}

inline void setUniform(core::Errors* errors, SceneAdaptor* sceneAdaptor, ramses::Appearance* appearance, const core::ValueHandle& valueHandle,
	const std::string& uniformEngineName, 
	std::vector<ramses_base::RamsesTextureSampler> newSamplers,
	std::vector<ramses_base::RamsesTextureSamplerMS> newSamplersMS,
	std::vector<ramses_base::RamsesTextureSamplerExternal> newSamplersExternal) {

	ramses::UniformInput input = appearance->getEffect().findUniformInput(uniformEngineName.c_str()).value();
	
	auto engineTypeAnno = valueHandle.query<user_types::EngineTypeAnnotation>();
	switch (engineTypeAnno->type()) {
		case core::EnginePrimitive::Bool:
			appearance->setInputValue<bool>(input, valueHandle.as<bool>());
			break;

		case core::EnginePrimitive::Double:
			appearance->setInputValue<float>(input, valueHandle.as<float>());
			break;
		case core::EnginePrimitive::Int32:
		case core::EnginePrimitive::UInt16:
		case core::EnginePrimitive::UInt32:
			appearance->setInputValue<int32_t>(input, static_cast<int32_t>(valueHandle.as<int>()));
			break;

		case core::EnginePrimitive::Vec2f:
			appearance->setInputValue<ramses::vec2f>(input, ramses::vec2f(valueHandle[0].as<float>(), valueHandle[1].as<float>()));
			break;

		case core::EnginePrimitive::Vec3f:
			appearance->setInputValue<ramses::vec3f>(input, ramses::vec3f(valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>()));
			break;

		case core::EnginePrimitive::Vec4f:
			appearance->setInputValue<ramses::vec4f>(input, ramses::vec4f(valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>(), valueHandle[3].as<float>()));
			break;

		case core::EnginePrimitive::Vec2i:
			appearance->setInputValue<ramses::vec2i>(input, ramses::vec2i(valueHandle[0].as<int>(), valueHandle[1].as<int>()));
			break;

		case core::EnginePrimitive::Vec3i:
			appearance->setInputValue<ramses::vec3i>(input, ramses::vec3i(valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>()));
			break;

		case core::EnginePrimitive::Vec4i:
			appearance->setInputValue<ramses::vec4i>(input, ramses::vec4i(valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>(), valueHandle[3].as<int>()));
			break;

		case core::EnginePrimitive::Array:
			// no-op since arrays of primitive type are now set via the binding, see setUniformRecursive below.
			break;

		case core::EnginePrimitive::TextureSampler2DMS: {
			if (auto buffer = valueHandle.asTypedRef<user_types::RenderBufferMS>()) {
				if (auto adaptor = sceneAdaptor->lookup<RenderBufferMSAdaptor>(buffer)) {
					if (auto samplerMS = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *samplerMS);
						newSamplersMS.emplace_back(samplerMS);
					} else {
						errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "Sampler for this RenderBufferMS not available.");
					}
				}
			} else {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "RenderBufferMS needed for this uniform.");
			}
		} break;

		case core::EnginePrimitive::TextureSamplerExternal: {
			if (auto texture = valueHandle.asTypedRef<user_types::TextureExternal>()) {
				if (auto adaptor = sceneAdaptor->lookup<TextureExternalAdaptor>(texture)) {
					if (auto sampler = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *sampler);
						newSamplersExternal.emplace_back(sampler);
					} else {
						errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "Sampler for this TextureExternal not available.");
					}
				}
			} else {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "TextureExternal needed for this uniform.");
			}
		} break;

		case core::EnginePrimitive::TextureSampler2D: {
			ramses_base::RamsesTextureSampler sampler = nullptr;
			if (auto texture = valueHandle.asTypedRef<user_types::Texture>()) {
				if (auto adaptor = sceneAdaptor->lookup<TextureSamplerAdaptor>(texture)) {
					sampler = adaptor->getRamsesObjectPointer();
				}
			} else if (auto buffer = valueHandle.asTypedRef<user_types::RenderBuffer>()) {
				if (auto adaptor = sceneAdaptor->lookup<RenderBufferAdaptor>(buffer)) {
					sampler = adaptor->getRamsesObjectPointer();
					if (!sampler) {
						errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "Sampler for this RenderBuffer not available.");
					}
				}
			} else {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "Texture or RenderBuffer needed for this uniform.");
			}
			if (sampler) {
				appearance->setInputTexture(input, *sampler);
				newSamplers.emplace_back(sampler);
			}
		} break;

		case core::EnginePrimitive::TextureSamplerCube: {
			if (auto texture = valueHandle.asTypedRef<user_types::CubeMap>()) {
				if (auto adaptor = sceneAdaptor->lookup<CubeMapAdaptor>(texture)) {
					if (auto sampler = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *sampler);
						newSamplers.emplace_back(sampler);
					}
				}
			} else {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, valueHandle, "CubeMap needed for this uniform.");
			}
		} break;

		default:
			break;
	}
}

inline void setUniformRecursive(core::Errors* errors, SceneAdaptor* sceneAdaptor, ramses::Appearance* appearance, const core::ValueHandle& uniformContainerHandle, const core::ValueHandle& handle,
	std::vector<ramses_base::RamsesTextureSampler> newSamplers,
	std::vector<ramses_base::RamsesTextureSamplerMS> newSamplersMS,
	std::vector<ramses_base::RamsesTextureSamplerExternal> newSamplersExternal) {
	auto engineTypeAnno = handle.query<user_types::EngineTypeAnnotation>();
	switch (engineTypeAnno->type()) {
		case core::EnginePrimitive::Struct:
			for (size_t index = 0; index < handle.size(); index++) {
				setUniformRecursive(errors, sceneAdaptor, appearance, uniformContainerHandle, handle[index], newSamplers, newSamplersMS, newSamplersExternal);
			}
			break;

		case core::EnginePrimitive::Array: {
			auto elementAnno = handle[0].query<user_types::EngineTypeAnnotation>();
			if (elementAnno->type() == core::EnginePrimitive::Struct) {
				for (size_t index = 0; index < handle.size(); index++) {
					setUniformRecursive(errors, sceneAdaptor, appearance, uniformContainerHandle, handle[index], newSamplers, newSamplersMS, newSamplersExternal);
				}
			} else {
				// uniform arrays of primitive type must be set via the AppearanceBinding since the binding
				// will perform an atomic update of the entire array if even a single array element changed by a link.
				auto adaptor = sceneAdaptor->lookupAdaptor(handle.rootObject());
				auto prop = dynamic_cast<ILogicPropertyProvider*>(adaptor)->getProperty(handle.getPropertyNamesVector());
				setLuaInputInEngine(prop, handle);
			}

		} break;

		default:
			setUniform(errors, sceneAdaptor, appearance, handle, ramses_base::getRamsesUniformPropertyName(uniformContainerHandle, handle), newSamplers, newSamplersMS, newSamplersExternal);
	}
}

void updateAppearance(core::Errors* errors, SceneAdaptor* sceneAdaptor, ramses_base::RamsesAppearance appearance, user_types::BlendOptions& options, const core::ValueHandle& optionsHandle, const core::ValueHandle& uniformsHandle) {
	auto colorOp = static_cast<user_types::EBlendOperation>(*options.blendOperationColor_);
	auto alphaOp = static_cast<user_types::EBlendOperation>(*options.blendOperationAlpha_);
	if (colorOp == user_types::EBlendOperation::Disabled && alphaOp != user_types::EBlendOperation::Disabled) {
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, optionsHandle.get("blendOperationAlpha"),
			"Inconsistent Blend Operation settings: Color disabled while Alpha is not.");
	} else {
		errors->removeError(optionsHandle.get("blendOperationAlpha"));
	}
	if (colorOp != user_types::EBlendOperation::Disabled && alphaOp == user_types::EBlendOperation::Disabled) {
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, optionsHandle.get("blendOperationColor"),
			"Inconsistent Blend Operation settings: Alpha disabled while Color is not.");
	} else {
		errors->removeError(optionsHandle.get("blendOperationColor"));
	}

	(*appearance)->setDepthWrite(*options.depthwrite_ ? ramses::EDepthWrite::Enabled : ramses::EDepthWrite::Disabled);
	auto ramsesDepthFunc = ramses_base::enumerationTranslationsDepthFunc.at(static_cast<user_types::EDepthFunc>(*options.depthFunction_));
	(*appearance)->setDepthFunction(ramsesDepthFunc);

	auto ramsesColorOp = ramses_base::enumerationTranslationsBlendOperation.at(static_cast<user_types::EBlendOperation>(*options.blendOperationColor_));
	auto ramsesAlphaOp = ramses_base::enumerationTranslationsBlendOperation.at(static_cast<user_types::EBlendOperation>(*options.blendOperationAlpha_));
	(*appearance)->setBlendingOperations(ramsesColorOp, ramsesAlphaOp);

	auto ramsesSrcColor = ramses_base::enumerationTranslationsBlendFactor.at(static_cast<user_types::EBlendFactor>(*options.blendFactorSrcColor_));
	auto ramsesDestColor = ramses_base::enumerationTranslationsBlendFactor.at(static_cast<user_types::EBlendFactor>(*options.blendFactorDestColor_));
	auto ramsesSrcAlpha = ramses_base::enumerationTranslationsBlendFactor.at(static_cast<user_types::EBlendFactor>(*options.blendFactorSrcAlpha_));
	auto ramsesDestAlpha = ramses_base::enumerationTranslationsBlendFactor.at(static_cast<user_types::EBlendFactor>(*options.blendFactorDestAlpha_));
	(*appearance)->setBlendingFactors(ramsesSrcColor, ramsesDestColor, ramsesSrcAlpha, ramsesDestAlpha);

	(*appearance)->setBlendingColor(ramses::vec4f(*options.blendColor_->x, *options.blendColor_->y, *options.blendColor_->z, *options.blendColor_->w));

	auto ramsesCullMode = ramses_base::enumerationTranslationsCullMode.at(static_cast<user_types::ECullMode>(*options.cullmode_));
	(*appearance)->setCullingMode(ramsesCullMode);

	const auto& stencilOptions = *options.stencilOptions_;
	(*appearance)->setStencilFunction(
		static_cast<ramses::EStencilFunc>(*stencilOptions.stencilFunc_),
		std::clamp(*stencilOptions.stencilRef_, 0, 255),
		std::clamp(*stencilOptions.stencilMask_, 0, 255));
	(*appearance)->setStencilOperation(
		static_cast<ramses::EStencilOperation>(*stencilOptions.stencilOpStencilFail_), 
		static_cast<ramses::EStencilOperation>(*stencilOptions.stencilOpDepthFail_), 
		static_cast<ramses::EStencilOperation>(*stencilOptions.stencilOpDepthSucc_));

	const auto& scissorOptions = *options.scissorOptions_;
	(*appearance)->setScissorTest(
		(*scissorOptions.scissorEnable_ ? ramses::EScissorTest::Enabled : ramses::EScissorTest::Disabled),
		*scissorOptions.scissorRegion_->offsetX_, 
		*scissorOptions.scissorRegion_->offsetY_, 
		*scissorOptions.scissorRegion_->width_, 
		*scissorOptions.scissorRegion_->height_);

	(*appearance)->setColorWriteMask(
		*options.colorWriteMask_->red_, 
		*options.colorWriteMask_->green_, 
		*options.colorWriteMask_->blue_, 
		*options.colorWriteMask_->alpha_);

	std::vector<ramses_base::RamsesTextureSampler> newSamplers;
	std::vector<ramses_base::RamsesTextureSamplerMS> newSamplersMS;
	std::vector<ramses_base::RamsesTextureSamplerExternal> newSamplersExternal;

	for (size_t i{0}; i < uniformsHandle.size(); i++) {
		setUniformRecursive(errors, sceneAdaptor, appearance->get(), uniformsHandle, uniformsHandle[i], newSamplers, newSamplersMS, newSamplersExternal);
	}
	appearance->replaceTrackedSamplers(newSamplers, newSamplersMS, newSamplersExternal);
}

};	// namespace raco::ramses_adaptor
