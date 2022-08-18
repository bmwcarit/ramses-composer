/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_base/Utils.h"

#include "data_storage/Table.h"
#include "lodepng.h"
#include "ramses_base/LogicEngine.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"
#include "utils/MathUtils.h"

#include <ramses-client-api/TextureEnums.h>
#include <ramses-logic/Logger.h>
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/LuaModule.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/Property.h>
#include <sstream>

namespace {

LodePNGColorType ramsesTextureFormatToPngFormat(ramses::ETextureFormat textureFormat) {
	std::map<ramses::ETextureFormat, LodePNGColorType> formats = {
		{ramses::ETextureFormat::R8, LCT_GREY},

		// no LCT_GREY_ALPHA for RG8 because it would not keep the RG channels as intended
		{ramses::ETextureFormat::RG8, LCT_RGB},

		{ramses::ETextureFormat::RGB8, LCT_RGB},
		{ramses::ETextureFormat::RGBA8, LCT_RGBA},
		{ramses::ETextureFormat::RGB16F, LCT_RGB},
		{ramses::ETextureFormat::RGBA16F, LCT_RGBA},
		{ramses::ETextureFormat::SRGB8, LCT_RGB},
		{ramses::ETextureFormat::SRGB8_ALPHA8, LCT_RGBA}};

	auto foundFormat = formats.find(textureFormat);

	return (foundFormat == formats.end()) ? LCT_RGBA : foundFormat->second;
}

std::string pngColorTypeToColorInfo(int colorType) {
	switch (colorType) {
		case LCT_GREY:
			return "R";
		case LCT_GREY_ALPHA:
			return "RG";
		case LCT_PALETTE:
		case LCT_RGB:
			return "RGB";
		case LCT_RGBA:
			return "RGBA";
		default:
			return "Unknown";
	}
}

std::string ramsesTextureFormatToRamsesColorInfo(int colorType, ramses::ETextureFormat textureFormat) {
	static std::unordered_map<int, std::map<ramses::ETextureFormat, std::string>> colorInfos = {
		{LCT_GREY,
			{{ramses::ETextureFormat::R8, "R"},
				{ramses::ETextureFormat::RG8, "RR"},
				{ramses::ETextureFormat::RGB8, "RRR"},
				{ramses::ETextureFormat::RGBA8, "RRR1"},
				{ramses::ETextureFormat::SRGB8, "RRR"},
				{ramses::ETextureFormat::SRGB8_ALPHA8, "RRR1"},
				{ramses::ETextureFormat::RGB16F, "RRR"},
				{ramses::ETextureFormat::RGBA16F, "RRR1"}}},
		{LCT_GREY_ALPHA,
			{{ramses::ETextureFormat::R8, "R"},
				{ramses::ETextureFormat::RG8, "RG"},
				{ramses::ETextureFormat::RGB8, "RRR"},
				{ramses::ETextureFormat::RGBA8, "RRRG"},
				{ramses::ETextureFormat::SRGB8, "RRR"},
				{ramses::ETextureFormat::SRGB8_ALPHA8, "RRRG"},
				{ramses::ETextureFormat::RGB16F, "RRR"},
				{ramses::ETextureFormat::RGBA16F, "RRRG"}}},
		{LCT_PALETTE,
			{{ramses::ETextureFormat::R8, "R"},
				{ramses::ETextureFormat::RG8, "RG"},
				{ramses::ETextureFormat::RGB8, "RGB"},
				{ramses::ETextureFormat::RGBA8, "RGB1"},
				{ramses::ETextureFormat::SRGB8, "RGB"},
				{ramses::ETextureFormat::SRGB8_ALPHA8, "RGB1"},
				{ramses::ETextureFormat::RGB16F, "RGB"},
				{ramses::ETextureFormat::RGBA16F, "RGB1"}}},
		{LCT_RGB,
			{{ramses::ETextureFormat::R8, "R"},
				{ramses::ETextureFormat::RG8, "RG"},
				{ramses::ETextureFormat::RGB8, "RGB"},
				{ramses::ETextureFormat::RGBA8, "RGB1"},
				{ramses::ETextureFormat::SRGB8, "RGB"},
				{ramses::ETextureFormat::SRGB8_ALPHA8, "RGB1"},
				{ramses::ETextureFormat::RGB16F, "RGB"},
				{ramses::ETextureFormat::RGBA16F, "RGB1"}}},
		{LCT_RGBA,
			{{ramses::ETextureFormat::R8, "R"},
				{ramses::ETextureFormat::RG8, "RG"},
				{ramses::ETextureFormat::RGB8, "RGB"},
				{ramses::ETextureFormat::RGBA8, "RGBA"},
				{ramses::ETextureFormat::SRGB8, "RGB"},
				{ramses::ETextureFormat::SRGB8_ALPHA8, "RGBA"},
				{ramses::ETextureFormat::RGB16F, "RGB"},
				{ramses::ETextureFormat::RGBA16F, "RGBA"}}}};

	return colorInfos[colorType][textureFormat];
}

std::string ramsesColorInfoToShaderColorInfo(const std::string &ramsesColorInfo) {
	if (ramsesColorInfo.size() == 3) {
		return ramsesColorInfo + "1";
	}
	if (ramsesColorInfo.size() == 2) {
		return ramsesColorInfo + "01";
	}
	if (ramsesColorInfo.size() == 1) {
		return ramsesColorInfo + "001";
	}

	return ramsesColorInfo;
}

}  // namespace

