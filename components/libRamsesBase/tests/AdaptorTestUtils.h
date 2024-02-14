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

#include "testing/TestUtil.h"

template <typename T>
void checkUniformScalar(const ramses::Appearance* appearance, const std::string& name, const T value) {
	ramses::UniformInput input = appearance->getEffect().findUniformInput(name.c_str()).value();
	if constexpr (std::is_same<T, bool>::value) {
		bool bvalue;
		appearance->getInputValue(input, bvalue);
		EXPECT_EQ(bvalue, value);
	}
	else if constexpr (std::is_same<T, int32_t>::value) {
		int32_t ivalue;
		appearance->getInputValue(input, ivalue);
		EXPECT_EQ(ivalue, value);
	}
	else if constexpr (std::is_same<T, float>::value) {
		float fvalue;
		appearance->getInputValue(input, fvalue);
		EXPECT_EQ(fvalue, value);
	}
	else if constexpr (std::is_same<T, std::array<float, 2>>::value) {
		ramses::vec2f value2f;
		appearance->getInputValue(input, value2f);
		EXPECT_EQ(value2f, glm::vec2(value[0], value[1]));
	}
	else if constexpr (std::is_same<T, std::array<float, 3>>::value) {
		ramses::vec3f value3f;
		appearance->getInputValue(input, value3f);
		EXPECT_EQ(value3f, glm::vec3(value[0], value[1], value[2]));
	}
	else if constexpr (std::is_same<T, std::array<float, 4>>::value) {
		ramses::vec4f value4f;
		appearance->getInputValue(input, value4f);
		EXPECT_EQ(value4f, glm::vec4(value[0], value[1], value[2], value[3]));
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 2>>::value) {
		ramses::vec2i value2i;
		appearance->getInputValue(input, value2i);
		EXPECT_EQ(value2i, glm::ivec2(value[0], value[1]));
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 3>>::value) {
		ramses::vec3i value3i;
		appearance->getInputValue(input, value3i);
		EXPECT_EQ(value3i, glm::ivec3(value[0], value[1], value[2]));
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 4>>::value) {
		ramses::vec4i value4i;
		appearance->getInputValue(input, value4i);
		EXPECT_EQ(value4i, glm::ivec4(value[0], value[1], value[2], value[3]));
	}
	else if constexpr (std::is_same<T, const ramses::TextureSampler*>::value) {
		const ramses::TextureSampler* u_sampler_texture;
		EXPECT_TRUE(appearance->getInputTexture(input, u_sampler_texture));
		EXPECT_EQ(u_sampler_texture, value);
	}
	else if constexpr (std::is_same<T, const ramses::TextureSamplerMS*>::value) {
		const ramses::TextureSamplerMS* u_sampler_texture;
		EXPECT_TRUE(appearance->getInputTextureMS(input, u_sampler_texture));
		EXPECT_EQ(u_sampler_texture, value);
	}
	else {
		EXPECT_TRUE(false);
	}
}

template <typename T>
void checkUniformScalar(const core::ValueHandle& handle, const ramses::Appearance* appearance, const std::string& name, const T value) {
	using namespace raco;

	checkUniformScalar(appearance, name, value);
	if constexpr (std::is_same<T, bool>::value) {
		EXPECT_EQ(handle.asBool(), value);
	} else if constexpr (std::is_same<T, int32_t>::value) {
		EXPECT_EQ(handle.asInt(), value);
	} else if constexpr (std::is_same<T, float>::value) {
		EXPECT_EQ(handle.asDouble(), value);
	} else if constexpr (std::is_same<T, std::array<float, 2>>::value) {
		checkVec2fValue(handle, value);
	} else if constexpr (std::is_same<T, std::array<float, 3>>::value) {
		checkVec3fValue(handle, value);
	} else if constexpr (std::is_same<T, std::array<float, 4>>::value) {
		checkVec4fValue(handle, value);
	} else if constexpr (std::is_same<T, std::array<int32_t, 2>>::value) {
		checkVec2iValue(handle, value);
	} else if constexpr (std::is_same<T, std::array<int32_t, 3>>::value) {
		checkVec3iValue(handle, value);
	} else if constexpr (std::is_same<T, std::array<int32_t, 4>>::value) {
		checkVec4iValue(handle, value);
	} else {
		EXPECT_TRUE(false);
	}
}


template <typename T, int length>
void checkUniformVector(const ramses::Appearance* appearance, const std::string& name, int index, const T& value) {
	ramses::UniformInput input = appearance->getEffect().findUniformInput(name.c_str()).value();
	if constexpr (std::is_same<T, bool>::value) {
		std::array<bool, length> ivalue;
		appearance->getInputValue(input, length, ivalue.data());
		EXPECT_EQ(ivalue[index], value) << fmt::format(" Property name '{}' index = {}", name, index);
	} 
	else if constexpr (std::is_same<T, int32_t>::value) {
		std::array<int32_t, length> ivalue;
		appearance->getInputValue(input, length, ivalue.data());
		EXPECT_EQ(ivalue[index], value) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, float>::value) {
		std::array<float, length> fvalue;
		appearance->getInputValue(input, length, fvalue.data());
		EXPECT_EQ(fvalue[index], value) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<float, 2>>::value) {
		std::array<ramses::vec2f, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::vec2(value[0], value[1])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<float, 3>>::value) {
		std::array<ramses::vec3f, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::vec3(value[0], value[1], value[2])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<float, 4>>::value) {
		std::array<ramses::vec4f, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::vec4(value[0], value[1], value[2], value[3])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 2>>::value) {
		std::array<ramses::vec2i, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::ivec2(value[0], value[1])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 3>>::value) {
		std::array<ramses::vec3i, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::ivec3(value[0], value[1], value[2])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 4>>::value) {
		std::array<ramses::vec4i, length> values;
		appearance->getInputValue(input, length, values.data());
		EXPECT_EQ(values[index], glm::ivec4(value[0], value[1], value[2], value[3])) << fmt::format(" Property name '{}' index = {}", name, index);
	}
	else {
		EXPECT_TRUE(false);
	}
}
