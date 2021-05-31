/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "FileChangeMonitor.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

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


// A node that may be part of a complex mesh scenegraph.
// Holds information for when we want to translate mesh scenegraph information to our scenegraph.
struct MeshScenegraphNode {
	static inline constexpr int NO_PARENT = -1;

	int parentIndex{NO_PARENT};
	std::vector<unsigned> subMeshIndeces{};
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
	std::vector<MeshScenegraphNode> nodes;
	std::vector<std::string> materials;
	std::vector<std::string> meshes;

	void clear() {
		nodes.clear();
		materials.clear();
		meshes.clear();
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

	virtual MeshScenegraph getScenegraph(bool bakeAllSubmeshes) = 0;

	virtual int getTotalMeshCount(bool bakeAllSubmeshes) = 0;
};

using UniqueMeshCacheEntry = std::unique_ptr<MeshCacheEntry>;

class MeshCache : public FileChangeMonitor {
public:
	virtual ~MeshCache() = default;

	virtual SharedMeshData loadMesh(const raco::core::MeshDescriptor& descriptor) = 0;
	
	virtual MeshScenegraph getMeshScenegraph(const std::string& absPath, bool bakeAllSubmeshes) = 0;
	virtual std::string getMeshError(const std::string& absPath) = 0;

	virtual int getTotalMeshCount(const std::string& absPath, bool bakeAllSubmeshes) = 0;

protected:
	virtual MeshCacheEntry* getLoader(std::string absPath) = 0;
};

}  // namespace raco::core