/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/utilities.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"
#include "ramses_adaptor/NodeAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"

#include "core/MeshCacheInterface.h"

namespace raco::ramses_adaptor {

raco::ramses_base::RamsesNodeBinding lookupNodeBinding(const SceneAdaptor* sceneAdaptor, core::SEditorObject node) {
	if (auto nodeAdaptor = sceneAdaptor->lookup<NodeAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<MeshNodeAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<PerspectiveCameraAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	} else if (auto nodeAdaptor = sceneAdaptor->lookup<OrthographicCameraAdaptor>(node)) {
		return nodeAdaptor->nodeBinding();
	}
	return {};
}

void depthFirstSearch(core::SEditorObject object, core::SEditorObjectSet const& instances, core::SEditorObjectSet& sortedObjs, std::vector<DependencyNode>& outSorted);

void depthFirstSearch(data_storage::ReflectionInterface* object, DependencyNode& item, core::SEditorObjectSet const& instances, core::SEditorObjectSet& sortedObjs, std::vector<DependencyNode>& outSorted) {
	for (size_t index = 0; index < object->size(); index++) {
		auto v = (*object)[index];
		if (v->type() == data_storage::PrimitiveType::Ref) {
			auto refValue = v->asRef();
			if (refValue && instances.find(refValue) != instances.end()) {
				depthFirstSearch(refValue, instances, sortedObjs, outSorted);
				item.referencedObjects.insert(refValue);
			}
		} else if (data_storage::hasTypeSubstructure(v->type())) {
			depthFirstSearch(&v->getSubstructure(), item, instances, sortedObjs, outSorted);
		}
	}
}

void depthFirstSearch(core::SEditorObject object, core::SEditorObjectSet const& instances, core::SEditorObjectSet& sortedObjs, std::vector<DependencyNode>& outSorted) {
	using namespace raco::core;

	if (sortedObjs.find(object) != sortedObjs.end()) {
		return;
	}

	DependencyNode item;
	item.object = object;
	depthFirstSearch(object.get(), item, instances, sortedObjs, outSorted);

	outSorted.emplace_back(item);
	sortedObjs.insert(item.object);
}

std::vector<DependencyNode> buildSortedDependencyGraph(core::SEditorObjectSet const& objects) {
	std::vector<DependencyNode> graph;
	graph.reserve(objects.size());
	core::SEditorObjectSet sortedObjs;
	for (auto obj : objects) {
		depthFirstSearch(obj, objects, sortedObjs, graph);
	}
	return graph;
}

ramses_base::RamsesArrayResource arrayResourceFromAttribute(ramses::Scene* scene, core::SharedMeshData mesh, int attribIndex, std::string_view name) {
	auto buffer = mesh->attribBuffer(attribIndex);
	auto elementCount = mesh->attribElementCount(attribIndex);

	switch (mesh->attribDataType(attribIndex)) {
		case core::MeshData::VertexAttribDataType::VAT_Float:
			return ramses_base::ramsesArrayResource(scene, elementCount, reinterpret_cast<const float*>(buffer), name);
		case core::MeshData::VertexAttribDataType::VAT_Float2:
			return ramses_base::ramsesArrayResource(scene, elementCount, reinterpret_cast<const ramses::vec2f*>(buffer), name);
		case core::MeshData::VertexAttribDataType::VAT_Float3:
			return ramses_base::ramsesArrayResource(scene, elementCount, reinterpret_cast<const ramses::vec3f*>(buffer), name);
		case core::MeshData::VertexAttribDataType::VAT_Float4:
			return ramses_base::ramsesArrayResource(scene, elementCount, reinterpret_cast<const ramses::vec4f*>(buffer), name);
	}
	return {};
}

}  // namespace raco::ramses_adaptor