/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EnumerationDescriptions.h"

#include "data_storage/Table.h"

#include "core/EngineInterface.h"

#include "log_system/log.h"

#include "ramses_base/BaseEngineBackend.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"

#include "user_types/Enumerations.h"
#include "user_types/LuaScriptModule.h"

#include <ramses-logic/LuaInterface.h>
#include <ramses-logic/LuaModule.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/Property.h>

namespace raco::ramses_base {

namespace {
void fillLuaScriptInterface(std::vector<raco::core::PropertyInterface>& interface, const rlogic::Property* property) {
	static const std::map<rlogic::EPropertyType, raco::core::EnginePrimitive> typeMap = {
		{rlogic::EPropertyType::Float, raco::core::EnginePrimitive::Double},
		{rlogic::EPropertyType::Vec2f, raco::core::EnginePrimitive::Vec2f},
		{rlogic::EPropertyType::Vec3f, raco::core::EnginePrimitive::Vec3f},
		{rlogic::EPropertyType::Vec4f, raco::core::EnginePrimitive::Vec4f},
		{rlogic::EPropertyType::Int32, raco::core::EnginePrimitive::Int32},
		{rlogic::EPropertyType::Int64, raco::core::EnginePrimitive::Int64},
		{rlogic::EPropertyType::Vec2i, raco::core::EnginePrimitive::Vec2i},
		{rlogic::EPropertyType::Vec3i, raco::core::EnginePrimitive::Vec3i},
		{rlogic::EPropertyType::Vec4i, raco::core::EnginePrimitive::Vec4i},
		{rlogic::EPropertyType::String, raco::core::EnginePrimitive::String},
		{rlogic::EPropertyType::Bool, raco::core::EnginePrimitive::Bool},
		{rlogic::EPropertyType::Struct, raco::core::EnginePrimitive::Struct},
		{rlogic::EPropertyType::Array, raco::core::EnginePrimitive::Array}};
	interface.reserve(property->getChildCount());
	for (int i{0}; i < property->getChildCount(); i++) {
		auto child{property->getChild(i)};
		if (typeMap.find(child->getType()) != typeMap.end()) {
			// has children
			auto& it = interface.emplace_back(std::string{child->getName()}, typeMap.at(child->getType()));
			if (child->getChildCount() > 0) {
				fillLuaScriptInterface(it.children, child);
			}
		}
	}
}
}  // namespace

CoreInterfaceImpl::CoreInterfaceImpl(BaseEngineBackend* backend) : backend_{backend} {}

bool CoreInterfaceImpl::parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, raco::core::PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& outError) {
	return raco::ramses_base::parseShaderText(backend_->internalScene(), vertexShader, geometryShader, fragmentShader, shaderDefines, outUniforms, outAttributes, outError);
}

bool CoreInterfaceImpl::parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, raco::core::PropertyInterfaceList& outInputs, raco::core::PropertyInterfaceList& outOutputs, std::string& outError) {
	rlogic::LuaConfig luaConfig = createLuaConfig(stdModules);
	std::vector<RamsesLuaModule> tempModules;

	for (auto i = 0; i < modules.size(); ++i) {
		if (auto moduleRef = modules.get(i)->asRef()) {
			const auto module = moduleRef->as<raco::user_types::LuaScriptModule>();
			if (module->isValid()) {
				auto moduleConfig = raco::ramses_base::createLuaConfig(module->stdModules_->activeModules());
				const auto tempModule = tempModules.emplace_back(ramsesLuaModule(module->currentScriptContents(), &backend_->logicEngine(), moduleConfig, moduleRef->objectName(), moduleRef->objectIDAsRamsesLogicID()));
				assert(tempModule != nullptr);
				luaConfig.addDependency(modules.name(i), *tempModule);
			} else {
				// We already checked the module validity before parsing
				assert(false);
				return false;
			}
		} else {
			// We already checked for non-empty module references before parsing
			assert(false);
			return false;
		}
	}

	const auto script = backend_->logicEngine().createLuaScript(luaScript, luaConfig, scriptName);
	if (!script) {
		outError = backend_->logicEngine().getErrors().at(0).message;
		return false;
	}

	if (const auto inputs = script->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}
	if (const auto outputs = script->getOutputs()) {
		fillLuaScriptInterface(outOutputs, outputs);
	}
	auto status = backend_->logicEngine().destroy(*script);
	if (!status) {
		LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{backend_->logicEngine()});
	}
	return true;
}

