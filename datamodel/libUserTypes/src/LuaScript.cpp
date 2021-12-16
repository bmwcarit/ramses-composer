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
#include "log_system/log.h"
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void LuaScript::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	uriListener_.reset();
}

void LuaScript::onAfterContextActivated(BaseContext& context) {
	uriListener_ = registerFileChangedHandler(context, {shared_from_this(), &LuaScript::uri_}, [this, &context]() { this->syncLuaInterface(context); });
	syncLuaInterface(context);
}

void LuaScript::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	syncLuaInterface(context);
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void LuaScript::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&LuaScript::uri_)) {
		uriListener_ = registerFileChangedHandler(context, value, [this, &context]() { this->syncLuaInterface(context); });
		syncLuaInterface(context);
	}

	if (ValueHandle(shared_from_this(), &LuaScript::objectName_) == value) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}

	const auto& moduleTable = luaModules_.asTable();
	for (auto i = 0; i < moduleTable.size(); ++i) {
		if (value == ValueHandle{shared_from_this(), {"luaModules", moduleTable.name(i)}}) {
			syncLuaInterface(context);
			return;
		}
	}
}

void LuaScript::syncLuaInterface(BaseContext& context) {
	std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScript::uri_}));	

	PropertyInterfaceList inputs{};
	PropertyInterfaceList outputs{};
	std::string error{};
	context.errors().removeError({shared_from_this()});

	syncLuaModules(context, luaScript, error);

	if (error.empty() && !luaScript.empty()) {
		context.engineInterface().parseLuaScript(luaScript, luaModules_.asTable(), inputs, outputs, error);
	}

	if (validateURI(context, {shared_from_this(), &LuaScript::uri_})) {
		if (error.size() > 0) {
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
		}
	}

	if (std::find_if(inputs.begin(), inputs.end(), [](const PropertyInterface& intf) { return intf.type == EnginePrimitive::Int32 && intf.name == "time_ms"; }) != inputs.end()) {
		auto infoText = "Dear Animator,\n\n"
			"this LuaScript uses the 'time_ms'-based runtime hack which will be deprecated in a future version of Ramses Composer.\n"
			"Please prepare to transfer your timer-based animations to our new user types Animation and AnimationChannel.";
		context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::WARNING, shared_from_this(), infoText);
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaScript::luaInputs_), cachedLuaInputValues_, false, true);
	OutdatedPropertiesStore dummyCache{};
	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), &LuaScript::luaOutputs_), dummyCache, true, false);

	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());

	context.changeMultiplexer().recordValueChanged({shared_from_this(), &raco::user_types::LuaScript::luaModules_});
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void LuaScript::syncLuaModules(BaseContext& context, const std::string& fileContents, std::string& outError) {
	std::vector<std::string> moduleDeps;
	auto& moduleTable = luaModules_.asTable();

	context.engineInterface().extractLuaDependencies(fileContents, moduleDeps, outError);
	if (!outError.empty()) {
		moduleTable.clear();
		return;
	}

	for (auto i = 0; i < moduleTable.size(); ++i) {
		cachedModuleRefs_[moduleTable.name(i)] = moduleTable.get(i)->asRef();
	}

	moduleTable.clear();

	std::vector<std::string> redeclaredStandardModules;
	for (const auto& dep : moduleDeps) {
		if (raco::user_types::LuaScriptModule::LUA_STANDARD_MODULES.find(dep) != raco::user_types::LuaScriptModule::LUA_STANDARD_MODULES.end()) {
			redeclaredStandardModules.emplace_back(dep);
		}
	}

	if (!redeclaredStandardModules.empty()) {
		outError = fmt::format("Error while parsing Lua script file: Found redeclaration of standard Lua module{} {}", redeclaredStandardModules.size() == 1 ? "" : "s", fmt::join(redeclaredStandardModules, ", "));
		return;
	}

	for (const auto& dep : moduleDeps) {
		auto moduleRef = std::unique_ptr<raco::data_storage::ValueBase>(new Value<SLuaScriptModule>());
		auto newDep = context.addProperty({shared_from_this(), &LuaScript::luaModules_}, dep, std::move(moduleRef));

		auto cachedModuleIt = cachedModuleRefs_.find(dep);
		if (cachedModuleIt != cachedModuleRefs_.end()) {
			newDep->setRef(cachedModuleIt->second);
		}
	}
}

}  // namespace raco::user_types
