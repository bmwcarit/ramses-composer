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

namespace raco::ramses_base {
class BaseEngineBackend;

class CoreInterfaceImpl final : public raco::core::EngineInterface {
public:
	explicit CoreInterfaceImpl(BaseEngineBackend* backend);
	bool parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, raco::core::PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& error) override;
	bool parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, raco::core::PropertyInterfaceList& outInputs, raco::core::PropertyInterfaceList& outOutputs, std::string& error) override;
	bool parseLuaInterface(const std::string& interfaceText, PropertyInterfaceList& outInputs, std::string& outError) override;
	bool parseLuaScriptModule(const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) override;
	bool extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string& outError) override;
	const std::map<int, std::string>& enumerationDescription(raco::core::EngineEnumeration type) const override;
	std::string luaNameForPrimitiveType(raco::core::EnginePrimitive engineType) const override;

private:
	BaseEngineBackend* backend_;
};

}  // namespace raco::ramses_base
