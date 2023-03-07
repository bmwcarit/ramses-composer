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

#include "core/EditorObject.h"

namespace raco::object_tree::model {

enum class ObjectTreeNodeType {
	EditorObject,
	ExternalProject,
	ExtRefGroup,
	TypeParent,
	Root
};

enum class VisibilityState {
	Disabled,
	Invisible,
	Visible,
	Missing
};

class ObjectTreeNode {
public:
	explicit ObjectTreeNode(core::SEditorObject obj, ObjectTreeNode *parent);
	explicit ObjectTreeNode(ObjectTreeNodeType type, ObjectTreeNode *parent);
	explicit ObjectTreeNode(const std::string& typeName);

	~ObjectTreeNode();

    ObjectTreeNode *getParent();
	size_t childCount() const;
	void addChild(ObjectTreeNode *child);
	void addChildFront(ObjectTreeNode *child);
	ptrdiff_t row() const;

	std::vector<ObjectTreeNode*> getChildren();
	ObjectTreeNode *getChild(int row);
	core::SEditorObject getRepresentedObject() const;

	void setParent(ObjectTreeNode *parent);

	ObjectTreeNodeType getType() const;
	std::string getID() const;
	std::vector<std::string> getUserTags() const;
	std::string getDisplayName() const;
	std::string getDisplayType() const;
	std::string getTypeName() const;
	std::string getExternalProjectName() const;
	std::string getExternalProjectPath() const;
	VisibilityState getVisibility() const;
	void setBelongsToExternalProject(const std::string &path, const std::string &name);

protected:
	ObjectTreeNode *parent_;
	ObjectTreeNodeType type_;
	std::string externalProjectPath_ = "";
	std::string externalProjectName_ = "";
	std::string typeName_ = "";
    std::vector<ObjectTreeNode*> children_;
	core::SEditorObject representedObject_;
};
}