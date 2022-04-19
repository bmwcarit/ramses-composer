/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/MaterialAdaptor.h"

#include "ramses_adaptor/CubeMapAdaptor.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_base/Utils.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/Material.h"
#include "user_types/RenderBuffer.h"
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

		appearanceBinding_ = raco::ramses_base::ramsesAppearanceBinding(*appearance_->get(), & sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_AppearanceBinding");
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
	if (names.size() > 1) {
		if (appearanceBinding_) {
			const rlogic::Property* prop{appearanceBinding_->getInputs()};
			// The first element in the names is the uniforms container
			for (size_t i = 1; i < names.size(); i++) {
				prop = prop->getChild(names.at(i));
			}
			return prop;
		}
		return nullptr;
	}
	return nullptr;
}

void MaterialAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject()};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR, level, valueHandle, message);
}

void updateAppearance(core::Errors* errors, SceneAdaptor* sceneAdaptor, raco::ramses_base::RamsesAppearance appearance, const core::ValueHandle& optionsHandle, const core::ValueHandle& uniformsHandle) {
	setDepthWrite(appearance->get(), optionsHandle.get("depthwrite"));
	setDepthFunction(appearance->get(), optionsHandle.get("depthFunction"));
	setBlendMode(appearance->get(), optionsHandle);
	setBlendColor(appearance->get(), optionsHandle.get("blendColor"));
	setCullMode(appearance->get(), optionsHandle.get("cullmode"));

	std::vector<raco::ramses_base::RamsesTextureSampler> newSamplers;

	for (size_t i{0}; i < uniformsHandle.size(); i++) {
		setUniform(appearance->get(), uniformsHandle[i]);

		if (uniformsHandle[i].type() == core::PrimitiveType::Ref) {
			auto anno = uniformsHandle[i].query<user_types::EngineTypeAnnotation>();
			auto engineType = anno->type();

			raco::ramses_base::RamsesTextureSampler sampler = nullptr;
			if (engineType == raco::core::EnginePrimitive::TextureSampler2D) {
				if (auto texture = uniformsHandle[i].asTypedRef<user_types::Texture>()) {
					if (auto adaptor = sceneAdaptor->lookup<TextureSamplerAdaptor>(texture)) {
						sampler = adaptor->getRamsesObjectPointer();
					}
				} else if (auto buffer = uniformsHandle[i].asTypedRef<user_types::RenderBuffer>()) {
					if (auto adaptor = sceneAdaptor->lookup<RenderBufferAdaptor>(buffer)) {
						sampler = adaptor->getRamsesObjectPointer();
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
	appearance->replaceTrackedSamplers(newSamplers);
}

};	// namespace raco::ramses_adaptor
