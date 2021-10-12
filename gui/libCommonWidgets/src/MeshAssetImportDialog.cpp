/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/MeshAssetImportDialog.h"
#include "style/Icons.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"

#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

namespace raco::common_widgets {

MeshAssetImportDialog::MeshAssetImportDialog(raco::core::MeshScenegraph& sceneGraph, QWidget* parent)
	: sceneGraph_{sceneGraph},
	  nodeTreeList_{std::vector<QTreeWidgetItem*>(sceneGraph_.nodes.size())},
	  meshTreeList_{std::vector<QTreeWidgetItem*>(sceneGraph_.meshes.size())} {
	setWindowTitle("Import External Assets");

	widget_ = new QTreeWidget(this);
	widget_->setHeaderItem(new QTreeWidgetItem({"Name", "Type"}));
	widget_->setColumnWidth(0, width() / 3);
	widget_->setAlternatingRowColors(true);
	connect(widget_, &QTreeWidget::itemChanged, [this](QTreeWidgetItem* item) {
		widget_->blockSignals(true);

		if (item->checkState(0) == Qt::Unchecked) {
			iterateThroughChildren(item, [](auto* item) {
				item->setCheckState(0, Qt::Unchecked);
			});
		} else if (item->checkState(0) == Qt::Checked) {
			for (auto parent = item->parent(); parent; parent = parent->parent()) {
				parent->setCheckState(0, Qt::Checked);
			}

			iterateThroughChildren(item, [](auto* item) {
				item->setCheckState(0, Qt::Checked);
			});
		}

		widget_->blockSignals(false);
	});

	selectAllButton_ = new QPushButton("Check All", this);
	connect(selectAllButton_, &QPushButton::clicked, [this]() {
		widget_->blockSignals(true);

		for (auto* nodeItem : nodeTreeList_) {
			nodeItem->setCheckState(0, Qt::Checked);
		}
		for (auto* meshItem : meshTreeList_) {
			meshItem->setCheckState(0, Qt::Checked);
		}
		for (auto& [node, primitives] : nodeToPrimitiveTreeList_) {
			for (auto* primitive : primitives) {
				primitive->setCheckState(0, Qt::Checked);
			}
		}

		widget_->blockSignals(false);
	});

	deselectAllButton_ = new QPushButton("Uncheck All", this);
	connect(deselectAllButton_, &QPushButton::clicked, [this]() {
		for (auto topLevelIndex = 0; topLevelIndex < widget_->topLevelItemCount(); ++topLevelIndex) {
			widget_->topLevelItem(topLevelIndex)->setCheckState(0, Qt::Unchecked);
		}
	});

	useReferencedResourcesButton_ = new QPushButton("Check Resources Used by Enabled Nodes", this);
	connect(useReferencedResourcesButton_, &QPushButton::clicked, [this]() {
		std::set<QTreeWidgetItem*> checkedResources;

		for (auto nodeIndex = 0; nodeIndex < nodeTreeList_.size(); ++nodeIndex) {
			auto* nodeItem = nodeTreeList_[nodeIndex];
			auto subMeshIndeces = widgetItemToSubmeshIndexMap_[nodeItem];

			if (subMeshIndeces->size() == 1) {
				auto meshReferencedByNode = meshTreeList_[subMeshIndeces->front().value()];
				if (checkedResources.find(meshReferencedByNode) == checkedResources.end()) {
					auto nodeCheckState = nodeItem->checkState(0);
					meshReferencedByNode->setCheckState(0, nodeCheckState);

					if (nodeCheckState == Qt::CheckState::Checked) {
						checkedResources.emplace(meshReferencedByNode);
					}
				}
			} else if (subMeshIndeces->size() > 1) {
				auto primitiveItems = nodeToPrimitiveTreeList_[nodeIndex];

				for (auto primIndex = 1; primIndex < primitiveItems.size(); ++primIndex) {
					auto* primitiveItem = primitiveItems[primIndex];
					auto meshReferencedByPrimitive = meshTreeList_[(*subMeshIndeces)[primIndex - 1].value()];

					if (checkedResources.find(meshReferencedByPrimitive) == checkedResources.end()) {
						auto primitiveCheckState = primitiveItem->checkState(0);
						meshReferencedByPrimitive->setCheckState(0, primitiveCheckState);

						if (primitiveCheckState == Qt::CheckState::Checked) {
							checkedResources.emplace(meshReferencedByPrimitive);
						}
					}
				}
			}
		}
	});

	massSelectButtonLayout_ = new QHBoxLayout(nullptr);
	massSelectButtonLayout_->addWidget(selectAllButton_);
	massSelectButtonLayout_->addWidget(deselectAllButton_);
	massSelectButtonLayout_->addWidget(useReferencedResourcesButton_);
	massSelectButtonLayout_->addStretch();

	dialogButtonBox_ = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
	dialogButtonBox_->button(QDialogButtonBox::Ok)->setText("Import");
	connect(dialogButtonBox_, &QDialogButtonBox::rejected, this, &MeshAssetImportDialog::reject);
	connect(dialogButtonBox_, &QDialogButtonBox::accepted, [this]() {
		applyChangesToScenegraph();
		Q_EMIT accept();
	});

	layout_ = new QGridLayout{this};
	layout_->setRowStretch(0, 0);
	layout_->setRowStretch(1, 1);
	layout_->setRowStretch(2, 1);
	layout_->setRowStretch(3, 1);

	layout_->addWidget(new QLabel("Check/Uncheck the external assets that you would like to import into the scene.", 0));
	layout_->addWidget(widget_, 1, 0);
	layout_->addLayout(massSelectButtonLayout_, 2, 0);
	layout_->addWidget(dialogButtonBox_, 3, 0);

	for (auto i = 0; i < sceneGraph_.nodes.size(); ++i) {
		auto& node = sceneGraph_.nodes[i].value();

		auto& item = nodeTreeList_[i] = new QTreeWidgetItem({QString::fromStdString(node.name), QString::fromStdString((node.subMeshIndeces.size() == 1) ? raco::user_types::MeshNode::typeDescription.typeName : raco::user_types::Node::typeDescription.typeName)});
		widgetItemToSubmeshIndexMap_[item] = &node.subMeshIndeces;
		item->setIcon(0, node.subMeshIndeces.size() == 1 ? raco::style::Icons::pixmap(raco::style::Pixmap::typeMesh) : raco::style::Icons::pixmap(raco::style::Pixmap::typeNode));
		item->setCheckState(0, Qt::CheckState::Checked);
		if (node.parentIndex == raco::core::MeshScenegraphNode::NO_PARENT) {
			widget_->addTopLevelItem(item);
		}

		// Handle edge case of a node containing multiple primitives:
		// We currently split those into separate MeshNodes under a singular, separate Node
		if (node.subMeshIndeces.size() > 1) {
			auto primitiveParent = new QTreeWidgetItem({QString::fromStdString(fmt::format("{}_meshnodes", node.name)), QString::fromStdString(raco::user_types::Node::typeDescription.typeName)});
			primitiveParent->setIcon(0, raco::style::Icons::pixmap(raco::style::Pixmap::typeNode));
			primitiveParent->setCheckState(0, Qt::CheckState::Checked);
			item->addChild(primitiveParent);
			// first element in primitive tree list is the aforementioned singular Node (primitive parent)
			nodeToPrimitiveTreeList_[i].emplace_back(primitiveParent);

			for (auto primitiveIndex = 0; primitiveIndex < node.subMeshIndeces.size(); ++primitiveIndex) {
				auto* primitive = new QTreeWidgetItem({QString::fromStdString(fmt::format("{}_meshnode_{}", node.name, primitiveIndex)), QString::fromStdString(raco::user_types::MeshNode::typeDescription.typeName)});
				primitive->setIcon(0, raco::style::Icons::pixmap(raco::style::Pixmap::typeMesh));
				primitive->setCheckState(0, Qt::CheckState::Checked);
				nodeToPrimitiveTreeList_[i].emplace_back(primitive);
				primitiveToMeshIndexMap_[primitive] = *node.subMeshIndeces[primitiveIndex];
				primitiveParent->addChild(primitive);
			}
		}
		for (auto& index : node.subMeshIndeces) {
			meshNodeIndexReferencedByNodes_[*index].emplace(i);
		}
	}

	for (auto i = 0; i < sceneGraph_.nodes.size(); ++i) {
		auto& node = sceneGraph_.nodes[i].value();
		if (node.parentIndex != raco::core::MeshScenegraphNode::NO_PARENT) {
			auto* child = nodeTreeList_[i];
			nodeTreeList_[node.parentIndex]->addChild(child);
		}
	}

	for (auto i = 0; i < sceneGraph_.meshes.size(); ++i) {
		auto& mesh = sceneGraph_.meshes[i].value();
		auto item = meshTreeList_[i] = new QTreeWidgetItem({QString::fromStdString(mesh), QString::fromStdString(raco::user_types::Mesh::typeDescription.typeName)});
		item->setIcon(0, raco::style::Icons::pixmap(raco::style::Pixmap::typeMesh));
		item->setCheckState(0, Qt::CheckState::Checked);
		widget_->addTopLevelItem(item);
	}
}

void MeshAssetImportDialog::iterateThroughChildren(QTreeWidgetItem* item, const std::function<void(QTreeWidgetItem*)>& func) {
	for (auto childIndex = 0; childIndex < item->childCount(); ++childIndex) {
		auto* child = item->child(childIndex);
		func(child);
		iterateThroughChildren(child, func);
	}
}

void MeshAssetImportDialog::applyChangesToScenegraph() {
	for (auto i = 0; i < meshTreeList_.size(); ++i) {
		if (meshTreeList_[i]->checkState(0) == Qt::Unchecked) {
			sceneGraph_.meshes[i].reset();
			for (const auto& nodeReferencingMesh : meshNodeIndexReferencedByNodes_[i]) {
				auto& submeshIndices = sceneGraph_.nodes[nodeReferencingMesh]->subMeshIndeces;
				std::replace(submeshIndices.begin(), submeshIndices.end(), i, -1);
			}
		}
	}

	for (auto i = 0; i < nodeTreeList_.size(); ++i) {
		if (nodeTreeList_[i]->checkState(0) == Qt::Unchecked) {
			sceneGraph_.nodes[i].reset();
			nodeToPrimitiveTreeList_.erase(i);
		}
	}

	for (const auto& [nodeIndex, primitiveItems] : nodeToPrimitiveTreeList_) {
		auto primitiveParent = primitiveItems.front();
		auto& subMeshIndeces = sceneGraph_.nodes[nodeIndex]->subMeshIndeces;
		if (primitiveParent->checkState(0) == Qt::Unchecked) {
			subMeshIndeces.clear();
		} else {
			for (auto primIndex = 1; primIndex < subMeshIndeces.size() + 1; ++primIndex) {
				if (primitiveItems[primIndex]->checkState(0) == Qt::Unchecked) {
					subMeshIndeces[primIndex - 1].reset();
				}
			}
		}
	}
}

}  // namespace raco::common_widgets
