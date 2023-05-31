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
#include "core/Errors.h"
#include "core/Project.h"
#include "data_storage/Value.h"
#include "log_system/log.h"
#include "ramses_base/LogicEngine.h"
#include <map>
#include <optional>
#include <ramses-client-api/EffectInputDataType.h>
#include <ramses-client.h>
#include <ramses-framework-api/RamsesVersion.h>
#include <ramses-logic/LuaConfig.h>
#include <ramses-logic/RamsesLogicVersion.h>
#include <string>

namespace raco::ramses_base {

std::unique_ptr<ramses::EffectDescription> createEffectDescription(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines);

using PropertyInterfaceList = raco::core::PropertyInterfaceList;
// Parse shaders using Ramses and return set of uniforms with name and type.
// Returns true if shader can be successfully parsed.
bool parseShaderText(ramses::Scene& scene, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& outError);

rlogic::LuaConfig defaultLuaConfig();
rlogic::LuaConfig createLuaConfig(const std::vector<std::string>& stdModules);

ramses::RamsesVersion getRamsesVersion();
rlogic::RamsesLogicVersion getLogicEngineVersion();

std::string getRamsesVersionString();
std::string getLogicEngineVersionString();

void installRamsesLogHandler(bool enableTrace);
void installLogicLogHandler();
void setRamsesLogLevel(spdlog::level::level_enum level);
void setLogicLogLevel(spdlog::level::level_enum level);

struct PngCompatibilityInfo {
	std::string errorMsg;
	raco::core::ErrorLevel errorLvl;
	bool conversionNeeded;
};

struct PngDecodingInfo {
	int width = -1;
	int height = -1;
	int bitdepth = -1;
	int originalBitdepth = -1;
	int originalPngFormat = -1;
	ramses::ETextureFormat convertedPngFormat = ramses::ETextureFormat::Invalid;
	std::string pngColorChannels;
	std::string ramsesColorChannels;
	std::string shaderColorChannels;
};

PngCompatibilityInfo validateTextureColorTypeAndBitDepth(ramses::ETextureFormat selectedTextureFormat, int colorType, int bitdepth);
std::string ramsesTextureFormatToString(ramses::ETextureFormat textureFormat);
std::string pngColorTypeToString(int colorType);
int ramsesTextureFormatToChannelAmount(ramses::ETextureFormat textureFormat);
void normalize16BitColorData(std::vector<unsigned char>& data);
std::vector<unsigned char> generateColorDataWithoutBlueChannel(const std::vector<unsigned char>& data);
std::vector<unsigned char> decodeMipMapData(core::Errors* errors, core::Project& project, core::SEditorObject obj, const std::string& uriPropName, int level, PngDecodingInfo& decodingInfo, bool swizzle = false);
std::tuple<std::string, ramses::ETextureFormat, ramses::TextureSwizzle> ramsesTextureFormatToSwizzleInfo(int colorType, ramses::ETextureFormat textureFormat);


int clipAndCheckIntProperty(const raco::core::ValueHandle value, core::Errors* errors, bool* allValid);

ramses::ERenderBufferType ramsesRenderBufferTypeFromFormat(ramses::ERenderBufferFormat format);


std::vector<std::string> getRamsesUniformPropertyNames(core::ValueHandle uniformContainerHandle, const std::vector<std::string>& propertyNames, size_t startIndex = 0);

std::string getRamsesUniformPropertyName(core::ValueHandle uniformContainerHandle, core::ValueHandle uniformHandle);

};	// namespace raco::ramses_base
