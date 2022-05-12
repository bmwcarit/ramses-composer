/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_base/Utils.h"

#include "data_storage/Table.h"
#include "lodepng.h"
#include "ramses_base/LogicEngine.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/LuaScriptModule.h"
#include "utils/MathUtils.h"

#include <ramses-client-api/TextureEnums.h>
#include <ramses-logic/Logger.h>
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/LuaModule.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/Property.h>
#include <sstream>

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
		while (std::getline(definesFile, define)) {
			if (!define.empty() && define.rfind("//", 0) != 0) {
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

PngCompatibilityInfo validateTextureColorTypeAndBitDepth(ramses::ETextureFormat selectedTextureFormat, int colorType, int bitdepth) {
	if (bitdepth != 8 && bitdepth != 16) {
		return {"Invalid bit depth (only 8 or 16 bits allowed).", raco::core::ErrorLevel::ERROR, false};
	} else if (bitdepth == 8 && (selectedTextureFormat == ramses::ETextureFormat::RGB16F || selectedTextureFormat == ramses::ETextureFormat::RGBA16F)) {
		return {"Invalid texture format for bit depth (only 8-bit-based formats allowed).", raco::core::ErrorLevel::ERROR, false};
	} else if (bitdepth == 16 && (selectedTextureFormat != ramses::ETextureFormat::RGB16F && selectedTextureFormat != ramses::ETextureFormat::RGBA16F)) {
		return {"Invalid texture format for bit depth (only 16-bit-based formats allowed).", raco::core::ErrorLevel::ERROR, false};
	}

	static std::map<std::pair<int, int>, std::set<ramses::ETextureFormat>> compatibleTextureFormats = {
		{{LCT_GREY, 8}, {ramses::ETextureFormat::R8}},
		{{LCT_GREY_ALPHA, 8}, {ramses::ETextureFormat::RG8}},
		{{LCT_RGB, 8}, {ramses::ETextureFormat::RGB8, ramses::ETextureFormat::SRGB8}},
		{{LCT_RGBA, 8}, {ramses::ETextureFormat::RGBA8, ramses::ETextureFormat::SRGB8_ALPHA8}},
		{{LCT_RGB, 16}, {ramses::ETextureFormat::RGB16F}},
		{{LCT_RGBA, 16}, {ramses::ETextureFormat::RGBA16F}},
		// palette format is unsupported but should still be convertable
		{{LCT_PALETTE, 8}, {}},
	};

	std::pair<int, int> pngFormat = {colorType, bitdepth};
	if (compatibleTextureFormats.find(pngFormat) == compatibleTextureFormats.end()) {
		return {"Invalid PNG color type.", raco::core::ErrorLevel::ERROR, false};
	}

	auto compatiblePngFormat = compatibleTextureFormats[pngFormat].find(selectedTextureFormat);
	if (compatiblePngFormat == compatibleTextureFormats[pngFormat].end()) {
		auto colorTypeToString = [](auto colorType) {
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

		return {fmt::format("Selected format {} is not equal to PNG color type {} - image will be converted.", ramsesTextureFormatToString(selectedTextureFormat), colorTypeToString(colorType)), raco::core::ErrorLevel::WARNING, true};
	}

	return {"", raco::core::ErrorLevel::NONE, false};
}

int ramsesTextureFormatToPngFormat(ramses::ETextureFormat textureFormat) {
	std::map<ramses::ETextureFormat, int> formats = {
		{ramses::ETextureFormat::R8, LCT_GREY},

		// no LCT_GREY_ALPHA for RG8 because it would not keep the RG channels as intended
		{ramses::ETextureFormat::RG8, LCT_RGB},

		{ramses::ETextureFormat::RGB8, LCT_RGB},
		{ramses::ETextureFormat::RGBA8, LCT_RGBA},
		{ramses::ETextureFormat::RGB16F, LCT_RGB},
		{ramses::ETextureFormat::RGBA16F, LCT_RGBA},
		{ramses::ETextureFormat::SRGB8, LCT_RGB},
		{ramses::ETextureFormat::SRGB8_ALPHA8, LCT_RGBA}
	};

	auto foundFormat = formats.find(textureFormat);

	return (foundFormat == formats.end()) ? LCT_RGBA : foundFormat->second;
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

void normalize16BitColorData(std::vector<unsigned char>& data) {
	// convert 16-bit byte data to 16-bit float data
	// see ramses/integration/TestContent/src/Texture2DFormatScene.cpp in Ramses repo for example data
	assert(data.size() % 2 == 0);

	for (auto i = 0; i < data.size(); i += 2) {
		auto dataHalfFloat = raco::utils::math::twoBytesToHalfFloat(data[i], data[i + 1]);
		data[i] = dataHalfFloat;
		data[i + 1] = dataHalfFloat >> 8;
	}
}

std::vector<unsigned char> generateColorDataWithoutBlueChannel(const std::vector<unsigned char>& data) {
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

}  // namespace raco::ramses_base
