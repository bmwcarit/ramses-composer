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
#include "user_types/MeshNode.h"

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

ObjectTreeNode::ObjectTreeNode(ObjectTreeNodeType type, const std::string& typeName, ObjectTreeNode* parent)
	: parent_(parent),
	  type_(type),
	  representedObject_{nullptr},
	  typeName_(typeName) {
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
		{serialization::proxy::meshTypeName, "Meshes"},
		{serialization::proxy::renderPassTypeName, "RenderPasses"},
		{serialization::proxy::blitPassTypeName, "BlitPasses"},
		{serialization::proxy::renderBufferMSTypeName, "RenderBufferMS"}};
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
		case ObjectTreeNodeType::Tag:
			return typeName_;
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

VisibilityState ObjectTreeNode::getPreviewVisibility() const {
	auto visibility = VisibilityState::Missing;
	if (representedObject_) {
		if (auto node = representedObject_->as<user_types::Node>()) {
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

VisibilityState ObjectTreeNode::getAbstractViewVisibility() const {
	if (representedObject_) {
		if (auto node = representedObject_->as<user_types::MeshNode>()) {
			return *node->editorVisibility_ ? VisibilityState::Visible : VisibilityState::Invisible;
		}
	}
	return VisibilityState::Missing;
}

void ObjectTreeNode::setExternalProjectInfo(const std::string& path, const std::string& name) {
	assert(type_ == ObjectTreeNodeType::EditorObject || type_ == ObjectTreeNodeType::ExternalProject);
	externalProjectPath_ = path;
	externalProjectName_ = name;
}

std::string ObjectTreeNode::getInputBuffers() const {
	return inputBuffers_;
}

std::string ObjectTreeNode::getOutputBuffers() const {
	return outputBuffers_;
}

std::optional<int> ObjectTreeNode::getRenderOrder() const {
	return renderOrder_;
}

void ObjectTreeNode::setBuffers(const std::string& inputBuffers, const std::string& outputBuffers) {
	inputBuffers_ = inputBuffers;
	outputBuffers_ = outputBuffers;
}

void ObjectTreeNode::setRenderOrder(int order) {
	renderOrder_ = order;
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

std::vector<std::string> ObjectTreeNode::getUserTags() const {
	if (type_ == ObjectTreeNodeType::EditorObject) {
		return representedObject_->as<user_types::BaseObject>()->userTags_->asVector<std::string>();
	}
	return {};
}

SEditorObject ObjectTreeNode::getRepresentedObject() const {
	return representedObject_;
}
void ObjectTreeNode::setParent(ObjectTreeNode* parent) {
	parent_ = parent;
}

}  // namespace raco::object_tree::model