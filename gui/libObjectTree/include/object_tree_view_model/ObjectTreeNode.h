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

#include "core/EditorObject.h"

namespace raco::object_tree::model {

class ObjectTreeNode {
public:
	explicit ObjectTreeNode(core::SEditorObject obj = core::SEditorObject(), ObjectTreeNode *parent = nullptr);
	~ObjectTreeNode();

    ObjectTreeNode *getParent();
	size_t childCount() const;
	void addChild(ObjectTreeNode *child);
	ptrdiff_t row() const;

	std::vector<ObjectTreeNode*> getChildren();
	ObjectTreeNode *getChild(int row);
	core::SEditorObject getRepresentedObject() const;

	void setParent(ObjectTreeNode *parent);

protected:
    ObjectTreeNode *parent_;
    std::vector<ObjectTreeNode*> children_;
	core::SEditorObject representedObject_;
};
}