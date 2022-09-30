/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/LinkGraph.h"

namespace raco::core {

void LinkGraph::addLink(SLink link) {
	if (!*link->isWeak_) {
		graph[*link->startObject_][*link->endObject_].insert(link->descriptor());
	}
}

void LinkGraph::removeLink(const SLink link) {
	auto startIt = graph.find(*link->startObject_);
	if (startIt != graph.end()) {
		auto& mapByEnd = startIt->second;
		auto endIt = mapByEnd.find(*link->endObject_);
		if (endIt != mapByEnd.end()) {
			endIt->second.erase(link->descriptor());
			if (endIt->second.empty()) {
				mapByEnd.erase(*link->endObject_);
				if (mapByEnd.empty()) {
					graph.erase(*link->startObject_);
				}
			}
		}
	}
}

void LinkGraph::removeAllLinks() {
	graph.clear();
}

bool LinkGraph::createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const {
	auto startObj = start.object();
	auto endObj = end.object();
	if (startObj == endObj) {
		return true;
	}
	if (graph.find(endObj) != graph.end()) {
		SEditorObjectSet visited;
		return depthFirstSearch(endObj, startObj, visited);
	}
	return false;
}

bool LinkGraph::depthFirstSearch(SEditorObject current, SEditorObject obj, SEditorObjectSet& visited) const {
	if (current == obj) {
		return true;
	}
	if (visited.find(current) != visited.end()) {
		return false;
	}
	auto it = graph.find(current);
	if (it != graph.end()) {
		for (auto& [depObj, dummySet] : it->second) {
			if (depthFirstSearch(depObj, obj, visited)) {
				return true;
			}
		}
	}
	visited.insert(current);
	return false;
}

}  // namespace raco::core