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

#include "FileChangeMonitor.h"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace raco::core {

// Single mesh that can be handed over to Ramses.
// May contain only part of an entire file; see MeshCacheEntry.
class MeshData {
public:
	static constexpr const char* ATTRIBUTE_POSITION {"a_Position"};
	static constexpr const char* ATTRIBUTE_NORMAL   {"a_Normal"};
	static constexpr const char* ATTRIBUTE_TANGENT  {"a_Tangent"};
	static constexpr const char* ATTRIBUTE_BITANGENT{"a_Bitangent"};
	static constexpr const char* ATTRIBUTE_UVMAP    {"a_TextureCoordinate"};
	static constexpr const char* ATTRIBUTE_UVWMAP   {"a_TextureCoordinate"};
	static constexpr const char* ATTRIBUTE_COLOR    {"a_Color"};

	struct IndexBufferRangeInfo {
		uint32_t start;
		uint32_t count;
	};

	enum class VertexAttribDataType {
		VAT_Float = 0,
		VAT_Float2,
		VAT_Float3,
		VAT_Float4
	};

	virtual uint32_t numSubmeshes() const = 0;

	virtual uint32_t numTriangles() const = 0;
	virtual uint32_t numVertices() const = 0;

	virtual std::vector<std::string> getMaterialNames() const = 0;

	virtual const std::vector<uint32_t>& getIndices() const = 0;

	virtual std::map<std::string, std::string> getMetadata() const = 0;

	virtual const std::vector<IndexBufferRangeInfo>& submeshIndexBufferRanges() const = 0;

	virtual uint32_t numAttributes() const = 0;
	virtual std::string attribName(int attribIndex) const = 0;
	//! Size of the attribute buffer in bytes.
	virtual uint32_t attribDataSize(int attribIndex) const = 0;
	virtual uint32_t attribElementCount(int attribIndex) const  = 0;
	virtual VertexAttribDataType attribDataType(int attribIndex) const = 0;
	virtual const char* attribBuffer(int attribIndex) const = 0;

	int attribIndex(const std::string& name) const {
		for (uint32_t i{0}; i < numAttributes(); i++) {
			if (name == attribName(i)) {
				return i;
			}
		}
		return -1;
	}
};

using SharedMeshData = std::shared_ptr<MeshData>;

enum class MeshAnimationInterpolation {
	Linear,
	CubicSpline,
	Step
};

// Animation sampler data holder - currently created using MeshCache::getAnimationSamplerData()
struct MeshAnimationSamplerData {
	MeshAnimationInterpolation interpolation;
	std::vector<float> input;
	std::vector<std::vector<float>> output;

	size_t getOutputComponentSize() {
		if (output.empty()) {
			return 0;
		}
		return output.front().size();
	}

	template <typename DataType>
	std::array<std::vector<DataType>, 3> getOutputData() {
		std::array<std::vector<DataType>, 3> outputData;
		auto animInterpolationIsCubic = (interpolation == raco::core::MeshAnimationInterpolation::CubicSpline);

		auto& tangentInData = outputData[0];
		auto& transformedData = outputData[1];
		auto& tangentOutData = outputData[2];

		if (!animInterpolationIsCubic) {
			if constexpr (std::is_same_v<DataType, std::array<float, 2>>) {
				// edge case: weights
				// see https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_018_MorphTargets.md
				for (auto i = 0; i < output.size(); i += 2) {
					transformedData.push_back({output[i][0], output[i + 1][0]});
				}
			} else {
				for (const auto& vecfKeyframe : output) {
					if constexpr (std::is_same_v<DataType, std::array<float, 3>>) {
						transformedData.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2]});
					} else if constexpr (std::is_same_v<DataType, std::array<float, 4>>) {
						transformedData.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2], vecfKeyframe[3]});
					}
				}
			}
		} else {
			for (auto i = 0; i < output.size(); i += 3) {
				auto& tangentIn = output[i];
				auto& vecfKeyframe = output[i + 1];
				auto& tangentOut = output[i + 2];

				if constexpr (std::is_same_v <DataType, std::array<float, 3>>) {
					tangentInData.push_back({tangentIn[0], tangentIn[1], tangentIn[2]});
					transformedData.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2]});
					tangentOutData.push_back({tangentOut[0], tangentOut[1], tangentOut[2]});
				} else if constexpr (std::is_same_v<DataType, std::array<float, 4>>) {
					tangentInData.push_back({tangentIn[0], tangentIn[1], tangentIn[2], tangentIn[3]});
					transformedData.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2], vecfKeyframe[3]});
					tangentOutData.push_back({tangentOut[0], tangentOut[1], tangentOut[2], tangentOut[3]});
				} else if constexpr (std::is_same_v<DataType, std::array<float, 2>>) {
					tangentInData.push_back({tangentIn[0], tangentIn[1]});
					transformedData.push_back({vecfKeyframe[0], vecfKeyframe[1]});
					tangentOutData.push_back({tangentOut[0], tangentOut[2]});
				}
			}
		}

		return outputData;
	}

	std::string interpolationToString() const {
		switch (interpolation) {
			case raco::core::MeshAnimationInterpolation::Linear: {
				return "Linear";
			}
			case raco::core::MeshAnimationInterpolation::CubicSpline: {
				return "Cubic";
			}
			case raco::core::MeshAnimationInterpolation::Step: {
				return "Step";
			}
			default: {
				return "Invalid";
			}
		}
	}
};

