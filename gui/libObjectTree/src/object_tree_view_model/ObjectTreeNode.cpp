/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view_model/ObjectTreeNode.h"

#include <algorithm>
#include <cassert>
#include <core/ExternalReferenceAnnotation.h>

namespace raco::object_tree::model {

using namespace raco::core;

ObjectTreeNode::ObjectTreeNode(SEditorObject obj, ObjectTreeNode* parent)
	: parent_{parent},
	  type_{ObjectTreeNodeType::EditorObject},
	  representedObject_{obj} {
	if (parent_) {
		parent_->addChild(this);
	}
}

ObjectTreeNode::ObjectTreeNode(ObjectTreeNodeType type, ObjectTreeNode* parent)
	: parent_{parent},
	  type_{type},
	  representedObject_{nullptr} {
	assert(type != ObjectTreeNodeType::EditorObject);

	if (parent_) {
		assert(type != ObjectTreeNodeType::Root);
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
	children_.push_back(child);
}

void ObjectTreeNode::addChildFront(ObjectTreeNode* child) {
	child->setParent(this);
	children_.insert(children_.begin(), child);
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

ObjectTreeNodeType ObjectTreeNode::getType() const {
	return type_;
}

std::string ObjectTreeNode::getDisplayName() const {
	switch (type_) {
		case ObjectTreeNodeType::EditorObject:
			return representedObject_->objectName();
		case ObjectTreeNodeType::ExternalProject:
			return externalProjectPath_;
		case ObjectTreeNodeType::ExtRefGroup:
			return "External References";
		default: 
			return "";
	}
}
std::string ObjectTreeNode::getDisplayType() const {
	switch (type_) {
		case ObjectTreeNodeType::EditorObject:
			return representedObject_->getTypeDescription().typeName;
		case ObjectTreeNodeType::ExternalProject:
			return "Project";
		default:
			return "";
	}
}

std::string ObjectTreeNode::getExternalProjectPath() const {
	return externalProjectPath_;
}

std::string ObjectTreeNode::getExternalProjectName() const {
	return externalProjectName_;
}

void ObjectTreeNode::setBelongsToExternalProject(const std::string& path, const std::string& name) {
	assert(type_ == ObjectTreeNodeType::EditorObject || type_ == ObjectTreeNodeType::ExternalProject);
	externalProjectPath_ = path;
	externalProjectName_ = name;
}

std::string ObjectTreeNode::getID() const {
	switch (type_) {
		case ObjectTreeNodeType::EditorObject:
			return representedObject_->objectID();
		case ObjectTreeNodeType::ExternalProject:
			return externalProjectPath_;
		case ObjectTreeNodeType::ExtRefGroup:
			return "External References";
		case ObjectTreeNodeType::Root:
			return "Root";
		default:
			return "INVALID";
	}
}

SEditorObject ObjectTreeNode::getRepresentedObject() const {
	return representedObject_;
}

void ObjectTreeNode::setParent(ObjectTreeNode* parent) {
	parent_ = parent;
}

}  // namespace raco::object_tree::model