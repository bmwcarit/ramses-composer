/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/EngineInterface.h"
#include "data_storage/Value.h"
#include "ramses_base/LogicEngine.h"
#include "log_system/log.h"
#include <map>
#include <optional>
#include <ramses-client-api/EffectInputDataType.h>
#include <ramses-client.h>
#include <ramses-framework-api/RamsesVersion.h>
#include <ramses-logic/LuaConfig.h>
#include <ramses-logic/RamsesLogicVersion.h>
#include <string>

namespace raco::ramses_base {

std::unique_ptr<ramses::EffectDescription> createEffectDescription(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines );

using PropertyInterfaceList = raco::core::PropertyInterfaceList;
// Parse shaders using Ramses and return set of uniforms with name and type.
// Returns true if shader can be successfully parsed.
bool parseShaderText(ramses::Scene& scene, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& outError);

// Parse luascripts using ramses logic and return set of in and out parameters with name and type.
// Returns true if script can be successfully parsed.
bool parseLuaScript(LogicEngine& engine, const std::string& luaScript, const raco::data_storage::Table& modules, PropertyInterfaceList& outInputs, PropertyInterfaceList& outOutputs, std::string& outError);

// Parse luascript module using ramses logic.
// Returns true if module can be successfully parsed.
bool parseLuaScriptModule(LogicEngine& engine, const std::string& luaScriptModule, std::string& outError);

rlogic::LuaConfig defaultLuaConfig();

ramses::RamsesVersion getRamsesVersion();
rlogic::RamsesLogicVersion getLogicEngineVersion();

std::string getRamsesVersionString();
std::string getLogicEngineVersionString();

void installLogicLogHandler();
void setRamsesLogLevel(spdlog::level::level_enum level);
void setLogicLogLevel(spdlog::level::level_enum level);

};	// namespace raco::ramses_base
