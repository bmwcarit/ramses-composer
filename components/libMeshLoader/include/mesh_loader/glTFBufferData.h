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

#include <log_system/log.h>
#include <set>
#include <tiny_gltf.h>

struct glTFBufferData {
	glTFBufferData(const tinygltf::Model &scene, int accessorIndex, const std::set<int> &allowedComponentTypes, const std::set<int> &allowedTypes)
		: scene_(scene),
		  accessor_(scene_.accessors[accessorIndex]),
		  view_(scene_.bufferViews[accessor_.bufferView]),
		  bufferBytes(scene_.buffers[view_.buffer].data) {
		if (!allowedComponentTypes.empty() && allowedComponentTypes.find(accessor_.componentType) == allowedComponentTypes.end()) {
			LOG_ERROR(raco::log_system::MESH_LOADER, "glTF buffer accessor '{}' has invalid component type '{}'", accessor_.name, accessor_.componentType);
		}
		if (!allowedTypes.empty() && allowedTypes.find(accessor_.type) == allowedTypes.end()) {
			LOG_ERROR(raco::log_system::MESH_LOADER, "glFT buffer accessor '{}' has invalid elemenet type '{}'", accessor_.name, accessor_.type);
		}
	}

	int numComponents() const {
		static const std::map<int, int> numComponentsForType{
			{TINYGLTF_TYPE_SCALAR, 1},
			{TINYGLTF_TYPE_VEC2, 2},
			{TINYGLTF_TYPE_VEC3, 3},
			{TINYGLTF_TYPE_VEC4, 4},
			{TINYGLTF_TYPE_MAT2, 4},
			{TINYGLTF_TYPE_MAT3, 9},
			{TINYGLTF_TYPE_MAT4, 16}};
		return numComponentsForType.at(accessor_.type);
	}

	int type() const {
		return accessor_.type;
	}

	template <typename T, typename U = T>
	T getDataArray(size_t index, bool useComponentSize = true) const {
		auto componentSize = (useComponentSize) ? accessor_.ByteStride(view_) / sizeof(typename T::value_type) : 1;
		assert(componentSize > 0);

		auto firstByte = reinterpret_cast<const typename U::value_type *>(&bufferBytes[(accessor_.byteOffset + view_.byteOffset)]);

		T values;

		for (int i = 0; i < values.size(); ++i) {
			values[i] = static_cast<typename T::value_type>(firstByte[index * componentSize + i]);
		}

		return values;
	}

	template <typename T, typename U = T>
	std::vector<T> getDataAt(size_t index, bool useComponentSize = true) const {
		auto componentSize = (useComponentSize) ? accessor_.ByteStride(view_) / sizeof(U) : 1;
		assert(componentSize > 0);

		auto firstByte = reinterpret_cast<const U *>(&bufferBytes[(accessor_.byteOffset + view_.byteOffset)]);
		
		std::vector<T> values(numComponents());

		for (int i = 0; i < values.size(); ++i) {
			values[i] = static_cast<T>(firstByte[index * componentSize + i]);
		}

		return values;
	}

	std::vector<float> getNormalizedData(size_t index, bool useComponentSize = true) {
		switch (accessor_.componentType) {
			case TINYGLTF_PARAMETER_TYPE_FLOAT:
				return getDataAt<float>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_BYTE:
				return normalize(getDataAt<int8_t>(index, useComponentSize));
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				return normalize(getDataAt<uint8_t>(index, useComponentSize));
				break;

			case TINYGLTF_PARAMETER_TYPE_SHORT:
				return normalize(getDataAt<int16_t>(index, useComponentSize));
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				return normalize(getDataAt<uint16_t>(index, useComponentSize));
				break;

			case TINYGLTF_PARAMETER_TYPE_INT:
				return normalize(getDataAt<int32_t>(index, useComponentSize));
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				return normalize(getDataAt<uint32_t>(index, useComponentSize));
				break;
		}
		return {};
	}

	template<typename T>
	std::vector<T> getConvertedData(size_t index, bool useComponentSize = true) {
		switch (accessor_.componentType) {
			case TINYGLTF_PARAMETER_TYPE_FLOAT:
				return getDataAt<T>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_BYTE:
				return getDataAt<T, int8_t>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				return getDataAt<T, uint8_t>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_SHORT:
				return getDataAt<T, int16_t>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				return getDataAt<T, uint16_t>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_INT:
				return getDataAt<T, int32_t>(index, useComponentSize);
				break;

			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				return getDataAt<T, uint32_t>(index, useComponentSize);
				break;

		}
		return {};
	}

	template<typename T>
	std::vector<float> normalize(const std::vector<T> &data){
		std::vector<float> result(data.size());
		for (auto i = 0; i < data.size(); i++) {
			result[i] = std::max(-1.0F, data[i] / static_cast<float>(std::numeric_limits<T>::max()));
		}
		return result;
	}

	const tinygltf::Model &scene_;
	const tinygltf::Accessor &accessor_;
	const tinygltf::BufferView &view_;
	const std::vector<unsigned char> &bufferBytes;
};