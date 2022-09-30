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

#include "EditorObject.h"
#include "Link.h"

#include <map>
#include <set>

namespace raco::core {

/**
 * @brief Maintains the connection graph of all non-weak links in the project and implements loop detection.
 *
 * Weak links are not relevant for loop detection and are therefore not included in the link graph.
 */
class LinkGraph {
public:
	void addLink(SLink link);
	void removeLink(SLink link);

	void removeAllLinks();

	bool createsLoop(const PropertyDescriptor& start, const PropertyDescriptor& end) const;

private:
	bool depthFirstSearch(SEditorObject current, SEditorObject obj, SEditorObjectSet& visited) const;

	// Outer map is indexed by start object, inner map is indexed by end object and
	// contains set of link descriptors between start and end objects.
	std::map<SEditorObject, std::map<SEditorObject, std::set<LinkDescriptor>>> graph;
};

}  // namespace raco::core