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
#include "core/Errors.h"
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

rlogic::LuaConfig defaultLuaConfig();

ramses::RamsesVersion getRamsesVersion();
rlogic::RamsesLogicVersion getLogicEngineVersion();

std::string getRamsesVersionString();
std::string getLogicEngineVersionString();

void installLogicLogHandler();
void setRamsesLogLevel(spdlog::level::level_enum level);
void setLogicLogLevel(spdlog::level::level_enum level);

struct PngCompatibilityInfo {
	std::string errorMsg;
	raco::core::ErrorLevel errorLvl;
	bool conversionNeeded;
};

PngCompatibilityInfo validateTextureColorTypeAndBitDepth(ramses::ETextureFormat selectedTextureFormat, int colorType, int bitdepth);
int ramsesTextureFormatToPngFormat(ramses::ETextureFormat textureFormat);
std::string ramsesTextureFormatToString(ramses::ETextureFormat textureFormat);
int ramsesTextureFormatToChannelAmount(ramses::ETextureFormat textureFormat);
void normalize16BitColorData(std::vector<unsigned char>& data);
std::vector<unsigned char> generateColorDataWithoutBlueChannel(const std::vector<unsigned char>& data);
};	// namespace raco::ramses_base
