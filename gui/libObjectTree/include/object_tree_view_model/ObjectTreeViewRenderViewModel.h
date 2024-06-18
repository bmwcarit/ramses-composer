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

#include "ObjectTreeViewDefaultModel.h"

namespace raco::object_tree::model {

class ObjectTreeViewRenderViewModel : public ObjectTreeViewDefaultModel {
	Q_OBJECT

public:
	ObjectTreeViewRenderViewModel(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore);

	std::vector<ColumnIndex> hiddenColumns() const override;
	virtual ColumnIndex defaultSortColumn() const;

protected:
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	std::vector<std::string> creatableTypes(const QModelIndex& index) const override;
	bool canProgramaticallyGotoObject() const override;
private:
	void buildObjectTree() override;

	std::vector<ObjectTreeNode*> buildTaggedNodeSubtree(const std::vector<core::SEditorObject>& objects, const std::set<std::string>& tags, bool parentActive, const std::set<std::string>& materialFilterTags, bool materialFilterExclusive, const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs, core::SEditorObjectSet& inputBuffers);

	void buildRenderLayerSubtree(ObjectTreeNode* layerNode, core::SEditorObject layer, const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs, const std::vector<core::SEditorObject>& topLevelNodes, const std::vector<user_types::SRenderLayer>& renderLayers, core::SEditorObjectSet& inputBuffers);
	void buildRenderPassSubtree(ObjectTreeNode* layerNode, core::SEditorObject layer, const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs, const std::vector<core::SEditorObject>& topLevelNodes, const std::vector<user_types::SRenderLayer>& renderLayers);
	void buildRenderPasses(ObjectTreeNode* rootNode, const std::vector<core::SEditorObject>& children);

	void updateSubscriptions();

	std::vector<components::Subscription> subscriptions_;
};

}  // namespace raco::object_tree::model