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
#include <map>
#include <optional>
#include <ramses/client/ramses-client.h>
#include <ramses/client/logic/LogicEngine.h>
#include <ramses/framework/RamsesVersion.h>
#include <ramses/client/logic/LuaConfig.h>
#include <string>

#include <QCommandLineParser>

namespace raco::ramses_base {

std::unique_ptr<ramses::EffectDescription> createEffectDescription(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines);

using PropertyInterfaceList = core::PropertyInterfaceList;
// Parse shaders using Ramses and return set of uniforms with name and type.
// Returns true if shader can be successfully parsed.
bool parseShaderText(ramses::Scene& scene, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, PropertyInterfaceList& outUniforms, core::PropertyInterfaceList& outAttributes, std::string& outError);

ramses::LuaConfig defaultLuaConfig();
ramses::LuaConfig createLuaConfig(const std::vector<std::string>& stdModules);

ramses::RamsesVersion getRamsesVersion();

std::string getRamsesVersionString();

ramses::ELogLevel toRamsesLogLevel(spdlog::level::level_enum level);

void installRamsesLogHandler(bool enableTrace);

spdlog::level::level_enum getLevelFromArg(const QString& arg);
ramses::ELogLevel getRamsesLogLevelFromArg(const QString& arg);

void addRamseFrameworkOptions(QCommandLineParser& parser);
std::vector<QString> ramsesLogCategories(); 

struct PngCompatibilityInfo {
	std::string errorMsg;
	core::ErrorLevel errorLvl;
	bool conversionNeeded;
};

struct PngDecodingInfo {
	int width = -1;
	int height = -1;
	int bitdepth = -1;
	int originalBitdepth = -1;
	int originalPngFormat = -1;
	ramses::ETextureFormat convertedPngFormat = ramses::ETextureFormat::RGB8;
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


int clipAndCheckIntProperty(const core::ValueHandle value, core::Errors* errors, bool* allValid);


std::vector<std::string> getRamsesUniformPropertyNames(core::ValueHandle uniformContainerHandle, const std::vector<std::string_view>& propertyNames, size_t startIndex = 0);

std::string getRamsesUniformPropertyName(core::ValueHandle uniformContainerHandle, core::ValueHandle uniformHandle);

};	// namespace raco::ramses_base
