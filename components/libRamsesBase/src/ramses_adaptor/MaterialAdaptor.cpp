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
#include "ramses_base/Utils.h"
#include "user_types/Material.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "utils/FileUtils.h"
#include "ramses_base/Utils.h"

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
		auto uniformsHandle = raco::core::ValueHandle(editorObject(), &raco::user_types::Material::uniforms_);
		return uniformsHandle.contains(handle);
	});

	bool status = TypedObjectAdaptor<user_types::Material, ramses::Effect>::sync(errors);

	appearanceBinding_.reset();
	appearance_.reset();

	if (editorObject()->isShaderValid()) {
		std::string const vertexShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriVertex_}));
		std::string const fragmentShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriFragment_}));
		std::string const geometryShader = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriGeometry_}));
		std::string const shaderDefines = utils::file::read(raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Material::uriDefines_}));
		auto const effectDescription = raco::ramses_base::createEffectDescription(vertexShader, geometryShader, fragmentShader, shaderDefines);
		reset(raco::ramses_base::ramsesEffect(sceneAdaptor_->scene(), *effectDescription));
	} else {
		reset(createEffect(sceneAdaptor_));
	}

	appearance_ = raco::ramses_base::ramsesAppearance(sceneAdaptor_->scene(), getRamsesObjectPointer());
	(*appearance_)->setName(std::string(this->editorObject()->objectName() + "_Appearance").c_str());

	// Only create appearance binding and set uniforms & blend options if we are using a valid shader but not if
	// we are using the empty default shaders.
	if (editorObject()->isShaderValid()) {
		core::ValueHandle optionsHandle = {editorObject(), &user_types::Material::options_};
		core::ValueHandle uniformsHandle = {editorObject(), &user_types::Material::uniforms_};
		updateAppearance(errors, sceneAdaptor_, appearance_, optionsHandle, uniformsHandle);

		appearanceBinding_ = raco::ramses_base::ramsesAppearanceBinding(*appearance_->get(), &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_AppearanceBinding", editorObject_->objectIDAsRamsesLogicID());
	}

	tagDirty(false);
	return true;
}

void MaterialAdaptor::getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const {
	if (appearanceBinding_) {
		logicNodes.push_back(appearanceBinding_.get());
	}
}

const rlogic::Property* MaterialAdaptor::getProperty(const std::vector<std::string>& names) {
	if (appearanceBinding_ && names.size() > 1 && names[0] == "uniforms") {
		auto ramsesPropNames = ramses_base::getRamsesUniformPropertyNames(core::ValueHandle(editorObject(), &raco::user_types::Material::uniforms_), names, 1);
		return ILogicPropertyProvider::getPropertyRecursive(appearanceBinding_->getInputs(), ramsesPropNames, 0);
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
		result.emplace_back("AppearanceBinding", appearanceBinding_->getName().data());
	}

	return result;
}

template <typename PropertyType, typename RamsesType>
std::vector<RamsesType> flattenUniformArrayOfVector(const core::ValueHandle& handle, int numComponents) {
	std::vector<RamsesType> values;
	for (size_t index = 0; index < handle.size(); index++) {
		for (size_t component = 0; component < numComponents; component++) {
			values.emplace_back(handle[index][component].as<PropertyType>());
		}
	}
	return values;
}

