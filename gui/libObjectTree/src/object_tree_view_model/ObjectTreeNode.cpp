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
#include "user_types/Node.h"

#include <algorithm>
#include <cassert>
#include "core/ProxyTypes.h"

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

ObjectTreeNode::ObjectTreeNode(const std::string& typeName)
	: parent_{nullptr},
	  type_{ObjectTreeNodeType::TypeParent},
      representedObject_{nullptr},
	  typeName_{typeName} {
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

namespace {
	std::map<std::string, std::string> irregularObjectTypePluralNames{
		{raco::serialization::proxy::meshTypeName, "Meshes"},
		{raco::serialization::proxy::renderPassTypeName, "RenderPasses"}};
}

std::string ObjectTreeNode::getDisplayName() const {
	switch (type_) {
		case ObjectTreeNodeType::EditorObject:
			return representedObject_->objectName();
		case ObjectTreeNodeType::ExternalProject:
			return externalProjectPath_;
		case ObjectTreeNodeType::ExtRefGroup:
			return "External References";
		case ObjectTreeNodeType::TypeParent:
			if (irregularObjectTypePluralNames.find(typeName_) != irregularObjectTypePluralNames.end()) {
				return irregularObjectTypePluralNames[typeName_];
			}

			return typeName_ + "s";
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

std::string ObjectTreeNode::getTypeName() const {
	return typeName_;
}

std::string ObjectTreeNode::getExternalProjectPath() const {
	return externalProjectPath_;
}

std::string ObjectTreeNode::getExternalProjectName() const {
	return externalProjectName_;
}

VisibilityState ObjectTreeNode::getVisibility() const {
	auto visibility = VisibilityState::Missing;
	if (representedObject_) {
		if (auto node = representedObject_->as<raco::user_types::Node>()) {
			// If Enabled property is set to False, Visibility is ignored by runtime.
			if (*node->enabled_) {
				visibility = *node->visibility_ ? VisibilityState::Visible : VisibilityState::Invisible;
			} else {
				visibility = VisibilityState::Disabled;
			}
		}
	}
	return visibility;
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
		case ObjectTreeNodeType::TypeParent:
			return parent_->getType() == ObjectTreeNodeType::ExtRefGroup
				? "external " + typeName_
				: typeName_;
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