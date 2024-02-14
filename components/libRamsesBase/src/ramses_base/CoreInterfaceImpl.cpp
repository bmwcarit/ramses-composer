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

#include <ramses/client/logic/LuaInterface.h>
#include <ramses/client/logic/LuaModule.h>
#include <ramses/client/logic/LuaScript.h>
#include <ramses/client/logic/Property.h>

namespace raco::ramses_base {

namespace {
void fillLuaScriptInterface(std::vector<core::PropertyInterface>& interface, const ramses::Property* property) {
	static const std::map<ramses::EPropertyType, core::EnginePrimitive> typeMap = {
		{ramses::EPropertyType::Float, core::EnginePrimitive::Double},
		{ramses::EPropertyType::Vec2f, core::EnginePrimitive::Vec2f},
		{ramses::EPropertyType::Vec3f, core::EnginePrimitive::Vec3f},
		{ramses::EPropertyType::Vec4f, core::EnginePrimitive::Vec4f},
		{ramses::EPropertyType::Int32, core::EnginePrimitive::Int32},
		{ramses::EPropertyType::Int64, core::EnginePrimitive::Int64},
		{ramses::EPropertyType::Vec2i, core::EnginePrimitive::Vec2i},
		{ramses::EPropertyType::Vec3i, core::EnginePrimitive::Vec3i},
		{ramses::EPropertyType::Vec4i, core::EnginePrimitive::Vec4i},
		{ramses::EPropertyType::String, core::EnginePrimitive::String},
		{ramses::EPropertyType::Bool, core::EnginePrimitive::Bool},
		{ramses::EPropertyType::Struct, core::EnginePrimitive::Struct},
		{ramses::EPropertyType::Array, core::EnginePrimitive::Array}};
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

ramses::LogicEngine* CoreInterfaceImpl::logicEngine() {
	return backend_->logicEngine();
}

bool CoreInterfaceImpl::parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, core::PropertyInterfaceList& outUniforms, core::PropertyInterfaceList& outAttributes, std::string& outError) {
	return ramses_base::parseShaderText(backend_->internalScene(), vertexShader, geometryShader, fragmentShader, shaderDefines, outUniforms, outAttributes, outError);
}

std::tuple<ramses::LuaConfig, bool> CoreInterfaceImpl::createFullLuaConfig(const std::vector<std::string>& stdModules, const data_storage::Table& modules) {
	ramses::LuaConfig luaConfig = createLuaConfig(stdModules);

	for (auto i = 0; i < modules.size(); ++i) {
		if (auto moduleRef = modules.get(i)->asRef()) {
			const auto module = moduleRef->as<user_types::LuaScriptModule>();
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

bool CoreInterfaceImpl::parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const data_storage::Table& modules, core::PropertyInterfaceList& outInputs, core::PropertyInterfaceList& outOutputs, std::string& outError) {
	auto [luaConfig, valid] = createFullLuaConfig(stdModules, modules);
	if (!valid) {
		return false;
	}

	const auto script = logicEngine()->createLuaScript(luaScript, luaConfig, scriptName);
	if (!script) {
		outError = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value().message;
		return false;
	}

	if (const auto inputs = script->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}
	if (const auto outputs = script->getOutputs()) {
		fillLuaScriptInterface(outOutputs, outputs);
	}
	auto status = logicEngine()->destroy(*script);
	if (!status) {
		auto error = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value();
		LOG_ERROR(log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", error.message);
	}
	return true;
}

bool CoreInterfaceImpl::parseLuaInterface(const std::string& interfaceText, const std::vector<std::string>& stdModules, const data_storage::Table& modules, PropertyInterfaceList& outInputs, std::string& outError) {
	auto [luaConfig, valid] = createFullLuaConfig(stdModules, modules);
	if (!valid) {
		return false;
	}

	ramses::LuaInterface* interface = logicEngine()->createLuaInterface(interfaceText, "Stage::Preprocess", luaConfig);

	if (!interface) {
		outError = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value().message;
		return false;
	}

	if (auto inputs = interface->getInputs()) {
		fillLuaScriptInterface(outInputs, inputs);
	}

	auto status = logicEngine()->destroy(*interface);
	if (!status) {
		auto error = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value();
		LOG_ERROR(log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", error.message);
	}
	return true;
}

bool CoreInterfaceImpl::parseLuaScriptModule(core::SEditorObject object, const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) {
	ramses::LuaConfig tempConfig = createLuaConfig(stdModules);

	if (auto tempModule = ramses_base::ramsesLuaModule(luaScriptModule, logicEngine(), tempConfig, moduleName, object->objectIDAsRamsesLogicID())) {
		cachedModules_[object] = tempModule;
		return true;
	} else {
		outError = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value().message;
		cachedModules_.erase(object);
		return false;
	}
}

void CoreInterfaceImpl::removeModuleFromCache(core::SCEditorObject object) {
	cachedModules_.erase(object);
}

void CoreInterfaceImpl::clearModuleCache() {
	cachedModules_.clear();
}

bool CoreInterfaceImpl::extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) {
	auto callback = [&moduleList](const std::string& module) { moduleList.emplace_back(module); };
	auto extractStatus = logicEngine()->extractLuaDependencies(luaScript, callback);
	if (!extractStatus) {
		outError = logicEngine()->getScene().getRamsesClient().getRamsesFramework().getLastError().value().message;
	}
	return extractStatus;
}

std::string CoreInterfaceImpl::luaNameForPrimitiveType(core::EnginePrimitive engineType) const {
	static const std::unordered_map<core::EnginePrimitive, std::string> nameMap =
		{{core::EnginePrimitive::Bool, "Bool"},
			{core::EnginePrimitive::Int32, "Int32"},
			{core::EnginePrimitive::Int64, "Int64"},
			{core::EnginePrimitive::Double, "Float"},
			{core::EnginePrimitive::String, "String"},
			{core::EnginePrimitive::Vec2f, "Vec2f"},
			{core::EnginePrimitive::Vec3f, "Vec3f"},
			{core::EnginePrimitive::Vec4f, "Vec4f"},
			{core::EnginePrimitive::Vec2i, "Vec2i"},
			{core::EnginePrimitive::Vec3i, "Vec3i"},
			{core::EnginePrimitive::Vec4i, "Vec4i"},
			{core::EnginePrimitive::Struct, "Struct"},
			{core::EnginePrimitive::Array, "Array"}};

	auto it = nameMap.find(engineType);
	if (it != nameMap.end()) {
		return it->second;
	}
	return "Unknown Type";
}

}  // namespace raco::ramses_base