// Low-level one-to-one mapping of animation channel data delivered by tinyglTF.
struct MeshAnimationChannel {
	std::string targetPath;
	int samplerIndex;
	int nodeIndex;
};

// Animation as delivered by tinyglTF.
// This purely uses the indeces from tinyglTF.
struct MeshAnimation {
	std::string name;
	std::vector<MeshAnimationChannel> channels;
};

// A node that may be part of a complex mesh scenegraph.
// Holds information for when we want to translate mesh scenegraph information to our scenegraph.
// Optional values are values that can be de-/activated in the MeshAssetImportDialog.
struct MeshScenegraphNode {
	static inline constexpr int NO_PARENT = -1;

	int parentIndex{NO_PARENT};
	std::vector<std::optional<int>> subMeshIndeces{};
	std::string name;
	struct Transformations {
		std::array<double, 3> scale;
		std::array<double, 3> rotation;
		std::array<double, 3> translation;
	} transformations;

	bool hasParent() {
		return parentIndex > NO_PARENT;
	}
};

struct MeshScenegraph {
	std::vector<std::optional<MeshScenegraphNode>> nodes;
	std::vector<std::optional<std::string>> materials;
	std::vector<std::optional<std::string>> meshes;
	std::vector<std::optional<MeshAnimation>> animations;

	// index of vector is index of the animation that uses the samplers
	std::vector<std::vector<std::optional<std::string>>> animationSamplers;

	void clear() {
		nodes.clear();
		materials.clear();
		meshes.clear();
		animations.clear();
		animationSamplers.clear();
	}
};


// MeshDescriptor contains all information to uniquely identify a mesh within a file.
// This includes at least the absolute path name of the file. It may include more information
// when dealing with more complex file formats like Collada.
struct MeshDescriptor {
	std::string absPath{};
	int submeshIndex{0};
	bool bakeAllSubmeshes{true};
};

// Cache entry for each file.
// Files may contain multiple meshes, e.g. Collada files may contain an entire scene graph
// with multiple meshes for the various meshnodes.
// This class wraps the importer for the file.
class MeshCacheEntry {
public:
	virtual ~MeshCacheEntry() = default;

	// Construct and return MeshData object. Will load file if necessary.
	// Returns nullptr if mesh loading failed.
	virtual SharedMeshData loadMesh(const MeshDescriptor& descriptor) = 0;

	virtual std::string getError() = 0;

	// Discard away the currently loaded file. Use this to force a reload of the file on the next loadMesh.
	virtual void reset() = 0;

	virtual MeshScenegraph* getScenegraph(const std::string& absPath) = 0;

	virtual int getTotalMeshCount() = 0;

	virtual std::shared_ptr<raco::core::MeshAnimationSamplerData> getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) = 0;
};

using UniqueMeshCacheEntry = std::unique_ptr<MeshCacheEntry>;

class MeshCache : public FileChangeMonitor {
public:
	virtual ~MeshCache() = default;

	virtual SharedMeshData loadMesh(const raco::core::MeshDescriptor& descriptor) = 0;
	
	virtual MeshScenegraph* getMeshScenegraph(const raco::core::MeshDescriptor& descriptor) = 0;
	virtual std::string getMeshError(const std::string& absPath) = 0;

	virtual int getTotalMeshCount(const std::string& absPath) = 0;

	virtual std::shared_ptr<raco::core::MeshAnimationSamplerData> getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) = 0;

protected:
	virtual MeshCacheEntry* getLoader(std::string absPath) = 0;
};

}  // namespace raco::core