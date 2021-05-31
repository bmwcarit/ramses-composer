/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view_model/ObjectTreeNode.h"

#include <algorithm>

namespace raco::object_tree::model {

using namespace raco::core;

ObjectTreeNode::ObjectTreeNode(SEditorObject obj, ObjectTreeNode* parent)
	: parent_{parent},
	  representedObject_{obj} {
	if (parent_) {
		parent_->addChild(this);
	}
}

ObjectTreeNode::~ObjectTreeNode() {
	for (auto child : children_) {
		delete child;
	}
}

ObjectTreeNode* ObjectTreeNode::getParent() {
	return parent_;
}

size_t ObjectTreeNode::childCount() const {
	return children_.size();
}

void ObjectTreeNode::addChild(ObjectTreeNode* child) {
	child->setParent(this);
	children_.emplace_back(child);
}

ptrdiff_t ObjectTreeNode::row() const {
	if (parent_) {
		auto nodeNeighbors = parent_->getChildren();
		auto myPosition = std::find_if(nodeNeighbors.begin(), nodeNeighbors.end(), [&](const auto* neighborNode) {
			return neighborNode == this;
		});
		return std::distance(nodeNeighbors.begin(), myPosition);
	}
	return 0;
}

std::vector<ObjectTreeNode*> ObjectTreeNode::getChildren() {
	return children_;
}

ObjectTreeNode* ObjectTreeNode::getChild(int row) {
	if (row >= 0 && row < childCount()) {
		return children_[row];
	}
	return nullptr;
}

SEditorObject ObjectTreeNode::getRepresentedObject() const {
	return representedObject_;
}

void ObjectTreeNode::setParent(ObjectTreeNode* parent) {
	parent_ = parent;
}

}  // namespace raco::object_tree::model