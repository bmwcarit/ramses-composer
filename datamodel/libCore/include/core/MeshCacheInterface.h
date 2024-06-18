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

#include "core/EngineInterface.h"

#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace raco::core {

// Single mesh that can be handed over to Ramses.
// May contain only part of an entire file; see MeshCacheEntry.
class MeshData {
public:
	static constexpr const char* ATTRIBUTE_POSITION{"a_Position"};
	static constexpr const char* ATTRIBUTE_NORMAL{"a_Normal"};
	static constexpr const char* ATTRIBUTE_TANGENT{"a_Tangent"};
	static constexpr const char* ATTRIBUTE_BITANGENT{"a_Bitangent"};
	static constexpr const char* ATTRIBUTE_UVMAP{"a_TextureCoordinate"};
	static constexpr const char* ATTRIBUTE_UVWMAP{"a_TextureCoordinate"};
	static constexpr const char* ATTRIBUTE_COLOR{"a_Color"};
	static constexpr const char* ATTRIBUTE_WEIGHTS{"a_Weights"};
	static constexpr const char* ATTRIBUTE_JOINTS{"a_Joints"};

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
	virtual uint32_t attribElementCount(int attribIndex) const = 0;
	virtual VertexAttribDataType attribDataType(int attribIndex) const = 0;
	virtual const char* attribBuffer(int attribIndex) const = 0;

	/**
	 * @brief Non-indexed triangle buffer that can be used for picking in ramses.
	 *
	 * @return Return vector of vertices forming triangles. Each 3 consecutive entries form one triangle.
	 */
	virtual const std::vector<glm::vec3>& triangleBuffer() const = 0;

	int attribIndex(const std::string& name) const {
		for (uint32_t i{0}; i < numAttributes(); i++) {
			if (name == attribName(i)) {
				return i;
			}
		}
		return -1;
	}

	/**
	 * @brief build non-interleaved triangle buffer from the index and vertex buffer data
	*/
	static std::vector<glm::vec3> buildTriangleBuffer(const glm::vec3* vertices, const std::vector<uint32_t>& indices) {
		std::vector<glm::vec3> triangleBuffer;

		// Build non-indexed triangle buffer to be used for picking in ramses
		auto numTriangles = indices.size() / 3;
		for (size_t index = 0; index < numTriangles; index++) {
			triangleBuffer.insert(triangleBuffer.end(),
				{vertices[indices[3 * index]],
					vertices[indices[3 * index + 1]],
					vertices[indices[3 * index + 2]]});
		}
		return triangleBuffer;
	}
};

using SharedMeshData = std::shared_ptr<MeshData>;

/**
 * @brief Holds the data needed to create a LogicEngine SkinBinding. Contains the inverse bind matrices.
 */
struct SkinData {
	static constexpr const char* INV_BIND_MATRICES_UNIFORM_NAME = "u_jointMat";

	std::vector<glm::mat4x4> inverseBindMatrices;

	size_t numSkins;
};

using SharedSkinData = std::shared_ptr<SkinData>;

enum class MeshAnimationInterpolation {
	Step = 0,
	Linear,
	CubicSpline,
	Linear_Quaternion,
	CubicSpline_Quaternion
};

template <typename T>
struct AnimationOutputData {
	std::vector<T> keyFrames;
	std::vector<T> tangentsIn;
	std::vector<T> tangentsOut;

	bool operator==(const AnimationOutputData<T>& rhs) const {
		return keyFrames == rhs.keyFrames &&
			   tangentsIn == rhs.tangentsIn &&
			   tangentsOut == rhs.tangentsOut;
	}
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Animation sampler data holder - currently created using MeshCache::getAnimationSamplerData()
struct AnimationSamplerData {
	MeshAnimationInterpolation interpolation;
	EnginePrimitive componentType;
	size_t componentArraySize;

	std::vector<float> timeStamps;

	using OutputDataVariant = std::variant<
		AnimationOutputData<float>,
		AnimationOutputData<glm::vec2>,
		AnimationOutputData<glm::vec3>,
		AnimationOutputData<glm::vec4>,
		AnimationOutputData<int32_t>,
		AnimationOutputData<glm::ivec2>,
		AnimationOutputData<glm::ivec3>,
		AnimationOutputData<glm::ivec4>,
		AnimationOutputData<std::vector<float>>>;

	OutputDataVariant output;

	size_t getOutputComponentSize() {
		size_t componentSize = componentArraySize;
		return std::visit(
			overloaded{
				[](const AnimationOutputData<float>& data) -> size_t { return 1; },
				[](const AnimationOutputData<glm::vec2>& data) -> size_t { return 2; },
				[](const AnimationOutputData<glm::vec3>& data) -> size_t { return 3; },
				[](const AnimationOutputData<glm::vec4>& data) -> size_t { return 4; },
				[](const AnimationOutputData<int32_t>& data) -> size_t { return 1; },
				[](const AnimationOutputData<glm::ivec2>& data) -> size_t { return 2; },
				[](const AnimationOutputData<glm::ivec3>& data) -> size_t { return 3; },
				[](const AnimationOutputData<glm::ivec4>& data) -> size_t { return 4; },
				[componentSize](const AnimationOutputData<std::vector<float>>& data) -> size_t { return componentSize; }},
			output);
	}

	bool operator==(const AnimationSamplerData& rhs) const {
		return interpolation == rhs.interpolation &&
			   componentType == rhs.componentType &&
			   timeStamps == rhs.timeStamps &&
			   output == rhs.output;
	}
};

using SharedAnimationSamplerData = std::shared_ptr<core::AnimationSamplerData>;

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

struct SkinDescription {
	std::string name;
	std::vector<int> meshNodeIndices;
	std::vector<int> jointNodeIndices;
};

// A node that may be part of a complex mesh scenegraph.
// Holds information for when we want to translate mesh scenegraph information to our scenegraph.
// Optional values are values that can be de-/activated in the MeshAssetImportDialog.
struct MeshScenegraphNode {
	static inline constexpr int NO_PARENT = -1;

	int parentIndex{NO_PARENT};
	std::vector<std::optional<int>> subMeshIndices{};
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
	std::vector<std::optional<SkinDescription>> skins;

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

	virtual const MeshScenegraph* getScenegraph(const std::string& absPath) = 0;

	virtual int getTotalMeshCount() = 0;

	virtual SharedAnimationSamplerData getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) = 0;

	virtual SharedSkinData loadSkin(const std::string& absPath, int skinIndex, std::string& outError) = 0;
};

using UniqueMeshCacheEntry = std::unique_ptr<MeshCacheEntry>;

class MeshCache : public FileChangeMonitor {
public:
	virtual ~MeshCache() = default;

	virtual SharedMeshData loadMesh(const core::MeshDescriptor& descriptor) = 0;

	virtual const MeshScenegraph* getMeshScenegraph(const std::string& absPath) = 0;
	virtual std::string getMeshError(const std::string& absPath) = 0;

	virtual int getTotalMeshCount(const std::string& absPath) = 0;

	virtual SharedAnimationSamplerData getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) = 0;

	virtual SharedSkinData loadSkin(const std::string& absPath, int skinIndex, std::string& outError) = 0;

protected:
	virtual MeshCacheEntry* getLoader(std::string absPath) = 0;
};

}  // namespace raco::core