inline void setUniform(core::Errors* errors, SceneAdaptor* sceneAdaptor, ramses::Appearance* appearance, const core::ValueHandle& valueHandle,
	const std::string& uniformEngineName, 
	std::vector<raco::ramses_base::RamsesTextureSampler> newSamplers,
	std::vector<raco::ramses_base::RamsesTextureSamplerMS> newSamplersMS,
	std::vector<raco::ramses_base::RamsesTextureSamplerExternal> newSamplersExternal) {

	ramses::UniformInput input;
	appearance->getEffect().findUniformInput(uniformEngineName.c_str(), input);

	auto engineTypeAnno = valueHandle.query<raco::user_types::EngineTypeAnnotation>();
	switch (engineTypeAnno->type()) {
		case core::EnginePrimitive::Double:
			appearance->setInputValueFloat(input, valueHandle.as<float>());
			break;
		case core::EnginePrimitive::Int32:
		case core::EnginePrimitive::UInt16:
		case core::EnginePrimitive::UInt32:
			appearance->setInputValueInt32(input, static_cast<int32_t>(valueHandle.as<int>()));
			break;

		case core::EnginePrimitive::Vec2f:
			appearance->setInputValueVector2f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>());
			break;

		case core::EnginePrimitive::Vec3f:
			appearance->setInputValueVector3f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>());
			break;

		case core::EnginePrimitive::Vec4f:
			appearance->setInputValueVector4f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>(), valueHandle[3].as<float>());
			break;

		case core::EnginePrimitive::Vec2i:
			appearance->setInputValueVector2i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()));
			break;

		case core::EnginePrimitive::Vec3i:
			appearance->setInputValueVector3i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()), static_cast<int32_t>(valueHandle[2].as<int>()));
			break;

		case core::EnginePrimitive::Vec4i:
			appearance->setInputValueVector4i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()), static_cast<int32_t>(valueHandle[2].as<int>()), static_cast<int32_t>(valueHandle[3].as<int>()));
			break;

		case core::EnginePrimitive::Array: {
			assert(valueHandle.size() >= 1);

			auto elementAnno = valueHandle[0].query<user_types::EngineTypeAnnotation>();
			switch (elementAnno->type()) {
				case core::EnginePrimitive::Double: {
					std::vector<float> values;
					for (size_t index = 0; index < valueHandle.size(); index++) {
						values.emplace_back(valueHandle[index].as<float>());
					}
					appearance->setInputValueFloat(input, values.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Int32:
				case core::EnginePrimitive::UInt16:
				case core::EnginePrimitive::UInt32: {
					std::vector<int32_t> values;
					for (size_t index = 0; index < valueHandle.size(); index++) {
						values.emplace_back(valueHandle[index].as<int>());
					}
					appearance->setInputValueInt32(input, values.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec2f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 2)};
					appearance->setInputValueVector2f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec3f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 3)};
					appearance->setInputValueVector3f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec4f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 4)};
					appearance->setInputValueVector4f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec2i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 2)};
					appearance->setInputValueVector2i(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec3i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 3)};
					appearance->setInputValueVector3i(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec4i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 4)};
					appearance->setInputValueVector4i(input, valueHandle.size(), values.data());
					break;
				}

				default:
					break;
			}
		} break;

		case raco::core::EnginePrimitive::TextureSampler2DMS: {
			if (auto buffer = valueHandle.asTypedRef<user_types::RenderBufferMS>()) {
				if (auto adaptor = sceneAdaptor->lookup<RenderBufferMSAdaptor>(buffer)) {
					if (auto samplerMS = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *samplerMS);
						newSamplersMS.emplace_back(samplerMS);
					} else {
						errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "Sampler for this RenderBufferMS not available.");
					}
				}
			} else {
				errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "RenderBufferMS needed for this uniform.");
			}
		} break;

		case raco::core::EnginePrimitive::TextureSamplerExternal: {
			if (auto texture = valueHandle.asTypedRef<user_types::TextureExternal>()) {
				if (auto adaptor = sceneAdaptor->lookup<TextureExternalAdaptor>(texture)) {
					if (auto sampler = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *sampler);
						newSamplersExternal.emplace_back(sampler);
					} else {
						errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "Sampler for this TextureExternal not available.");
					}
				}
			} else {
				errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "TextureExternal needed for this uniform.");
			}
		} break;

		case raco::core::EnginePrimitive::TextureSampler2D: {
			raco::ramses_base::RamsesTextureSampler sampler = nullptr;
			if (auto texture = valueHandle.asTypedRef<user_types::Texture>()) {
				if (auto adaptor = sceneAdaptor->lookup<TextureSamplerAdaptor>(texture)) {
					sampler = adaptor->getRamsesObjectPointer();
				}
			} else if (auto buffer = valueHandle.asTypedRef<user_types::RenderBuffer>()) {
				if (auto adaptor = sceneAdaptor->lookup<RenderBufferAdaptor>(buffer)) {
					sampler = adaptor->getRamsesObjectPointer();
					if (!sampler) {
						errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "Sampler for this RenderBuffer not available.");
					}
				}
			} else {
				errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "Texture or RenderBuffer needed for this uniform.");
			}
			if (sampler) {
				appearance->setInputTexture(input, *sampler);
				newSamplers.emplace_back(sampler);
			}
		} break;

		case raco::core::EnginePrimitive::TextureSamplerCube: {
			if (auto texture = valueHandle.asTypedRef<user_types::CubeMap>()) {
				if (auto adaptor = sceneAdaptor->lookup<CubeMapAdaptor>(texture)) {
					if (auto sampler = adaptor->getRamsesObjectPointer()) {
						appearance->setInputTexture(input, *sampler);
						newSamplers.emplace_back(sampler);
					}
				}
			} else {
				errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, valueHandle, "CubeMap needed for this uniform.");
			}
		} break;

		default:
			break;
	}
}

