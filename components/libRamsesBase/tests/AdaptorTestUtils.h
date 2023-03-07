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
	ramses::UniformInput input;
	if constexpr (std::is_same<T, int32_t>::value) {
		int32_t ivalue;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueInt32(input, ivalue);
		EXPECT_EQ(ivalue, value);
	}
	else if constexpr (std::is_same<T, float>::value) {
		float fvalue;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueFloat(input, fvalue);
		EXPECT_EQ(fvalue, value);
	}
	else if constexpr (std::is_same<T, std::array<float, 2>>::value) {
		std::array<float, 2> value2f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector2f(input, value2f[0], value2f[1]);
		EXPECT_EQ(value2f, value);
	}
	else if constexpr (std::is_same<T, std::array<float, 3>>::value) {
		std::array<float, 3> value3f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector3f(input, value3f[0], value3f[1], value3f[2]);
		EXPECT_EQ(value3f, value);
	}
	else if constexpr (std::is_same<T, std::array<float, 4>>::value) {
		std::array<float, 4> value4f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector4f(input, value4f[0], value4f[1], value4f[2], value4f[3]);
		EXPECT_EQ(value4f, value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 2>>::value) {
		std::array<int32_t, 2> value2i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector2i(input, value2i[0], value2i[1]);
		EXPECT_EQ(value2i, value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 3>>::value) {
		std::array<int32_t, 3> value3i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector3i(input, value3i[0], value3i[1], value3i[2]);
		EXPECT_EQ(value3i, value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 4>>::value) {
		std::array<int32_t, 4> value4i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector4i(input, value4i[0], value4i[1], value4i[2], value4i[3]);
		EXPECT_EQ(value4i, value);
	}
	else if constexpr (std::is_same<T, const ramses::TextureSampler*>::value) {
		const ramses::TextureSampler* u_sampler_texture;
		EXPECT_EQ(ramses::StatusOK, appearance->getEffect().findUniformInput(name.c_str(), input));
		EXPECT_EQ(ramses::StatusOK, appearance->getInputTexture(input, u_sampler_texture));
		EXPECT_EQ(u_sampler_texture, value);
	}
	else if constexpr (std::is_same<T, const ramses::TextureSamplerMS*>::value) {
		const ramses::TextureSamplerMS* u_sampler_texture;
		EXPECT_EQ(ramses::StatusOK, appearance->getEffect().findUniformInput(name.c_str(), input));
		EXPECT_EQ(ramses::StatusOK, appearance->getInputTextureMS(input, u_sampler_texture));
		EXPECT_EQ(u_sampler_texture, value);
	}
	else {
		EXPECT_TRUE(false);
	}
}

template <typename T>
void checkUniformScalar(const raco::core::ValueHandle& handle, const ramses::Appearance* appearance, const std::string& name, const T value) {
	using namespace raco;

	checkUniformScalar(appearance, name, value);
	if constexpr (std::is_same<T, int32_t>::value) {
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
	ramses::UniformInput input;
	if constexpr (std::is_same<T, int32_t>::value) {
		std::array<int32_t, length> ivalue;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueInt32(input, length, ivalue.data());
		EXPECT_EQ(ivalue[index], value);
	}
	else if constexpr (std::is_same<T, float>::value) {
		std::array<float, length> fvalue;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueFloat(input, length, fvalue.data());
		EXPECT_EQ(fvalue[index], value);
	}
	else if constexpr (std::is_same<T, std::array<float, 2>>::value) {
		std::array<std::array<float, 2>, length> value2f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector2f(input, length, value2f.data()->data());
		EXPECT_EQ(value2f[index], value);
	}
	else if constexpr (std::is_same<T, std::array<float, 3>>::value) {
		std::array<std::array<float, 3>, length> value3f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector3f(input, length, value3f.data()->data());
		EXPECT_EQ(value3f[index], value);
	}
	else if constexpr (std::is_same<T, std::array<float, 4>>::value) {
		std::array<std::array<float, 4>, length> value4f;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector4f(input, length, value4f.data()->data());
		EXPECT_EQ(value4f[index], value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 2>>::value) {
		std::array<std::array<int32_t, 2>, length> value2i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector2i(input, length, value2i.data()->data());
		EXPECT_EQ(value2i[index], value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 3>>::value) {
		std::array<std::array<int32_t, 3>, length> value3i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector3i(input, length, value3i.data()->data());
		EXPECT_EQ(value3i[index], value);
	}
	else if constexpr (std::is_same<T, std::array<int32_t, 4>>::value) {
		std::array<std::array<int32_t, 4>, length> value4i;
		appearance->getEffect().findUniformInput(name.c_str(), input);
		appearance->getInputValueVector4i(input, length, value4i.data()->data());
		EXPECT_EQ(value4i[index], value);
	}
	else {
		EXPECT_TRUE(false);
	}
}
