/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "data_storage/Table.h"

#include "core/EditorObject.h"
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

CoreInterfaceImpl::CoreInterfaceImpl(BaseEngineBackend* backend) : backend_{backend}, logicEngine_(std::make_unique<rlogic::LogicEngine>()) {}

bool CoreInterfaceImpl::parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, raco::core::PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& outError) {
	return raco::ramses_base::parseShaderText(backend_->internalScene(), vertexShader, geometryShader, fragmentShader, shaderDefines, outUniforms, outAttributes, outError);
}

std::tuple<rlogic::LuaConfig, bool> CoreInterfaceImpl::createFullLuaConfig(const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules) {
	rlogic::LuaConfig luaConfig = createLuaConfig(stdModules);

	for (auto i = 0; i < modules.size(); ++i) {
		if (auto moduleRef = modules.get(i)->asRef()) {
			const auto module = moduleRef->as<raco::user_types::LuaScriptModule>();
			if (module->isValid()) {
				auto it = cachedModules_.find(module);
				assert(it != cachedModules_.end());
				luaConfig.addDependency(modules.name(i), *it->second);
			} else {
				// We already checked the module validity before parsing
				assert(false);
				return {{}, false};
			}
		} else {
			// We already checked for non-empty module references before parsing
			assert(false);
			return {{}, false};
		}
	}

	return {luaConfig, true};
}

bool CoreInterfaceImpl::parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, raco::core::PropertyInterfaceList& outInputs, raco::core::PropertyInterfaceList& outOutputs, std::string& outError) {
	auto [luaConfig, valid] = createFullLuaConfig(stdModules, modules);
	if (!valid) {
		return false;
	}

	const auto script = logicEngine_->createLuaScript(luaScript, luaConfig, scriptName);
	if (!script) {
		outError = logicEngine_->getErrors().at(0).message;
		return false;
	}

	if (const auto inputs = script->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}
	if (const auto outputs = script->getOutputs()) {
		fillLuaScriptInterface(outOutputs, outputs);
	}
	auto status = logicEngine_->destroy(*script);
	if (!status) {
		LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{*logicEngine_});
	}
	return true;
}

bool CoreInterfaceImpl::parseLuaInterface(const std::string& interfaceText, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, bool useModules, PropertyInterfaceList& outInputs, std::string& outError) {
	rlogic::LuaInterface* interface = nullptr;
	if (useModules) {
		// New style creation function: must supply modules if interface text contains modules() statement
		// used at feature level >= 5
		auto [luaConfig, valid] = createFullLuaConfig(stdModules, modules);
		if (!valid) {
			return false;
		}

		interface =logicEngine_->createLuaInterface(interfaceText, "Stage::Preprocess", luaConfig);
	} else {
		// Old style creation function: doesn't generate error if interface text contains modules() statement
		// used at feature level < 5
		interface =logicEngine_->createLuaInterface(interfaceText, "Stage::Preprocess");
	}

	if (!interface) {
		outError =logicEngine_->getErrors().at(0).message;
		return false;
	}

	if (auto inputs = interface->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}

	auto status =logicEngine_->destroy(*interface);
	if (!status) {
		LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{backend_->logicEngine()});
	}
	return true;
}

bool CoreInterfaceImpl::parseLuaScriptModule(raco::core::SEditorObject object, const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) {
	rlogic::LuaConfig tempConfig = createLuaConfig(stdModules);

	if (auto tempModule = raco::ramses_base::ramsesLuaModule(luaScriptModule, logicEngine_.get(), tempConfig, moduleName, object->objectIDAsRamsesLogicID())) {
		cachedModules_[object] = tempModule;
		return true;
	} else {
		outError = logicEngine_->getErrors().at(0).message;
		cachedModules_.erase(object);
		return false;
	}
}

void CoreInterfaceImpl::removeModuleFromCache(raco::core::SCEditorObject object) {
	cachedModules_.erase(object);
}

void CoreInterfaceImpl::clearModuleCache() {
	cachedModules_.clear();
}

bool CoreInterfaceImpl::extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) {
	auto callback = [&moduleList](const std::string& module) { moduleList.emplace_back(module); };
	auto extractStatus = backend_->logicEngine().extractLuaDependencies(luaScript, callback);
	if (!extractStatus) {
		outError = backend_->logicEngine().getErrors().at(0).message;
	}
	return extractStatus;
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