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
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_adaptor/TextureExternalAdaptor.h"
#include "ramses_base/Utils.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/Material.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "utils/FileUtils.h"

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
		return ILogicPropertyProvider::getPropertyRecursive(appearanceBinding_->getInputs(), names, 1);
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

void updateAppearance(core::Errors* errors, SceneAdaptor* sceneAdaptor, raco::ramses_base::RamsesAppearance appearance, const core::ValueHandle& optionsHandle, const core::ValueHandle& uniformsHandle) {
	int colorOp = static_cast<ramses::EBlendOperation>(optionsHandle.get("blendOperationColor").as<int>());
	int alphaOp = static_cast<ramses::EBlendOperation>(optionsHandle.get("blendOperationAlpha").as<int>());
	if (colorOp == ramses::EBlendOperation::EBlendOperation_Disabled && alphaOp != ramses::EBlendOperation::EBlendOperation_Disabled) {
		errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, optionsHandle.get("blendOperationAlpha"),
			"Inconsistent Blend Operation settings: Color disabled while Alpha is not.");
	} else {
		errors->removeError(optionsHandle.get("blendOperationAlpha"));
	}
	if (colorOp != ramses::EBlendOperation::EBlendOperation_Disabled && alphaOp == ramses::EBlendOperation::EBlendOperation_Disabled) {
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
		setUniform(appearance->get(), uniformsHandle[i]);

		if (uniformsHandle[i].type() == core::PrimitiveType::Ref) {
			auto anno = uniformsHandle[i].query<user_types::EngineTypeAnnotation>();
			auto engineType = anno->type();

			if (engineType == raco::core::EnginePrimitive::TextureSampler2DMS) {
				if (auto buffer = uniformsHandle[i].asTypedRef<user_types::RenderBufferMS>()) {
					if (auto adaptor = sceneAdaptor->lookup<RenderBufferMSAdaptor>(buffer)) {
						if (auto samplerMS = adaptor->getRamsesObjectPointer()) {
							ramses::UniformInput input;
							(*appearance)->getEffect().findUniformInput(uniformsHandle[i].getPropName().c_str(), input);
							(*appearance)->setInputTexture(input, *samplerMS);
							newSamplersMS.emplace_back(samplerMS);
						} else {
							errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "Sampler for this RenderBufferMS not available.");
						}
					}
				} else {
					errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "RenderBufferMS needed for this uniform.");
				}
			} else if (engineType == raco::core::EnginePrimitive::TextureSamplerExternal) {
				if (auto texture = uniformsHandle[i].asTypedRef<user_types::TextureExternal>()) {
					if (auto adaptor = sceneAdaptor->lookup<TextureExternalAdaptor>(texture)) {
						if (auto sampler = adaptor->getRamsesObjectPointer()) {
							ramses::UniformInput input;
							(*appearance)->getEffect().findUniformInput(uniformsHandle[i].getPropName().c_str(), input);
							(*appearance)->setInputTexture(input, *sampler);
							newSamplersExternal.emplace_back(sampler);
						} else {
							errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "Sampler for this TextureExternal not available.");
						}
					}
				} else {
					errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "TextureExternal needed for this uniform.");
				}
			} else {
				raco::ramses_base::RamsesTextureSampler sampler = nullptr;
				if (engineType == raco::core::EnginePrimitive::TextureSampler2D) {
					if (auto texture = uniformsHandle[i].asTypedRef<user_types::Texture>()) {
						if (auto adaptor = sceneAdaptor->lookup<TextureSamplerAdaptor>(texture)) {
							sampler = adaptor->getRamsesObjectPointer();
						}
					} else if (auto buffer = uniformsHandle[i].asTypedRef<user_types::RenderBuffer>()) {
						if (auto adaptor = sceneAdaptor->lookup<RenderBufferAdaptor>(buffer)) {
							sampler = adaptor->getRamsesObjectPointer();
							if (!sampler) {
								errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "Sampler for this RenderBuffer not available.");
							}
						}
					} else {
						errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "Texture or RenderBuffer needed for this uniform.");
					}
				} else if (engineType == raco::core::EnginePrimitive::TextureSamplerCube) {
					if (auto texture = uniformsHandle[i].asTypedRef<user_types::CubeMap>()) {
						if (auto adaptor = sceneAdaptor->lookup<CubeMapAdaptor>(texture)) {
							sampler = adaptor->getRamsesObjectPointer();
						}
					} else {
						errors->addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, uniformsHandle[i], "CubeMap needed for this uniform.");
					}
				}

				if (sampler) {
					ramses::UniformInput input;
					(*appearance)->getEffect().findUniformInput(uniformsHandle[i].getPropName().c_str(), input);
					(*appearance)->setInputTexture(input, *sampler);
					newSamplers.emplace_back(sampler);
				}
			}
		}
	}
	appearance->replaceTrackedSamplers(newSamplers, newSamplersMS, newSamplersExternal);
}

};	// namespace raco::ramses_adaptor
