/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/EngineInterface.h"
#include "ramses_base/Utils.h"
#include "user_types/LuaScript.h"
#include "ramses_base/RamsesHandles.h"

#include <map>

namespace raco::ramses_base {
class BaseEngineBackend;

class CoreInterfaceImpl final : public core::EngineInterface {
public:
	explicit CoreInterfaceImpl(BaseEngineBackend* backend);
	bool parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, core::PropertyInterfaceList& outUniforms, core::PropertyInterfaceList& outAttributes, std::string& error) override;
	bool parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const data_storage::Table& modules, core::PropertyInterfaceList& outInputs, core::PropertyInterfaceList& outOutputs, std::string& error) override;
	bool parseLuaInterface(const std::string& interfaceText, const std::vector<std::string>& stdModules, const data_storage::Table& modules, PropertyInterfaceList& outInputs, std::string& outError) override;
	bool parseLuaScriptModule(core::SEditorObject object, const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) override;
	bool extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) override;
	std::string luaNameForPrimitiveType(core::EnginePrimitive engineType) const override;

	void removeModuleFromCache(core::SCEditorObject object) override;
	void clearModuleCache() override;

private:
	ramses::LogicEngine* logicEngine();

	std::tuple<ramses::LuaConfig, bool> createFullLuaConfig(const std::vector<std::string>& stdModules, const data_storage::Table& modules);

	BaseEngineBackend* backend_;

	std::map<core::SCEditorObject, ramses_base::RamsesLuaModule> cachedModules_;
};

}  // namespace raco::ramses_base
