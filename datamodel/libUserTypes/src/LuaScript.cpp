/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaScript.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"

namespace raco::user_types {


void LuaScript::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	// module changed
	// -> only script parsing/sync
	syncLuaScript(context, false);
}

void LuaScript::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	// uri changed -> updateFromExternalFile
	// -> sync lua modules and parse/sync script
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&LuaScript::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}

	ValueHandle modulesHandle(shared_from_this(), &LuaScript::luaModules_);
	if (modulesHandle.contains(value)) {
		// -> only script parsing
		syncLuaScript(context, false);
	}
}

void LuaScript::updateFromExternalFile(BaseContext& context) {
	syncLuaScript(context, true);
}

void LuaScript::syncLuaScript(BaseContext& context, bool syncModules) {
	context.errors().removeAll({shared_from_this()});

	PropertyInterfaceList inputs{};
	PropertyInterfaceList outputs{};

	if (validateURI(context, {shared_from_this(), &LuaScript::uri_})) {
		std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScript::uri_}));

		std::string error{};
		bool success = true;

		if (syncModules) {
			success = syncLuaModules(context, luaScript, error);
		}

		if (success) {
			const auto& moduleTable = luaModules_.asTable();
			ValueHandle moduleTableHandle{shared_from_this(), &LuaScript::luaModules_};
			for (auto i = 0; i < moduleTable.size(); ++i) {
				if (auto moduleRef = moduleTable.get(i)->asRef()) {
					const auto module = moduleRef->as<raco::user_types::LuaScriptModule>();
					if (!module->isValid()) {
						context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, moduleTableHandle[i], fmt::format("Invalid LuaScriptModule '{}' assigned.", moduleRef->objectName()));
						success = false;
					}
				} else {
					context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::ERROR, moduleTableHandle[i], fmt::format("Required LuaScriptModule '{}' is unassigned.", moduleTable.name(i)));
					success = false;
				}
			}
		}

		if (success) {
			success = context.engineInterface().parseLuaScript(luaScript, objectName(), *luaModules_, inputs, outputs, error);
		}

		if (success) {
			if (std::find_if(inputs.begin(), inputs.end(), [](const PropertyInterface& intf) { return intf.type == EnginePrimitive::Int32 && intf.name == "time_ms"; }) != inputs.end()) {
				auto infoText =
					"Dear Animator,\n\n"
					"this LuaScript uses the 'time_ms'-based runtime hack which will be deprecated in a future version of Ramses Composer.\n"
					"Please prepare to transfer your timer-based animations to our new user types Animation and AnimationChannel.";
				context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::WARNING, shared_from_this(), infoText);
			}
		} else {
			if (!error.empty()) {
				context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
			}
		}
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaScript::luaInputs_), cachedLuaInputValues_, false, true);
	OutdatedPropertiesStore dummyCache{};
	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), &LuaScript::luaOutputs_), dummyCache, true, false);

	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

bool LuaScript::syncLuaModules(BaseContext& context, const std::string& fileContents, std::string& outError) {
	std::vector<std::string> moduleDeps;
	bool success = context.engineInterface().extractLuaDependencies(fileContents, moduleDeps, outError);
	if (!success) {
		moduleDeps.clear();
	}

	std::vector<std::string> redeclaredStandardModules;
	for (const auto& dep : moduleDeps) {
		if (raco::user_types::LuaScriptModule::LUA_STANDARD_MODULES.find(dep) != raco::user_types::LuaScriptModule::LUA_STANDARD_MODULES.end()) {
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
	for (size_t index = 0; index < luaModules_->size(); index++) {
		const auto& name = luaModules_->name(index);
		if (std::find(moduleDeps.begin(), moduleDeps.end(), name) == moduleDeps.end()) {
			toRemove.emplace_back(name);
		}
	}
	ValueHandle moduleHandle{shared_from_this(), &LuaScript::luaModules_};
	for (const auto& propName : toRemove) {
		auto refValue = moduleHandle.get(propName).asRef();
		if (refValue) {
			cachedModuleRefs_[propName] = refValue->objectID();
		}
		context.removeProperty(moduleHandle, propName);
	}

	// Add new module properties
	for (const auto& moduleName : moduleDeps) {
		if (!luaModules_->hasProperty(moduleName)) {
			std::unique_ptr<raco::data_storage::ValueBase> newValue = std::make_unique<Value<SLuaScriptModule>>();
			auto it = cachedModuleRefs_.find(moduleName);			
			if (it != cachedModuleRefs_.end()) {
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
