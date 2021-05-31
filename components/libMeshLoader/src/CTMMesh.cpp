/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mesh_loader/CTMMesh.h"

#include <openctmpp.h>

namespace raco::mesh_loader {

using namespace raco::core;

CTMMesh::CTMMesh(CTMimporter& importer) {
	numTriangles_ = importer.GetInteger(CTM_TRIANGLE_COUNT);
	numVertices_ = importer.GetInteger(CTM_VERTEX_COUNT);

	auto indices = importer.GetIntegerArray(CTM_INDICES);
	indexBuffer_ = std::vector<uint32_t>(indices, indices + 3 * numTriangles_);

	auto vertices = importer.GetFloatArray(CTM_VERTICES);
	attributes_.emplace_back(Attribute{
		ATTRIBUTE_POSITION,
		VertexAttribDataType::VAT_Float3,
		std::vector<float>(vertices, vertices + 3 * numVertices_)});

	if (importer.GetInteger(CTM_HAS_NORMALS) == CTM_TRUE) {
		auto normals = importer.GetFloatArray(CTM_NORMALS);
		attributes_.emplace_back(Attribute{
			ATTRIBUTE_NORMAL,
			VertexAttribDataType::VAT_Float3,
			std::vector<float>(normals, normals + 3 * numVertices_)});
	}

	for (unsigned i = 0; i < importer.GetInteger(CTM_UV_MAP_COUNT); i++) {
		CTMenum mapIndex = CTMenum(CTM_UV_MAP_1 + i);
		auto array = importer.GetFloatArray(mapIndex);
		attributes_.emplace_back(Attribute{
			importer.GetUVMapString(mapIndex, CTM_NAME),
			VertexAttribDataType::VAT_Float2,
			std::vector<float>(array, array + 2 * numVertices_)});
	}

	for (unsigned i = 0; i < importer.GetInteger(CTM_ATTRIB_MAP_COUNT); i++) {
		CTMenum mapIndex = CTMenum(CTM_ATTRIB_MAP_1 + i);
		auto array = importer.GetFloatArray(mapIndex);
		attributes_.emplace_back(Attribute{
			importer.GetAttribMapString(mapIndex, CTM_NAME),
			VertexAttribDataType::VAT_Float4,
			std::vector<float>(array, array + 4 * numVertices_)});
	}

	submeshIndexBufferRanges_ = {{0, 3 * numTriangles_}};
}

uint32_t CTMMesh::numSubmeshes() const {
	return 1;
}

uint32_t CTMMesh::numTriangles() const {
	return numTriangles_;
}

uint32_t CTMMesh::numVertices() const {
	return numVertices_;
}

std::vector<std::string> CTMMesh::getMaterialNames() const {
	return {"material"};
}

const std::vector<uint32_t>& CTMMesh::getIndices() const {
	return indexBuffer_;
}

const std::vector<MeshData::IndexBufferRangeInfo>& CTMMesh::submeshIndexBufferRanges() const {
	return submeshIndexBufferRanges_;
}

uint32_t CTMMesh::numAttributes() const {
	return static_cast<uint32_t>(attributes_.size());
}

std::string CTMMesh::attribName(int attribIndex) const {
	return attributes_.at(attribIndex).name;
}

uint32_t CTMMesh::attribDataSize(int attribIndex) const {
	return static_cast<uint32_t>(attributes_.at(attribIndex).data.size() * sizeof(float));
}

uint32_t CTMMesh::attribElementCount(int attribIndex) const {
	switch (attributes_.at(attribIndex).type) {
		case VertexAttribDataType::VAT_Float2:
			return static_cast<uint32_t>(attributes_.at(attribIndex).data.size() / 2);
		case VertexAttribDataType::VAT_Float3:
			return static_cast<uint32_t>(attributes_.at(attribIndex).data.size() / 3);
		case VertexAttribDataType::VAT_Float4:
			return static_cast<uint32_t>(attributes_.at(attribIndex).data.size() / 4);
		default:
			return static_cast<uint32_t>(attributes_.at(attribIndex).data.size());
	}
}

MeshData::VertexAttribDataType CTMMesh::attribDataType(int attribIndex) const {
	return attributes_.at(attribIndex).type;
}

const char* CTMMesh::attribBuffer(int attribIndex) const {
	return reinterpret_cast<const char*>(attributes_.at(attribIndex).data.data());
}

}  // namespace raco::mesh_loader