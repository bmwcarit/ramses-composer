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
	glTFBufferData(const tinygltf::Model &scene, int accessorIndex, const std::set<int> &&allowedComponentTypes = {})
		: scene_(scene),
		  accessor_(scene_.accessors[accessorIndex]),
		  view_(scene_.bufferViews[accessor_.bufferView]),
		  bufferBytes(scene_.buffers[view_.buffer].data) {
		if (!allowedComponentTypes.empty() && allowedComponentTypes.find(accessor_.componentType) == allowedComponentTypes.end()) {
			LOG_ERROR(raco::log_system::MESH_LOADER, "glTF buffer accessor '{}' has invalid data type {}", accessor_.name, accessor_.componentType);
		}
	}

	template <typename T>
	std::vector<T> getDataAt(size_t index, bool useComponentSize = true) {
		auto componentSize = (useComponentSize) ? accessor_.ByteStride(view_) / sizeof(T) : 1;
		assert(componentSize > 0);

		auto firstByte = reinterpret_cast<const T *>(&bufferBytes[(accessor_.byteOffset + view_.byteOffset)]);
		std::vector<T> values(componentSize);

		for (int i = 0; i < values.size(); ++i) {
			values[i] = firstByte[index * componentSize + i];
		}

		return values;
	}

	const tinygltf::Model &scene_;
	const tinygltf::Accessor &accessor_;
	const tinygltf::BufferView &view_;
	const std::vector<unsigned char> &bufferBytes;
};