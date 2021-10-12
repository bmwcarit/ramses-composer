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

#include "core/MeshCacheInterface.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <unordered_set>

class QGridLayout;

namespace raco::common_widgets {

class MeshAssetImportDialog final : public QDialog {
public:
	explicit MeshAssetImportDialog(raco::core::MeshScenegraph& sceneGraph, QWidget* parent = nullptr);

	QGridLayout* layout_;
	QTreeWidget* widget_;
	QPushButton* selectAllButton_;
	QPushButton* deselectAllButton_;
	QPushButton* useReferencedResourcesButton_;
	QHBoxLayout* massSelectButtonLayout_;
	QDialogButtonBox* dialogButtonBox_;

private:
	void iterateThroughChildren(QTreeWidgetItem* item, const std::function<void(QTreeWidgetItem*)> &func);
	void applyChangesToScenegraph();

	raco::core::MeshScenegraph& sceneGraph_;
	std::vector<QTreeWidgetItem*> nodeTreeList_;
	std::map<QTreeWidgetItem*, std::vector<std::optional<int>>*> widgetItemToSubmeshIndexMap_;
	std::map<QTreeWidgetItem*, int> primitiveToMeshIndexMap_;
	std::map<int, std::vector<QTreeWidgetItem*>> nodeToPrimitiveTreeList_;
	std::vector<QTreeWidgetItem*> meshTreeList_;
	std::map<int, std::unordered_set<int>> meshNodeIndexReferencedByNodes_;
};

}  // namespace raco::common_widgets