inline void setUniformRecursive(core::Errors* errors, SceneAdaptor* sceneAdaptor, ramses::Appearance* appearance, const core::ValueHandle& uniformContainerHandle, const core::ValueHandle& handle,
	std::vector<raco::ramses_base::RamsesTextureSampler> newSamplers,
	std::vector<raco::ramses_base::RamsesTextureSamplerMS> newSamplersMS,
	std::vector<raco::ramses_base::RamsesTextureSamplerExternal> newSamplersExternal) {
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
				setUniform(errors, sceneAdaptor, appearance, handle, ramses_base::getRamsesUniformPropertyName(uniformContainerHandle, handle), newSamplers, newSamplersMS, newSamplersExternal);
			}

		} break;

		default:
			setUniform(errors, sceneAdaptor, appearance, handle, ramses_base::getRamsesUniformPropertyName(uniformContainerHandle, handle), newSamplers, newSamplersMS, newSamplersExternal);
	}
}

void updateAppearance(core::Errors* errors, SceneAdaptor* sceneAdaptor, raco::ramses_base::RamsesAppearance appearance, const core::ValueHandle& optionsHandle, const core::ValueHandle& uniformsHandle) {
	auto colorOp = static_cast<user_types::EBlendOperation>(optionsHandle.get("blendOperationColor").as<int>());
	auto alphaOp = static_cast<user_types::EBlendOperation>(optionsHandle.get("blendOperationAlpha").as<int>());
	if (colorOp == user_types::EBlendOperation::Disabled && alphaOp != user_types::EBlendOperation::Disabled) {
		errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, optionsHandle.get("blendOperationAlpha"),
			"Inconsistent Blend Operation settings: Color disabled while Alpha is not.");
	} else {
		errors->removeError(optionsHandle.get("blendOperationAlpha"));
	}
	if (colorOp != user_types::EBlendOperation::Disabled && alphaOp == user_types::EBlendOperation::Disabled) {
		errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, optionsHandle.get("blendOperationColor"),
			"Inconsistent Blend Operation settings: Alpha disabled while Color is not.");
	} else {
		errors->removeError(optionsHandle.get("blendOperationColor"));
	}

	setDepthWrite(appearance->get(), optionsHandle.get("depthwrite"));
	setDepthFunction(appearance->get(), optionsHandle.get("depthFunction"));
	setBlendMode(appearance->get(), optionsHandle);
	setBlendColor(appearance->get(), optionsHandle.get("blendColor"));
	setCullMode(appearance->get(), optionsHandle.get("cullmode"));

	std::vector<raco::ramses_base::RamsesTextureSampler> newSamplers;
	std::vector<raco::ramses_base::RamsesTextureSamplerMS> newSamplersMS;
	std::vector<raco::ramses_base::RamsesTextureSamplerExternal> newSamplersExternal;

	for (size_t i{0}; i < uniformsHandle.size(); i++) {
		setUniformRecursive(errors, sceneAdaptor, appearance->get(), uniformsHandle, uniformsHandle[i], newSamplers, newSamplersMS, newSamplersExternal);
	}
	appearance->replaceTrackedSamplers(newSamplers, newSamplersMS, newSamplersExternal);
}

};	// namespace raco::ramses_adaptor