bool CoreInterfaceImpl::parseLuaInterface(const std::string& interfaceText, PropertyInterfaceList& outInputs, std::string& outError) {
	auto interface = backend_->logicEngine().createLuaInterface(interfaceText, "Stage::Preprocess");
	if (!interface) {
		outError = backend_->logicEngine().getErrors().at(0).message;
		return false;
	}

	if (auto inputs = interface->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}

	auto status = backend_->logicEngine().destroy(*interface);
	if (!status) {
		LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{backend_->logicEngine()});
	}
	return true;
}


bool CoreInterfaceImpl::parseLuaScriptModule(const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) {
	rlogic::LuaConfig tempConfig = createLuaConfig(stdModules);
	if (auto tempModule = backend_->logicEngine().createLuaModule(luaScriptModule, tempConfig, moduleName)) {
		auto status = backend_->logicEngine().destroy(*tempModule);
		if (!status) {
			LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{backend_->logicEngine()});
		}
		return true;
	} else {
		outError = backend_->logicEngine().getErrors().at(0).message;
		return false;
	}
}

bool CoreInterfaceImpl::extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) {
	auto callback = [&moduleList](const std::string& module) { moduleList.emplace_back(module); };
	auto extractStatus = backend_->logicEngine().extractLuaDependencies(luaScript, callback);
	if (!extractStatus) {
		outError = backend_->logicEngine().getErrors().at(0).message;
	}
	return extractStatus;
}

const std::map<int, std::string>& CoreInterfaceImpl::enumerationDescription(raco::core::EngineEnumeration type) const {
	switch (type) {
		case raco::core::EngineEnumeration::CullMode:
			return enumerationCullMode;
		case raco::core::EngineEnumeration::BlendOperation:
			return enumerationBlendOperation;
		case raco::core::EngineEnumeration::BlendFactor:
			return enumerationBlendFactor;
		case raco::core::EngineEnumeration::DepthFunction:
			return enumerationDepthFunction;
		case raco::core::EngineEnumeration::TextureAddressMode:
			return enumerationTextureAddressMode;
		case raco::core::EngineEnumeration::TextureMinSamplingMethod:
			return enumerationTextureMinSamplingMethod;
		case raco::core::EngineEnumeration::TextureMagSamplingMethod:
			return enumerationTextureMagSamplingMethod;
		case raco::core::EngineEnumeration::TextureFormat:
			return enumerationTextureFormat;

		case raco::core::EngineEnumeration::RenderBufferFormat:
			return enumerationRenderBufferFormat;

		case raco::core::EngineEnumeration::RenderLayerOrder:
			return raco::user_types::enumerationRenderLayerOrder;

		case raco::core::EngineEnumeration::RenderLayerMaterialFilterMode:
			return raco::user_types::enumerationRenderLayerMaterialFilterMode;

		default:
			assert(false);
			return enumerationEmpty;
	}
}

std::string CoreInterfaceImpl::luaNameForPrimitiveType(raco::core::EnginePrimitive engineType) const {
	static const std::unordered_map<raco::core::EnginePrimitive, std::string> nameMap =
		{{raco::core::EnginePrimitive::Bool, "Bool"},
			{raco::core::EnginePrimitive::Int32, "Int32"},
			{raco::core::EnginePrimitive::Int64, "Int64"},
			{raco::core::EnginePrimitive::Double, "Float"},
			{raco::core::EnginePrimitive::String, "String"},
			{raco::core::EnginePrimitive::Vec2f, "Vec2f"},
			{raco::core::EnginePrimitive::Vec3f, "Vec3f"},
			{raco::core::EnginePrimitive::Vec4f, "Vec4f"},
			{raco::core::EnginePrimitive::Vec2i, "Vec2i"},
			{raco::core::EnginePrimitive::Vec3i, "Vec3i"},
			{raco::core::EnginePrimitive::Vec4i, "Vec4i"},
			{raco::core::EnginePrimitive::Struct, "Struct"},
			{raco::core::EnginePrimitive::Array, "Array"}};

	auto it = nameMap.find(engineType);
	if (it != nameMap.end()) {
		return it->second;
	}
	return "Unknown Type";
}

}  // namespace raco::ramses_base