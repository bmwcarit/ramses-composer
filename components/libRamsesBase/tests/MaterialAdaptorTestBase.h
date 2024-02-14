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

#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"

#include "user_types/CubeMap.h"

using namespace raco::user_types;

class MaterialAdaptorTestBase : public RamsesBaseFixture<> {
public:
	SCubeMap create_cubemap(const std::string& name, const std::string& relpath) {
		auto cubeMap = create<CubeMap>(name);
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriBack_ }, (test_path() / relpath).string());
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriBottom_ }, (test_path() / relpath).string());
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriFront_ }, (test_path() / relpath).string());
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriLeft_ }, (test_path() / relpath).string());
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriRight_ }, (test_path() / relpath).string());
		commandInterface.set({ cubeMap, &user_types::CubeMap::uriTop_ }, (test_path() / relpath).string());

		return cubeMap;
	}

	struct struct_prim {
		bool bool_val;
		int int_val;
		double float_val;
		std::array<float, 2> vec2f_val;
		std::array<float, 3> vec3f_val;
		std::array<float, 4> vec4f_val;
		std::array<int32_t, 2> vec2i_val;
		std::array<int32_t, 3> vec3i_val;
		std::array<int32_t, 4> vec4i_val;
	};

	static constexpr struct_prim default_struct_prim_values = {
		true,
		2,
		3.0,
		{0.0, 4.0},
		{0.0, 0.0, 5.0},
		{0.0, 0.0, 0.0, 6.0},
		{0, 7},
		{0, 0, 8},
		{0, 0, 0, 9} };

	void setStructComponents(core::ValueHandle uniformContainerHandle,
		bool bool_val,
		int int_val,
		double float_val,
		std::array<float, 2> vec2f_val,
		std::array<float, 3> vec3f_val,
		std::array<float, 4> vec4f_val,
		std::array<int32_t, 2> vec2i_val,
		std::array<int32_t, 3> vec3i_val,
		std::array<int32_t, 4> vec4i_val) {
		commandInterface.set(uniformContainerHandle.get("b"), bool_val);
		commandInterface.set(uniformContainerHandle.get("i"), int_val);
		commandInterface.set(uniformContainerHandle.get("f"), float_val);

		commandInterface.set(uniformContainerHandle.get("v2"), std::array<double, 2>({ vec2f_val[0], vec2f_val[1] }));
		commandInterface.set(uniformContainerHandle.get("v3"), std::array<double, 3>({ vec3f_val[0], vec3f_val[1], vec3f_val[2] }));
		commandInterface.set(uniformContainerHandle.get("v4"), std::array<double, 4>({ vec4f_val[0], vec4f_val[1], vec4f_val[2], vec4f_val[3] }));

		commandInterface.set(uniformContainerHandle.get("iv2"), vec2i_val);
		commandInterface.set(uniformContainerHandle.get("iv3"), vec3i_val);
		commandInterface.set(uniformContainerHandle.get("iv4"), vec4i_val);
	}

	void setStructComponents(core::ValueHandle uniformContainerHandle, const struct_prim& values) {
		setStructComponents(uniformContainerHandle, values.bool_val, values.int_val, values.float_val,
			values.vec2f_val, values.vec3f_val, values.vec4f_val,
			values.vec2i_val, values.vec3i_val, values.vec4i_val);
	}


	void setStructFloatComponents(core::ValueHandle uniformContainerHandle,
		double float_val,
		std::array<float, 2> vec2f_val,
		std::array<float, 3> vec3f_val,
		std::array<float, 4> vec4f_val) {
		commandInterface.set(uniformContainerHandle.get("f"), float_val);

		commandInterface.set(uniformContainerHandle.get("v2"), std::array<double, 2>({vec2f_val[0], vec2f_val[1]}));
		commandInterface.set(uniformContainerHandle.get("v3"), std::array<double, 3>({vec3f_val[0], vec3f_val[1], vec3f_val[2]}));
		commandInterface.set(uniformContainerHandle.get("v4"), std::array<double, 4>({vec4f_val[0], vec4f_val[1], vec4f_val[2], vec4f_val[3]}));
	}

	void setStructFloatComponents(core::ValueHandle uniformContainerHandle, const struct_prim& values) {
		setStructFloatComponents(uniformContainerHandle, values.float_val,
			values.vec2f_val, values.vec3f_val, values.vec4f_val);
	}

	void linkStructComponents(core::ValueHandle startContainer, core::ValueHandle endContainer) {
		for (auto prop : { "b", "i", "f", "v2", "v3", "v4", "iv2", "iv3", "iv4" }) {
			commandInterface.addLink(startContainer.get(prop), endContainer.get(prop));
		}
	}

	void linkStructIntComponents(core::ValueHandle startContainer, core::ValueHandle endContainer) {
		for (auto prop : {"b", "i", "iv2", "iv3", "iv4"}) {
			commandInterface.addLink(startContainer.get(prop), endContainer.get(prop));
		}
	}

	void checkStructComponents(const ramses::Appearance* appearance, std::string uniformBaseName,
		bool bool_val,
		int int_val,
		double float_val,
		std::array<float, 2> vec2f_val,
		std::array<float, 3> vec3f_val,
		std::array<float, 4> vec4f_val,
		std::array<int32_t, 2> vec2i_val,
		std::array<int32_t, 3> vec3i_val,
		std::array<int32_t, 4> vec4i_val) {
		checkUniformScalar<bool>(appearance, uniformBaseName + "b", bool_val);
		checkUniformScalar<int32_t>(appearance, uniformBaseName + "i", int_val);
		checkUniformScalar<float>(appearance, uniformBaseName + "f", float_val);

		checkUniformScalar<std::array<float, 2>>(appearance, uniformBaseName + "v2", vec2f_val);
		checkUniformScalar<std::array<float, 3>>(appearance, uniformBaseName + "v3", vec3f_val);
		checkUniformScalar<std::array<float, 4>>(appearance, uniformBaseName + "v4", vec4f_val);

		checkUniformScalar<std::array<int32_t, 2>>(appearance, uniformBaseName + "iv2", vec2i_val);
		checkUniformScalar<std::array<int32_t, 3>>(appearance, uniformBaseName + "iv3", vec3i_val);
		checkUniformScalar<std::array<int32_t, 4>>(appearance, uniformBaseName + "iv4", vec4i_val);
	}

	void checkStructComponents(const ramses::Appearance* appearance, std::string uniformBaseName, const struct_prim& values) {
		checkStructComponents(appearance, uniformBaseName, values.bool_val, values.int_val, values.float_val,
			values.vec2f_val, values.vec3f_val, values.vec4f_val,
			values.vec2i_val, values.vec3i_val, values.vec4i_val);
	}

	void checkStructComponents(const core::ValueHandle handle, const ramses::Appearance* appearance, std::string uniformBaseName,
		bool bool_val,
		int int_val,
		double float_val,
		std::array<float, 2> vec2f_val,
		std::array<float, 3> vec3f_val,
		std::array<float, 4> vec4f_val,
		std::array<int32_t, 2> vec2i_val,
		std::array<int32_t, 3> vec3i_val,
		std::array<int32_t, 4> vec4i_val) {
		checkUniformScalar<bool>(handle.get("b"), appearance, uniformBaseName + "b", bool_val);
		checkUniformScalar<int32_t>(handle.get("i"), appearance, uniformBaseName + "i", int_val);
		checkUniformScalar<float>(handle.get("f"), appearance, uniformBaseName + "f", float_val);

		checkUniformScalar<std::array<float, 2>>(handle.get("v2"), appearance, uniformBaseName + "v2", vec2f_val);
		checkUniformScalar<std::array<float, 3>>(handle.get("v3"), appearance, uniformBaseName + "v3", vec3f_val);
		checkUniformScalar<std::array<float, 4>>(handle.get("v4"), appearance, uniformBaseName + "v4", vec4f_val);

		checkUniformScalar<std::array<int32_t, 2>>(handle.get("iv2"), appearance, uniformBaseName + "iv2", vec2i_val);
		checkUniformScalar<std::array<int32_t, 3>>(handle.get("iv3"), appearance, uniformBaseName + "iv3", vec3i_val);
		checkUniformScalar<std::array<int32_t, 4>>(handle.get("iv4"), appearance, uniformBaseName + "iv4", vec4i_val);
	}

	void checkStructComponents(const core::ValueHandle handle, const ramses::Appearance* appearance, std::string uniformBaseName, const struct_prim& values) {
		checkStructComponents(handle, appearance, uniformBaseName, values.bool_val, values.int_val, values.float_val,
			values.vec2f_val, values.vec3f_val, values.vec4f_val,
			values.vec2i_val, values.vec3i_val, values.vec4i_val);
	}
};
