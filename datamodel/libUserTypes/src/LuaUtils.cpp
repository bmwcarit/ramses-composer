/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LuaUtils.h"

#include "core/Errors.h"

#include "user_types/LuaScriptModule.h"


namespace raco::user_types {

bool checkLuaModules(ValueHandle moduleTableHandle, Errors& errors) {
	bool success = true;
	const Table& moduleTable = moduleTableHandle.constValueRef()->asTable();
	for (auto i = 0; i < moduleTable.size(); ++i) {
		if (auto moduleRef = moduleTable.get(i)->asRef()) {
			const auto module = moduleRef->as<user_types::LuaScriptModule>();
			if (!module->isValid()) {
				errors.addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, moduleTableHandle[i], fmt::format("Invalid LuaScriptModule '{}' assigned.", moduleRef->objectName()));
				success = false;
			}
		} else {
			errors.addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, moduleTableHandle[i], fmt::format("Required LuaScriptModule '{}' is unassigned.", moduleTable.name(i)));
			success = false;
		}
	}
	return success;
}

bool syncLuaModules(BaseContext& context, ValueHandle moduleHandle, const std::string& fileContents, std::map<std::string, std::string>& cachedModuleRefs, std::string& outError) {
	std::vector<std::string> moduleDeps;
	bool success = context.engineInterface().extractLuaDependencies(fileContents, moduleDeps, outError);
	if (!success) {
		moduleDeps.clear();
	}

	std::vector<std::string> redeclaredStandardModules;
	for (const auto& dep : moduleDeps) {
		if (user_types::LuaScriptModule::LUA_STANDARD_MODULES.find(dep) != user_types::LuaScriptModule::LUA_STANDARD_MODULES.end()) {
			redeclaredStandardModules.emplace_back(dep);
		}
	}

	if (!redeclaredStandardModules.empty()) {
		outError = fmt::format("Error while parsing Lua script file: Found redeclaration of standard Lua module{} {}", redeclaredStandardModules.size() == 1 ? "" : "s", fmt::join(redeclaredStandardModules, ", "));
		moduleDeps.clear();
		success = false;
	}

	// Remove outdated module properties
	std::vector<std::string> toRemove;
	const Table& moduleTable = moduleHandle.constValueRef()->asTable();

	for (size_t index = 0; index < moduleTable.size(); index++) {
		const auto& name = moduleTable.name(index);
		if (std::find(moduleDeps.begin(), moduleDeps.end(), name) == moduleDeps.end()) {
			toRemove.emplace_back(name);
		}
	}
	for (const auto& propName : toRemove) {
		auto refValue = moduleHandle.get(propName).asRef();
		if (refValue) {
			cachedModuleRefs[propName] = refValue->objectID();
		}
		context.removeProperty(moduleHandle, propName);
	}

	// Add new module properties
	for (const auto& moduleName : moduleDeps) {
		if (!moduleTable.hasProperty(moduleName)) {
			std::unique_ptr<data_storage::ValueBase> newValue = std::make_unique<Value<SLuaScriptModule>>();
			auto it = cachedModuleRefs.find(moduleName);
			if (it != cachedModuleRefs.end()) {
				std::string cachedRefID = it->second;
				auto cachedObject = context.project()->getInstanceByID(cachedRefID);
				if (cachedObject) {
					*newValue = cachedObject;
				}
			}
			context.addProperty(moduleHandle, moduleName, std::move(newValue));
		}
	}

	return success;
}

}  // namespace raco::user_types