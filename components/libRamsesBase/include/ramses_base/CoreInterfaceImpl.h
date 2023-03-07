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

class CoreInterfaceImpl final : public raco::core::EngineInterface {
public:
	explicit CoreInterfaceImpl(BaseEngineBackend* backend);
	bool parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, raco::core::PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& error) override;
	bool parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, raco::core::PropertyInterfaceList& outInputs, raco::core::PropertyInterfaceList& outOutputs, std::string& error) override;
	bool parseLuaInterface(const std::string& interfaceText, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, bool useModules, PropertyInterfaceList& outInputs, std::string& outError) override;
	bool parseLuaScriptModule(raco::core::SEditorObject object, const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) override;
	bool extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) override;
	std::string luaNameForPrimitiveType(raco::core::EnginePrimitive engineType) const override;

	void removeModuleFromCache(raco::core::SCEditorObject object) override;
	void clearModuleCache() override;

private:
	std::tuple<rlogic::LuaConfig, bool> createFullLuaConfig(const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules);

	typedef std::unique_ptr<rlogic::LogicEngine> UniqueLogicEngine;

	BaseEngineBackend* backend_;

	// Second LogicEngine instance only used for LuaScript/Interface/Module parsing.
	// This LogicEngine will always be at feature level 1 which is OK as long as we only use it for 
	// script parsing and the LogicEngine doesn't introduce changes to the script parsing at higher
	// feature levels.
	UniqueLogicEngine logicEngine_;

	std::map<raco::core::SCEditorObject, ramses_base::RamsesLuaModule> cachedModules_;
};

}  // namespace raco::ramses_base