namespace raco::ramses_base {

std::map<std::string, ramses::EEffectUniformSemantic> defaultUniformSemantics = {
	// Convention take over from Sicht/Absicht
	{"u_MMatrix", ramses::EEffectUniformSemantic::ModelMatrix},
	{"u_MVMatrix", ramses::EEffectUniformSemantic::ModelViewMatrix},
	{"u_MVPMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix},
	{"u_PMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix},
	{"u_VMatrix", ramses::EEffectUniformSemantic::ViewMatrix},
	{"u_NMatrix", ramses::EEffectUniformSemantic::NormalMatrix},
	{"u_CameraWorldPosition", ramses::EEffectUniformSemantic::CameraWorldPosition},
	{"u_resolution", ramses::EEffectUniformSemantic::DisplayBufferResolution},
	{"u_Resolution", ramses::EEffectUniformSemantic::DisplayBufferResolution},

	// Camel case attribute names
	{"uWorldMatrix", ramses::EEffectUniformSemantic::ModelMatrix},
	{"uWorldViewMatrix", ramses::EEffectUniformSemantic::ModelViewMatrix},
	{"uWorldViewProjectionMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix},
	{"uProjectionMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix},
	{"uViewMatrix", ramses::EEffectUniformSemantic::ViewMatrix},
	{"uNormalMatrix", ramses::EEffectUniformSemantic::NormalMatrix},
	{"uCameraPosition", ramses::EEffectUniformSemantic::CameraWorldPosition},
	{"uResolution", ramses::EEffectUniformSemantic::DisplayBufferResolution}};

static std::map<ramses::EEffectInputDataType, raco::core::EnginePrimitive> shaderTypeMap = {
	{ramses::EEffectInputDataType_Int32, raco::core::EnginePrimitive::Int32},
	{ramses::EEffectInputDataType_UInt16, raco::core::EnginePrimitive::UInt16},
	{ramses::EEffectInputDataType_UInt32, raco::core::EnginePrimitive::UInt32},
	{ramses::EEffectInputDataType_Float, raco::core::EnginePrimitive::Double},

	{ramses::EEffectInputDataType_Vector2F, raco::core::EnginePrimitive::Vec2f},
	{ramses::EEffectInputDataType_Vector3F, raco::core::EnginePrimitive::Vec3f},
	{ramses::EEffectInputDataType_Vector4F, raco::core::EnginePrimitive::Vec4f},

	{ramses::EEffectInputDataType_Vector2I, raco::core::EnginePrimitive::Vec2i},
	{ramses::EEffectInputDataType_Vector3I, raco::core::EnginePrimitive::Vec3i},
	{ramses::EEffectInputDataType_Vector4I, raco::core::EnginePrimitive::Vec4i},

	//{ramses::EEffectInputDataType_Matrix22F, },
	//{ramses::EEffectInputDataType_Matrix33F, },
	//{ramses::EEffectInputDataType_Matrix44F, },

	{ramses::EEffectInputDataType_TextureSampler2D, raco::core::EnginePrimitive::TextureSampler2D},
	{ramses::EEffectInputDataType_TextureSampler3D, raco::core::EnginePrimitive::TextureSampler3D},
	{ramses::EEffectInputDataType_TextureSamplerCube, raco::core::EnginePrimitive::TextureSamplerCube}};

std::unique_ptr<ramses::EffectDescription> createEffectDescription(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, const std::string &shaderDefines) {
	std::unique_ptr<ramses::EffectDescription> description{new ramses::EffectDescription};

	if (!shaderDefines.empty()) {
		std::istringstream definesFile(shaderDefines.c_str());
		std::string define;
		// Parse shader text line by line - this currently does not work with multi-line (/* ... */) comments
		while (std::getline(definesFile, define)) {
			if (!define.empty() && define.rfind("//", 0) != 0) {
				/// in Linux, if the file contains carriage returns, they are still contained in the line string
				const auto itr = std::remove_if(define.begin(), define.end(), [](char c) { return std::isspace(c); });
            	define.erase(itr, define.end());
				description->addCompilerDefine(define.c_str());
			}
		}
	}

	description->setVertexShader(vertexShader.c_str());
	description->setFragmentShader(fragmentShader.c_str());
	description->setGeometryShader(geometryShader.c_str());

	for (auto item : defaultUniformSemantics) {
		description->setUniformSemantic(item.first.c_str(), item.second);
	}
	return description;
}

bool parseShaderText(ramses::Scene &scene, const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, const std::string &shaderDefines, raco::core::PropertyInterfaceList &outUniforms, raco::core::PropertyInterfaceList &outAttributes, std::string &outError) {
	outUniforms.clear();
	outAttributes.clear();
	auto description = createEffectDescription(vertexShader, geometryShader, fragmentShader, shaderDefines);
	ramses::Effect *effect = scene.createEffect(*description, ramses::ResourceCacheFlag_DoNotCache, "glsl shader");
	bool success = false;
	if (effect) {
		uint32_t numUniforms = effect->getUniformInputCount();
		for (uint32_t i{0}; i < numUniforms; i++) {
			ramses::UniformInput uniform;
			effect->getUniformInput(i, uniform);
			if (uniform.getSemantics() == ramses::EEffectUniformSemantic::Invalid) {
				if (shaderTypeMap.find(uniform.getDataType()) != shaderTypeMap.end()) {
					outUniforms.emplace_back(std::string{uniform.getName()}, shaderTypeMap[uniform.getDataType()]);
				} else {
					outError += std::string(uniform.getName()) + " has unsupported type  ";
				}
			}
		}

		uint32_t numAttributes = effect->getAttributeInputCount();
		for (uint32_t i{0}; i < numAttributes; i++) {
			ramses::AttributeInput attrib;
			effect->getAttributeInput(i, attrib);
			if (shaderTypeMap.find(attrib.getDataType()) != shaderTypeMap.end()) {
				outAttributes.emplace_back(std::string(attrib.getName()), shaderTypeMap[attrib.getDataType()]);
			} else {
				outError += std::string(attrib.getName()) + " has unsupported type  ";
			}
		}

		success = true;
		scene.destroy(*effect);
	} else {
		outError = scene.getLastEffectErrorMessages();
	}
	return success;
}

rlogic::LuaConfig defaultLuaConfig() {
	rlogic::LuaConfig config;
	config.addStandardModuleDependency(rlogic::EStandardModule::All);
	return config;
}

rlogic::LuaConfig createLuaConfig(const std::vector<std::string> &stdModules) {
	rlogic::LuaConfig config;

	std::map<std::string, rlogic::EStandardModule> stdModuleMap = {
		{"base", rlogic::EStandardModule::Base},
		{"string", rlogic::EStandardModule::String},
		{"table", rlogic::EStandardModule::Table},
		{"math", rlogic::EStandardModule::Math},
		{"debug", rlogic::EStandardModule::Debug}};

	for (const auto &moduleName : stdModules) {
		auto it = stdModuleMap.find(moduleName);
		if (it != stdModuleMap.end()) {
			config.addStandardModuleDependency(it->second);
		}
	}
	return config;
}

ramses::RamsesVersion getRamsesVersion() {
	return ramses::GetRamsesVersion();
}

rlogic::RamsesLogicVersion getLogicEngineVersion() {
	return rlogic::GetRamsesLogicVersion();
}

std::string getRamsesVersionString() {
	return getRamsesVersion().string;
}

std::string getLogicEngineVersionString() {
	return std::string(getLogicEngineVersion().string);
}

rlogic::ELogMessageType toLogicLogLevel(spdlog::level::level_enum level) {
	using namespace spdlog::level;
	using namespace rlogic;

	switch (level) {
		case level_enum::trace:
			return ELogMessageType::Trace;
		case level_enum::debug:
			return ELogMessageType::Debug;
		case level_enum::info:
			return ELogMessageType::Info;
		case level_enum::warn:
			return ELogMessageType::Warn;
		case level_enum::err:
			return ELogMessageType::Error;
		case level_enum::critical:
			return ELogMessageType::Fatal;
		default:
			return ELogMessageType::Off;
	}
}

ramses::ELogLevel toRamsesLogLevel(spdlog::level::level_enum level) {
	using namespace spdlog::level;
	using namespace ramses;

	switch (level) {
		case level_enum::trace:
			return ELogLevel::Trace;
		case level_enum::debug:
			return ELogLevel::Debug;
		case level_enum::info:
			return ELogLevel::Info;
		case level_enum::warn:
			return ELogLevel::Warn;
		case level_enum::err:
			return ELogLevel::Error;
		case level_enum::critical:
			return ELogLevel::Fatal;
		default:
			return ELogLevel::Off;
	}
}

spdlog::level::level_enum toSpdLogLevel(ramses::ELogLevel level) {
	using namespace spdlog::level;
	using namespace ramses;

	switch (level) {
		case ELogLevel::Trace:
			return level_enum::trace;
		case ELogLevel::Debug:
			return level_enum::debug;
		case ELogLevel::Info:
			return level_enum::info;
		case ELogLevel::Warn:
			return level_enum::warn;
		case ELogLevel::Error:
			return level_enum::err;
		case ELogLevel::Fatal:
			return level_enum::critical;
		default:
			return level_enum::off;
	}
}

spdlog::level::level_enum toSpdLogLevel(rlogic::ELogMessageType level) {
	using namespace spdlog::level;
	using namespace rlogic;

	switch (level) {
		case ELogMessageType::Trace:
			return level_enum::trace;
		case ELogMessageType::Debug:
			return level_enum::debug;
		case ELogMessageType::Info:
			return level_enum::info;
		case ELogMessageType::Warn:
			return level_enum::warn;
		case ELogMessageType::Error:
			return level_enum::err;
		case ELogMessageType::Fatal:
			return level_enum::critical;
		default:
			return level_enum::off;
	}
}

void installRamsesLogHandler(bool enableTrace) {
	ramses::RamsesFramework::SetConsoleLogLevel(ramses::ELogLevel::Off);
	ramses::RamsesFramework::SetLogHandler([enableTrace](ramses::ELogLevel level, const std::string &context, const std::string &message) {
		if (!enableTrace && level == ramses::ELogLevel::Trace) {
			return;
		}

		SPDLOG_LOGGER_CALL(raco::log_system::get(raco::log_system::RAMSES), toSpdLogLevel(level), message);
	});
}

void installLogicLogHandler() {
	rlogic::Logger::SetDefaultLogging(false);
	rlogic::Logger::SetLogHandler([](rlogic::ELogMessageType level, std::string_view message) { SPDLOG_LOGGER_CALL(raco::log_system::get(raco::log_system::RAMSES_LOGIC), toSpdLogLevel(level), message); });
}

void setRamsesLogLevel(spdlog::level::level_enum level) {
	ramses::RamsesFramework::SetConsoleLogLevel(toRamsesLogLevel(level));
}

void setLogicLogLevel(spdlog::level::level_enum level) {
	rlogic::Logger::SetLogVerbosityLimit(toLogicLogLevel(level));
}

std::string pngColorTypeToString(int colorType) {
	switch (colorType) {
		case (LCT_GREY):
			return "GREY";
		case (LCT_GREY_ALPHA):
			return "GREY_ALPHA";
		case (LCT_PALETTE):
			return "PALETTE";
		case (LCT_RGB):
			return "RGB";
		case (LCT_RGBA):
			return "RGBA";
		default:
			return "UNKNOWN";
	}
};

PngCompatibilityInfo validateTextureColorTypeAndBitDepth(ramses::ETextureFormat selectedTextureFormat, int colorType, int bitdepth) {
	if (colorType != LCT_PALETTE && bitdepth != 8 && bitdepth != 16) {
		return {"Invalid bit depth (only 8 or 16 bits allowed).", raco::core::ErrorLevel::ERROR, false};
	} else if (bitdepth == 8 && (selectedTextureFormat == ramses::ETextureFormat::RGB16F || selectedTextureFormat == ramses::ETextureFormat::RGBA16F)) {
		return {"Invalid texture format for bit depth (only 8-bit-based formats allowed).", raco::core::ErrorLevel::ERROR, false};
	} else if (bitdepth == 16 && (selectedTextureFormat != ramses::ETextureFormat::RGB16F && selectedTextureFormat != ramses::ETextureFormat::RGBA16F)) {
		return {"Invalid texture format for bit depth (only 16-bit-based formats allowed).", raco::core::ErrorLevel::ERROR, false};
	}

	static std::map<std::pair<LodePNGColorType, int>, std::set<ramses::ETextureFormat>> validTextureFormats = {
		{{LCT_GREY, 8}, {ramses::ETextureFormat::R8}},
		{{LCT_GREY_ALPHA, 8}, {ramses::ETextureFormat::RG8}},
		{{LCT_RGB, 8}, {ramses::ETextureFormat::RGB8, ramses::ETextureFormat::SRGB8}},
		{{LCT_RGBA, 8}, {ramses::ETextureFormat::RGBA8, ramses::ETextureFormat::SRGB8_ALPHA8}},
		{{LCT_RGB, 16}, {ramses::ETextureFormat::RGB16F}},
		{{LCT_RGBA, 16}, {ramses::ETextureFormat::RGBA16F}},
		// palette format is unsupported but should still be convertable
		{{LCT_PALETTE, 8}, {}},
	};

	std::pair<LodePNGColorType, int> pngFormat = {static_cast <LodePNGColorType>(colorType), bitdepth};
	auto validTextureFormatIt = validTextureFormats.find(pngFormat);
	if (validTextureFormatIt == validTextureFormats.end()) {
		return {"Invalid PNG color type.", raco::core::ErrorLevel::ERROR, false};
	}
	if (validTextureFormatIt->second.find(selectedTextureFormat) != validTextureFormatIt->second.end()) {
		return {"", raco::core::ErrorLevel::NONE, false};
	}

	static std::map<std::pair<LodePNGColorType, int>, std::set<ramses::ETextureFormat>> downConvertableTextureFormats = {
		{{LCT_GREY, 8}, {}},
		{{LCT_GREY_ALPHA, 8}, {ramses::ETextureFormat::R8}},
		{{LCT_RGB, 8}, {ramses::ETextureFormat::R8, ramses::ETextureFormat::RG8}},
		{{LCT_RGBA, 8}, {ramses::ETextureFormat::R8, ramses::ETextureFormat::RG8, ramses::ETextureFormat::RGB8, ramses::ETextureFormat::SRGB8}},
		{{LCT_RGB, 16}, {}},
		{{LCT_RGBA, 16}, {ramses::ETextureFormat::RGB16F}},
		{{LCT_PALETTE, 8}, {ramses::ETextureFormat::R8, ramses::ETextureFormat::RG8, ramses::ETextureFormat::RGB8, ramses::ETextureFormat::RGBA8, ramses::ETextureFormat::SRGB8, ramses::ETextureFormat::SRGB8_ALPHA8}},
	};

	auto downConvertableTextureFormat = downConvertableTextureFormats[pngFormat].find(selectedTextureFormat);
	if (downConvertableTextureFormat != downConvertableTextureFormats[pngFormat].end()) {
		return {fmt::format("Selected format {} is not equal to PNG color type {} - image will be converted.", ramsesTextureFormatToString(selectedTextureFormat), pngColorTypeToString(colorType)), raco::core::ErrorLevel::INFORMATION, true};
	}
	return {fmt::format("[Deprecated Warning - this will be an error in a future version] Selected format {} is not equal to PNG color type {} - empty channels will be created.", ramsesTextureFormatToString(selectedTextureFormat), pngColorTypeToString(colorType)), raco::core::ErrorLevel::WARNING, true};
}

std::string ramsesTextureFormatToString(ramses::ETextureFormat textureFormat) {
	return std::string(ramses::getTextureFormatString(textureFormat)).substr(strlen("ETextureFormat_"));
}

int ramsesTextureFormatToChannelAmount(ramses::ETextureFormat textureFormat) {
	switch (textureFormat) {
		case ramses::ETextureFormat::R8:
			return 1;
		case ramses::ETextureFormat::RG8:
			return 2;
		case ramses::ETextureFormat::RGB8:
		case ramses::ETextureFormat::RGB16F:
		case ramses::ETextureFormat::SRGB8:
			return 3;
		case ramses::ETextureFormat::RGBA8:
		case ramses::ETextureFormat::RGBA16F:
		case ramses::ETextureFormat::SRGB8_ALPHA8:
			return 4;
		default:
			return 0;
	}
}

void normalize16BitColorData(std::vector<unsigned char> &data) {
	// convert 16-bit byte data to 16-bit float data
	// see ramses/integration/TestContent/src/Texture2DFormatScene.cpp in Ramses repo for example data
	assert(data.size() % 2 == 0);

	for (auto i = 0; i < data.size(); i += 2) {
		auto dataHalfFloat = raco::utils::math::twoBytesToHalfFloat(data[i], data[i + 1]);
		data[i] = dataHalfFloat;
		data[i + 1] = dataHalfFloat >> 8;
	}
}

std::vector<unsigned char> generateColorDataWithoutBlueChannel(const std::vector<unsigned char> &data) {
	auto rgSize = data.size() - (data.size() / 3);
	std::vector<unsigned char> dataWithoutBlue(rgSize);
	auto j = 0;
	for (auto i = 0; i < data.size(); ++i) {
		if ((i + 1) % 3 != 0) {
			dataWithoutBlue[j++] = data[i];
		}
	}

	return dataWithoutBlue;
}

std::vector<unsigned char> decodeMipMapData(core::Errors *errors, core::Project &project, core::SEditorObject obj, const std::string &uriPropName, int level, PngDecodingInfo &decodingInfo) {
	std::string uri = obj->get(uriPropName)->asString();
	if (uri.empty()) {
		return {};
	}

	auto selectedTextureFormat = static_cast<ramses::ETextureFormat>(obj->get("textureFormat")->asInt());
	decodingInfo.convertedPngFormat = selectedTextureFormat;

	std::vector<unsigned char> data;
	unsigned int curWidth;
	unsigned int curHeight;

	std::string pngPath = raco::core::PathQueries::resolveUriPropertyToAbsolutePath(project, {obj, {uriPropName}});
	auto rawBinaryData = raco::utils::file::readBinary(pngPath);
	lodepng::State pngImportState;
	pngImportState.decoder.color_convert = false;
	lodepng_inspect(&curWidth, &curHeight, &pngImportState, rawBinaryData.data(), rawBinaryData.size());

	auto &lodePngColorInfo = pngImportState.info_png.color;
	auto pngColorType = lodePngColorInfo.colortype;
	decodingInfo.originalPngFormat = pngColorType;
	decodingInfo.originalBitdepth = lodePngColorInfo.bitdepth;
	decodingInfo.pngColorChannels = pngColorTypeToColorInfo(decodingInfo.originalPngFormat);
	decodingInfo.ramsesColorChannels = ramsesTextureFormatToRamsesColorInfo(decodingInfo.originalPngFormat, decodingInfo.convertedPngFormat);
	decodingInfo.shaderColorChannels = ramsesColorInfoToShaderColorInfo(decodingInfo.ramsesColorChannels);
	auto textureFormatCompatInfo = raco::ramses_base::validateTextureColorTypeAndBitDepth(selectedTextureFormat, pngColorType, (pngColorType == LCT_PALETTE) ? 8 : lodePngColorInfo.bitdepth);

	auto ret = textureFormatCompatInfo.conversionNeeded
				   ? lodepng::decode(data, curWidth, curHeight, rawBinaryData, ramsesTextureFormatToPngFormat(selectedTextureFormat), (pngColorType == LCT_PALETTE) ? 8 : lodePngColorInfo.bitdepth)
				   : lodepng::decode(data, curWidth, curHeight, pngImportState, rawBinaryData);

	if (ret != 0) {
		LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "{} '{}': Couldn't load png file from '{}'", obj->getTypeDescription().typeName, obj->objectName(), uri);
		errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {obj->shared_from_this(), {uriPropName}}, "Image file could not be loaded.");
		return {};
	} else {
		if (&obj->getTypeDescription() == &raco::user_types::CubeMap::typeDescription && curWidth != curHeight) {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': non-square image '{}' for '{}'", obj->objectName(), uri, uriPropName);
			errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {obj->shared_from_this(), {uriPropName}},
				fmt::format("Non-square image size {}x{}", curWidth, curHeight));
			return {};
		}


		auto curBitDepth = pngImportState.info_png.color.bitdepth;

		if (level == 1 && decodingInfo.width == -1) {
			decodingInfo.width = curWidth;
			decodingInfo.height = curHeight;
			decodingInfo.bitdepth = curBitDepth;
		}

		int expectedWidth = decodingInfo.width * std::pow(0.5, level - 1);
		int expectedHeight = decodingInfo.height * std::pow(0.5, level - 1);

		if (curWidth != expectedWidth || expectedHeight != expectedHeight) {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "Texture '{}': incompatible image sizes", obj->objectName());
			auto errorMsg = (decodingInfo.width == -1)
								? "Level 1 mipmap not defined"
								: fmt::format("Incompatible image size {}x{}, expected is {}x{}", curWidth, curHeight, expectedWidth, expectedHeight);
			errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {obj->shared_from_this(), {uriPropName}}, errorMsg);
			return {};
		}

		if (curBitDepth != decodingInfo.bitdepth) {
			auto errorMsg = fmt::format("Incompatible image bit depth {}, expected is {}", curBitDepth, decodingInfo.bitdepth);
			errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {obj->shared_from_this(), {uriPropName}}, errorMsg);
			return {};
		}

		if (textureFormatCompatInfo.errorLvl != raco::core::ErrorLevel::NONE) {
			errors->addError(core::ErrorCategory::PARSE_ERROR, textureFormatCompatInfo.errorLvl, {obj->shared_from_this(), {uriPropName}}, textureFormatCompatInfo.errorMsg);
			if (textureFormatCompatInfo.errorLvl == raco::core::ErrorLevel::ERROR) {
				return {};
			}
		} else {
			errors->removeError({obj->shared_from_this(), {uriPropName}});
		}

		if (decodingInfo.bitdepth == 16) {
			raco::ramses_base::normalize16BitColorData(data);
		} else if (pngColorType != LCT_GREY_ALPHA && selectedTextureFormat == ramses::ETextureFormat::RG8) {
			data = raco::ramses_base::generateColorDataWithoutBlueChannel(data);
		}
	}

	return data;
}

}  // namespace raco::ramses_base
