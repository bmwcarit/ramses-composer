/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "object_tree_view/ObjectTreeView.h"


#include "components/RaCoPreferences.h"
#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"
#include "user_types/Mesh.h"
#include "user_types/Node.h"
#include "user_types/Material.h"

#include "object_tree_view_model/ObjectTreeNode.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "MeshData/MeshDataManager.h"
#include "utils/u8path.h"
#include "user_types/Texture.h"
#include "user_types/Node.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QDebug>

namespace raco::object_tree::view {

using namespace raco::object_tree::model;

ObjectTreeView::ObjectTreeView(const QString &viewTitle, ObjectTreeViewDefaultModel *viewModel, QSortFilterProxyModel *sortFilterProxyModel, QWidget *parent)
	: QTreeView(parent), treeModel_(viewModel), proxyModel_(sortFilterProxyModel), viewTitle_(viewTitle) {
	setAlternatingRowColors(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	setDragDropMode(QAbstractItemView::DragDrop);
	setDragEnabled(true);
	setDropIndicatorShown(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	viewport()->setAcceptDrops(true);

	if (proxyModel_) {
		proxyModel_->setSourceModel(treeModel_);
		QTreeView::setModel(proxyModel_);
	} else {
		QTreeView::setModel(treeModel_);
	}

	setTextElideMode(treeModel_->textElideMode());

	connect(this, &QTreeView::customContextMenuRequested, this, &ObjectTreeView::showContextMenu);
	connect(this, &QTreeView::expanded, this, &ObjectTreeView::expanded);
	connect(this, &QTreeView::collapsed, this, &ObjectTreeView::collapsed);

	connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const auto &selectedItemList, const auto &deselectedItemList) {
		if (auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_))) {
			Q_EMIT externalObjectSelected();
			return;
		}

		auto selectedObjects = indicesToSEditorObjects(selectedItemList.indexes());		
		for (const auto &selObj : selectedObjects) {
            selectedItemIDs_.emplace(selObj->objectID());
		}

		auto deselectedObjects = indicesToSEditorObjects(deselectedItemList.indexes());		
		for (const auto &deselObj : deselectedObjects) {
			selectedItemIDs_.erase(deselObj->objectID());
		}

		Q_EMIT newObjectTreeItemsSelected(getSelectedHandles());
	});

	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemExpansionStates);
	connect(treeModel_, &ObjectTreeViewDefaultModel::modelReset, this, &ObjectTreeView::restoreItemSelectionStates);
    connect(treeModel_, &raco::object_tree::model::ObjectTreeViewDefaultModel::editNodeOpreations, this, &ObjectTreeView::globalOpreations);

    connect(&signalProxy::GetInstance(), &signalProxy::sigRepaintAfterUndoOpreation, this, &ObjectTreeView::selectActiveObject);
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateMeshModelMatrix, this, &ObjectTreeView::updateMeshModelMatrix);

	setColumnWidth(ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, width() / 3);

	auto cutShortcut = new QShortcut(QKeySequence::Cut, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(cutShortcut, &QShortcut::activated, this, &ObjectTreeView::cut);
	auto deleteShortcut = new QShortcut({"Del"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(deleteShortcut, &QShortcut::activated, this, &ObjectTreeView::shortcutDelete);

	auto duplicateShortcut = new QShortcut({"Ctrl+D"}, this, nullptr, nullptr, Qt::WidgetShortcut);
	QObject::connect(duplicateShortcut, &QShortcut::activated, this, &ObjectTreeView::duplicateObjects);

    QObject::connect(&signalProxy::GetInstance(), &signalProxy::sigDeleteAniamtionNode, this, &ObjectTreeView::deleteAnimationHandle);
	QObject::connect(&signalProxy::GetInstance(), &signalProxy::sigReLoadNodeData, this, &ObjectTreeView::globalOpreations);
}

std::set<core::ValueHandle> ObjectTreeView::getSelectedHandles() const {
	auto selectedObjects = indicesToSEditorObjects(selectionModel()->selectedIndexes());

	return std::set<ValueHandle>(selectedObjects.begin(), selectedObjects.end());
}

void ObjectTreeView::setUniformsProperty(core::ValueHandle valueHandle, Uniform &tempUniform) {
	using PrimitiveType = core::PrimitiveType;
	tempUniform.setName(valueHandle.getPropName());
	std::string property = valueHandle.getPropName();
	switch (valueHandle.type()) {
		case PrimitiveType::String: {
			tempUniform.setName(property);
			tempUniform.setType(UniformType::String);
			tempUniform.setValue(valueHandle.asString());
			break;
		}
		case PrimitiveType::Bool: {
			tempUniform.setName(property);
			tempUniform.setType(UniformType::Bool);
			tempUniform.setValue(valueHandle.asBool());
			break;
		}
		case PrimitiveType::Int: {
			tempUniform.setName(property);
			tempUniform.setType(UniformType::Int);
			tempUniform.setValue(valueHandle.asInt());
			break;
		}
		case PrimitiveType::Double: {
			tempUniform.setName(property);
			tempUniform.setType(UniformType::Double);
			tempUniform.setValue(valueHandle.asDouble());
			break;
		}
		case PrimitiveType::Ref: {
			tempUniform.setName(property);
			tempUniform.setType(UniformType::Ref);
			if (valueHandle.asRef())
				tempUniform.setValue(valueHandle.asRef()->objectName());
			//TextureData textureData;
			//textureData.setUniformName(property);
			//setTexturePorperty(tempHandle.asRef(), materialData, textureData);

			//if (textureData.getName().empty()) {
			//	textureData.setName("empty");
			//	textureData.setBitmapRef("empty");
			//}
			//materialData.addTexture(textureData);
			break;
		}
		case PrimitiveType::Table:
		case PrimitiveType::Struct: {
			auto typeDesc = &valueHandle.constValueRef()->asStruct().getTypeDescription();
			if (typeDesc == &core::Vec2f::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec2f);
				Vec2 value;
				value.x = valueHandle[0].asDouble();
				value.y = valueHandle[1].asDouble();
				tempUniform.setValue(value);
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec3f);
				Vec3 value;
				value.x = valueHandle[0].asDouble();
				value.y = valueHandle[1].asDouble();
				value.z = valueHandle[2].asDouble();
				tempUniform.setValue(value);
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec4f);
				Vec4 value;
				value.x = valueHandle[0].asDouble();
				value.y = valueHandle[1].asDouble();
				value.z = valueHandle[2].asDouble();
				value.w = valueHandle[3].asDouble();
				tempUniform.setValue(value);
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec4i);
				Vec2int value;
				value.x = valueHandle[0].asInt();
				value.y = valueHandle[1].asInt();
				tempUniform.setValue(value);
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec3i);
				Vec3int value;
				value.x = valueHandle[0].asInt();
				value.y = valueHandle[1].asInt();
				value.z = valueHandle[2].asInt();
				tempUniform.setValue(value);
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Vec4i);
				Vec4int value;
				value.x = valueHandle[0].asInt();
				value.y = valueHandle[1].asInt();
				value.z = valueHandle[2].asInt();
				value.w = valueHandle[3].asInt();
				tempUniform.setValue(value);
			}
			break;
		}
		default: {
			break;
		}
	};
}
void ObjectTreeView::getOnehandle(QModelIndex index, NodeData *parent, raco::guiData::NodeDataManager &nodeDataManager, std::map<std::string, core::ValueHandle> &NodeNameHandleReMap) {
	
	core::ValueHandle tempHandle = indexToSEditorObject(index);
	NodeData tempNode;
	std::string str;
	str = tempHandle[0].getPropertyPath();
	tempNode.setName(str);
	str = tempHandle[0].asString();
	NodeNameHandleReMap.emplace(str, tempHandle);
	tempNode.setObjectID(str);
	if (tempHandle.get("mesh")) {
		auto materials = tempHandle.get("materials");
		auto material = materials[0];
		if (material) {
			auto uniforms = material.get("uniforms");
			if (uniforms) {
				for (int i = 0; i < uniforms.size(); i++) {
					Uniform un;
					un.setName(uniforms[i].getPropName());
					setUniformsProperty(uniforms[i], un);
					tempNode.insertUniformData(un);
				}
			}
		}
	}
	tempNode.setParent(parent);
    NodeData* data = nodeDataManager.searchNodeByID(tempNode.objectID());
    if (data) {
        tempNode.setNodeExtend(data->NodeExtendRef());
    }
	parent->childMapRef().emplace(tempNode.getName(), tempNode);

	if (model()->hasChildren(index)) {
		NodeData *pNode = &(parent->childMapRef().find(tempNode.getName())->second);
		for (int i{0}; i < model()->rowCount(index); i++) {
			QModelIndex tempIndex = model()->index(i, 0, index);
			core::ValueHandle tempHandle = indexToSEditorObject(tempIndex);
			getOnehandle(tempIndex, pNode, nodeDataManager, NodeNameHandleReMap);
		}
		NodeNameHandleReMap.emplace(tempNode.objectID(), tempHandle);
    }
}

void ObjectTreeView::getOneMeshHandle(QModelIndex index, QMatrix4x4 matrix) {
    if (!model()->hasChildren(index)) {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        computeWorldMatrix(tempHandle, matrix);

        raco::guiData::MeshData mesh;
        std::string objectID = tempHandle[0].asString();
        if (getOneMeshData(tempHandle, mesh)) {
            mesh.setModelMatrix(matrix);
            MeshDataManager::GetInstance().addMeshData(objectID, mesh);
        }
    } else {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        computeWorldMatrix(tempHandle, matrix);

        raco::guiData::MeshData mesh;
        std::string objectID = tempHandle[0].asString();
        if (getOneMeshData(tempHandle, mesh)) {
            mesh.setModelMatrix(matrix);
            MeshDataManager::GetInstance().addMeshData(objectID, mesh);
        }

        for (int i{0}; i < model()->rowCount(index); i++) {
            QModelIndex tempIndex = model()->index(i, 0, index);
            getOneMeshHandle(tempIndex, matrix);
        }
    }
}

void ObjectTreeView::getOneMeshModelMatrix(QModelIndex index, QMatrix4x4 matrix) {
    if (!model()->hasChildren(index)) {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        std::string str = tempHandle[0].getPropertyPath();
        computeWorldMatrix(tempHandle, matrix);

        raco::guiData::MeshData mesh;
        std::string objectID = tempHandle[0].asString();
        if (MeshDataManager::GetInstance().hasMeshData(objectID)) {
            MeshDataManager::GetInstance().setMeshModelMatrix(objectID, matrix);
        }
    } else {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        std::string str = tempHandle[0].getPropertyPath();
        computeWorldMatrix(tempHandle, matrix);

        std::string objectID = tempHandle[0].asString();
        if (MeshDataManager::GetInstance().hasMeshData(objectID)) {
            MeshDataManager::GetInstance().setMeshModelMatrix(objectID, matrix);
        }

        for (int i{0}; i < model()->rowCount(index); i++) {
            QModelIndex tempIndex = model()->index(i, 0, index);
            getOneMeshModelMatrix(tempIndex, matrix);
        }
    }
}

bool ObjectTreeView::getOneMeshData(ValueHandle valueHandle, raco::guiData::MeshData &meshData) {
    if (valueHandle.hasProperty("mesh")) {
        raco::core::ValueHandle tempHandle = valueHandle.get("mesh");
        if (tempHandle.type() == core::PrimitiveType::Ref) {
            raco::core::ValueHandle meshHandle = tempHandle.asRef();
            if (meshHandle != NULL) {
                // fill meshData
                raco::user_types::Mesh *mesh = dynamic_cast<raco::user_types::Mesh *>(meshHandle.rootObject().get());
				if (mesh->meshData() == NULL) {
					return false;
				}

                meshData.setNumTriangles(mesh->meshData()->numTriangles());
                meshData.setNumVertices(mesh->meshData()->numVertices());
                meshData.setIndices(mesh->meshData()->getIndices());
                for (int i{0}; i < mesh->meshData()->numAttributes(); i++) {
                    raco::guiData::Attribute attribute;
                    attribute.name = mesh->meshData()->attribName(i);
                    attribute.type = static_cast<raco::guiData::VertexAttribDataType>(mesh->meshData()->attribDataType(i));

                    auto firstPos = mesh->meshData()->attribBuffer(i);
                    auto posElementAmount = mesh->meshData()->attribElementCount(i);
                    attribute.data.resize(posElementAmount * attriElementSize(attribute.type));
                    std::memcpy(&attribute.data[0], firstPos, posElementAmount * attriElementSize(attribute.type) * sizeof(float));
                    meshData.addAttribute(attribute);
                }
                if (meshHandle.hasProperty("objectName") && meshHandle.hasProperty("uri")) {
                    std::string objectName = meshHandle.get("objectName").asString();
                    meshData.setMeshName(objectName);

                    std::string uri = "meshes/" + objectName + ".ctm";
                    meshData.setMeshUri(uri);
                }
                return true;
            }
        }
    }
    return false;
}

bool hasMaterial(raco::core::ValueHandle handle, std::string &id) {
    if (handle.hasProperty("materials")) {
        raco::core::ValueHandle tempHandle = handle.get("materials");
        if (tempHandle != NULL && tempHandle.hasProperty("material")) {
            tempHandle = tempHandle.get("material");
            id = tempHandle[0].asString();
            return true;
        }
    }
    return false;
}

bool ObjectTreeView::getOneMaterialHandle(ValueHandle &valueHandle) {
    if (valueHandle.hasProperty("materials")) {
        valueHandle = valueHandle.get("materials");
        if (valueHandle != NULL && valueHandle.hasProperty("material")) {
            valueHandle = valueHandle.get("material");
            if (valueHandle.type() == core::PrimitiveType::Table) {
                    if (valueHandle != NULL) {
                        return true;
                    }
            }
        }
    }
    return false;
}

void ObjectTreeView::getOneMaterials(QModelIndex index, std::map<std::string, core::ValueHandle> &materialHandleMap) {
    if (!model()->hasChildren(index)) {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        std::string objectID = tempHandle[0].asString();;
        if (getOneMaterialHandle(tempHandle)) {
            materialHandleMap.emplace(objectID, tempHandle);
        }
    } else {
        for (int i{0}; i < model()->rowCount(index); i++) {
            QModelIndex tempIndex = model()->index(i, 0, index);
            core::ValueHandle tempHandle = indexToSEditorObject(tempIndex);
            std::string objectID = tempHandle[0].asString();;
            if (getOneMaterialHandle(tempHandle)) {
                materialHandleMap.emplace(objectID, tempHandle);
            }
            getOneMaterials(tempIndex, materialHandleMap);
        }
    }
}

std::map<std::string, core::ValueHandle> ObjectTreeView::updateNodeTree() {
	std::map<std::string, core::ValueHandle> NodeNameHandleReMap;
	raco::guiData::NodeDataManager &nodeDataManager = raco::guiData::NodeDataManager::GetInstance();
    NodeData *parent = new NodeData;

	int row = model()->rowCount();
	for (int i{0}; i < row; ++i) {
		QModelIndex index = model()->index(i, 0);
        getOnehandle(index, parent, nodeDataManager, NodeNameHandleReMap);
	}
	if (nodeDataManager.IsFirstInit()) {
		nodeDataManager.setFirstInit(false);
	} else {
		nodeDataManager.clearNodeData();
		nodeDataManager.setRoot(*parent);
		nodeDataManager.setActiveNode(parent);
	}
	
	return NodeNameHandleReMap;
}

std::map<std::string, core::ValueHandle> ObjectTreeView::updateResource() {
	std::map<std::string, core::ValueHandle> ResHandleReMap;
	int row = model()->rowCount();
	for (int i{0}; i < row; ++i) {
		QModelIndex index = model()->index(i, 0);
		core::ValueHandle tempHandle = indexToSEditorObject(index);
		// ID
        std::string str = tempHandle[0].asString();
		ResHandleReMap.emplace(str, tempHandle);
	}
    return ResHandleReMap;
}

std::map<std::string, core::ValueHandle> ObjectTreeView::updateMaterial() {
    std::map<std::string, core::ValueHandle> materialHandleReMap;
    int row = model()->rowCount();
    for (int i{0}; i < row; ++i) {
        QModelIndex index = model()->index(i, 0);
        getOneMaterials(index, materialHandleReMap);
    }
    return materialHandleReMap;
}

std::map<std::string, core::ValueHandle> ObjectTreeView::updateTexture() {
	std::map<std::string, core::ValueHandle> textureHandleReMap;
	int row = model()->rowCount();
	for (int i{0}; i < row; ++i) {
		QModelIndex index = model()->index(i, 0);
		core::ValueHandle tempHandle = indexToSEditorObject(index);
		std::string str = tempHandle[0].asString();
		std::string path = tempHandle[0].getPropertyPath();
		if (&tempHandle.rootObject()->getTypeDescription() == &raco::user_types::Texture::typeDescription) {
			textureHandleReMap.emplace(str, tempHandle);
		}
	}
	return textureHandleReMap;
}

void ObjectTreeView::updateMeshData() {
    MeshDataManager::GetInstance().clearMesh();
    int row = model()->rowCount();
    for (int i{0}; i < row; ++i) {
        QModelIndex index = model()->index(i, 0);
        getOneMeshHandle(index);
    }
}

int ObjectTreeView::attriElementSize(VertexAttribDataType type) {
    switch (type) {
        case VertexAttribDataType::VAT_Float2:
            return 2;
        case VertexAttribDataType::VAT_Float3:
            return 3;
        case VertexAttribDataType::VAT_Float4:
            return 4;
        case VertexAttribDataType::VAT_Float:  // Falls through
            return 1;
        default:  // NOLINT(clang-diagnostic-covered-switch-default)
            return 0;
    }
}

void ObjectTreeView::convertGltfAnimation(QString fileName) {
    int row = model()->rowCount();
    raco::core::ValueHandle valueHandle;
    std::set<raco::core::ValueHandle> handles;
    for (int i{0}; i < row; ++i) {
        QModelIndex index = model()->index(i, 0);
        if (getAnimationHandle(index, valueHandle, handles)) {
            Q_EMIT raco::signal::signalProxy::GetInstance().sigUpdateGltfAnimation(handles, fileName);
        }
    }
}

bool ObjectTreeView::getAnimationHandle(QModelIndex index, core::ValueHandle valueHandle, std::set<core::ValueHandle> &handles) {
    if (!model()->hasChildren(index)) {
        core::ValueHandle tempHandle = indexToSEditorObject(index);
        if (tempHandle.rootObject().get()->getTypeDescription().typeName.compare("Animation") == 0) {
            handles.emplace(tempHandle);
        }
    } else {
        for (int i{0}; i < model()->rowCount(index); i++) {
            QModelIndex tempIndex = model()->index(i, 0, index);
            getAnimationHandle(tempIndex, valueHandle, handles);
        }
    }
    return true;
}

void ObjectTreeView::globalCopyCallback() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.empty()) {
		if (canCopyAtIndices(selectedIndices)) {
			treeModel_->copyObjectsAtIndices(selectedIndices, false);
		}
	}
}

void ObjectTreeView::globalOpreations() {
    // TBD
    if (viewTitle_.compare("Scene Graph") != 0) {
        return;
    }
    QTime dieTime = QTime::currentTime().addMSecs(5);
    while( QTime::currentTime() < dieTime ) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    std::map<std::string, core::ValueHandle> handleMap = updateNodeTree();
    Q_EMIT updateNodeHandles(viewTitle_, handleMap);
}

void ObjectTreeView::shortcutDelete() {
	auto selectedIndices = getSelectedIndices();
	if (!selectedIndices.empty()) {
		auto delObjAmount = treeModel_->deleteObjectsAtIndices(selectedIndices);

		if (delObjAmount > 0) {
			selectionModel()->Q_EMIT selectionChanged({}, {});
		}
	}
}

void ObjectTreeView::selectObject(const QString &objectID) {
	if (objectID.isEmpty()) {
		resetSelection();
		return;
	}

	auto objectIndex = indexFromTreeNodeID(objectID.toStdString());
	if (objectIndex.isValid()) {
		resetSelection();
		selectionModel()->select(objectIndex, SELECTION_MODE);
		scrollTo(objectIndex);
	}
}

void ObjectTreeView::expandAllParentsOfObject(const QString &objectID) {
	auto objectIndex = indexFromTreeNodeID(objectID.toStdString());
	if (objectIndex.isValid()) {
		expandAllParentsOfObject(objectIndex);
	}
}

void ObjectTreeView::getMaterialResHandles() {
    if (viewTitle_.compare("Scene Graph") != 0) {
        return;
    }
    std::map<std::string, core::ValueHandle> handleMap = updateMaterial();
    Q_EMIT setMaterialResHandles(handleMap);
}

void ObjectTreeView::getTextureResHandles() {
	if (viewTitle_.compare("Resources") != 0) {
		return;
	}
	std::map<std::string, core::ValueHandle> handleMap = updateTexture();
	Q_EMIT setTextureResHandles(handleMap);
}

void ObjectTreeView::fillMeshData() {
    if (viewTitle_.compare("Scene Graph") != 0) {
        return;
    }
    updateMeshData();
}

void ObjectTreeView::selectActiveObject() {
    if (auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_))) {
        Q_EMIT externalObjectSelected();
        return;
    }

    std::string objId;
    if (raco::guiData::NodeDataManager::GetInstance().getActiveNode()) {
        objId = raco::guiData::NodeDataManager::GetInstance().getActiveNode()->objectID();
    } else {
        objId = raco::guiData::NodeDataManager::GetInstance().root().objectID();
    }

    if (objId.empty()) {
        return;
    }
    auto objectIndex = indexFromTreeNodeID(objId);
    resetSelection();
    selectionModel()->blockSignals(true);
    selectionModel()->select(objectIndex, SELECTION_MODE);
    selectionModel()->blockSignals(false);

    scrollTo(objectIndex);
    selectedItemIDs_.emplace(objId);

    std::set<ValueHandle> handles = getSelectedHandles();
    if (!handles.empty()) {
        auto handle = *handles.begin();
        Q_EMIT signalProxy::GetInstance().sigRepaintPropertyBrowserAfterUndo(handle);
    }
}

void ObjectTreeView::updateMeshModelMatrix() {
    if (viewTitle_.compare(QString("Scene Graph")) != 0) {
        return;
    }

    int row = model()->rowCount();
    for (int i{0}; i < row; ++i) {
        QModelIndex index = model()->index(i, 0);
        getOneMeshModelMatrix(index);
    }
}

void ObjectTreeView::deleteAnimationHandle(std::set<std::string> ids) {
    for (const auto &id : ids) {
        auto index = indexFromTreeNodeID(id);
        auto delObjAmount = treeModel_->deleteObjectsAtIndices(QModelIndexList() << index);

        if (delObjAmount > 0) {
            selectionModel()->Q_EMIT selectionChanged({}, {});
        }
    }
}

void ObjectTreeView::expanded(const QModelIndex &index) {
	expandedItemIDs_.insert(indexToTreeNodeID(index));

	if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
		expandRecursively(index);
	}
}

void ObjectTreeView::collapsed(const QModelIndex &index) {
	expandedItemIDs_.erase(indexToTreeNodeID(index));

	if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
		collapseRecusively(index);
	}
}

void ObjectTreeView::collapseRecusively(const QModelIndex& index) {
	collapse(index);
	
	for (int i = 0; i < index.model()->rowCount(index); ++i) {
		collapseRecusively(index.model()->index(i, 0, index));
	}
}

void ObjectTreeView::cut() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty()) {
		treeModel_->cutObjectsAtIndices(selectedIndices, false);
	}
}

void raco::object_tree::view::ObjectTreeView::duplicateObjects() {
	auto selectedIndices = getSelectedIndices(true);
	if (!selectedIndices.isEmpty() && treeModel_->canDuplicateAtIndices(selectedIndices)) {
		treeModel_->duplicateObjectsAtIndices(selectedIndices);
	}
}

void ObjectTreeView::globalPasteCallback(const QModelIndex &index, bool asExtRef) {
	if (canPasteIntoIndex(index, asExtRef)) {
		treeModel_->pasteObjectAtIndex(index, asExtRef);
	} else if (canPasteIntoIndex({}, asExtRef)) {
		treeModel_->pasteObjectAtIndex({}, asExtRef);
	}
}

QString ObjectTreeView::getViewTitle() const {
	return viewTitle_;
}

void ObjectTreeView::requestNewNode(EditorObject::TypeDescriptor nodeType, const std::string &nodeName, const QModelIndex &parent) {
	Q_EMIT dockSelectionFocusRequested(this);

	selectedItemIDs_.clear();
	auto createdObject = treeModel_->createNewObject(nodeType, nodeName, parent);
	selectedItemIDs_.insert(createdObject->objectID());
}

void ObjectTreeView::showContextMenu(const QPoint &p) {
	auto *treeViewMenu = createCustomContextMenu(p);
	treeViewMenu->exec(viewport()->mapToGlobal(p));
}

bool ObjectTreeView::canCopyAtIndices(const QModelIndexList &parentIndices) {
	return treeModel_->canCopyAtIndices(parentIndices);
}

bool ObjectTreeView::canPasteIntoIndex(const QModelIndex &index, bool asExtref) {
	if (!RaCoClipboard::hasEditorObject()) {
		return false;
	} else {
		auto [pasteObjects, sourceProjectTopLevelObjectIds] = treeModel_->getObjectsAndRootIdsFromClipboardString(RaCoClipboard::get());
		return treeModel_->canPasteIntoIndex(index, pasteObjects, sourceProjectTopLevelObjectIds, asExtref);
	}
}

bool ObjectTreeView::canProgrammaticallyGoToObject() {
	return treeModel_->canProgramaticallyGotoObject();
}

QSortFilterProxyModel* ObjectTreeView::proxyModel() const {
	return proxyModel_;
}

void ObjectTreeView::resetSelection() {
	selectionModel()->reset();
	selectedItemIDs_.clear();
	viewport()->update();
}

// TODO:
void ObjectTreeView::createBMWMaterial(const std::vector<MaterialData> &materialArr, const QModelIndex &parent) {
	EditorObject::TypeDescriptor nodeType;
	nodeType.isResource = true;
	nodeType.typeName = "Material";

	selectedItemIDs_.clear();
	auto createdObject = treeModel_->createNewObject(nodeType, "testMaterial", parent);
	selectedItemIDs_.insert(createdObject->objectID());

	for (EditorObject::ChildIterator it = createdObject->begin(); it != createdObject->end(); ++it) {
		qDebug() << QString::fromStdString((*it)->objectName());
	}

	if (createdObject->hasProperty("objectName")) {
		auto baseHandle = createdObject->get("objectName");
		qDebug() << QString::fromStdString(baseHandle->asString());
		baseHandle->set<std::string>("test1");
	}
	if(createdObject->hasProperty("uriVertex")) {
			auto baseHandle = createdObject->get("uriVertex");
			qDebug() << QString::fromStdString(baseHandle->asString());
			baseHandle->set<std::string>("shaders/simple_texture.vert");
	}
	if (createdObject->hasProperty("uriFragment")) {
		auto baseHandle = createdObject->get("uriFragment");
		qDebug() << QString::fromStdString(baseHandle->asString());
		baseHandle->set<std::string>("uuuuu");
	}

	if (createdObject->hasProperty("uniforms")) {
		auto unValueBase = createdObject->get("uniforms");
		unValueBase->set<SEditorObject>(createdObject);

		auto un1 = treeModel_->createNewObject(nodeType, "u_move", parent);	 // 继续创建子结点  nodeType=u_move
		selectedItemIDs_.insert(un1->objectID());
		if (un1->hasProperty("objectName")) {
			auto baseHandle = un1->get("objectName");
			qDebug() << QString::fromStdString(baseHandle->asString());
			baseHandle->set<std::string>("u_move");
		}
	}

	Q_EMIT dockSelectionFocusRequested(this);
}

void ObjectTreeView::importBMWAssets(NodeData *root, const std::vector<MaterialData> &materialArr) {
	auto insertionTargetIndex = getSelectedInsertionTargetIndex();
	createBMWMaterial(materialArr, insertionTargetIndex);
}

QMenu* ObjectTreeView::createCustomContextMenu(const QPoint &p) {
	auto treeViewMenu = new QMenu(this);

	auto selectedItemIndices = getSelectedIndices(true);
	auto insertionTargetIndex = getSelectedInsertionTargetIndex();
	
	bool canDeleteSelectedIndices = treeModel_->canDeleteAtIndices(selectedItemIndices);
	bool canCopySelectedIndices = treeModel_->canCopyAtIndices(selectedItemIndices);

	auto externalProjectModel = (dynamic_cast<ObjectTreeViewExternalProjectModel *>(treeModel_));
	auto prefabModel = (dynamic_cast<ObjectTreeViewPrefabModel *>(treeModel_));
	auto resourceModel = (dynamic_cast<ObjectTreeViewResourceModel *>(treeModel_));
	auto allTypes = treeModel_->objectFactory()->getTypes();
	auto allowedCreatableUserTypes = treeModel_->typesAllowedIntoIndex(insertionTargetIndex);
	auto canInsertMeshAsset = false;

	for (auto type : allowedCreatableUserTypes) {
		if (allTypes.count(type) > 0 && treeModel_->objectFactory()->isUserCreatable(type)) {
			auto typeDescriptor = allTypes[type];
			auto actionCreate = treeViewMenu->addAction(QString::fromStdString("Create " + type), [this, typeDescriptor, insertionTargetIndex]() {
				requestNewNode(typeDescriptor.description, "", insertionTargetIndex);
			});
		}
		if (type == raco::user_types::Node::typeDescription.typeName) {
			canInsertMeshAsset = true;
		}
	}

	if (canInsertMeshAsset) {
		treeViewMenu->addSeparator();

		auto actionImport = treeViewMenu->addAction("Import glTF Assets...", [this, insertionTargetIndex]() {
			auto sceneFolder = raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, treeModel_->project()->currentFolder());
			auto file = QFileDialog::getOpenFileName(this, "Load Asset File", QString::fromStdString(sceneFolder.string()), "glTF files (*.gltf *.glb)");
			if (!file.isEmpty()) {
                bool keyAnimation;
                treeModel_->importMeshScenegraph(file, insertionTargetIndex, keyAnimation);
                if (keyAnimation) {
					QString fileName = file.section("/",-1);
                    convertGltfAnimation(fileName);
                    updateMeshData();
                }
			}
		});
		actionImport->setEnabled(canInsertMeshAsset);
	}

	if (!externalProjectModel || !allowedCreatableUserTypes.empty()) {
		treeViewMenu->addSeparator();
	}

	auto actionDelete = treeViewMenu->addAction(
		"Delete", [this, selectedItemIndices]() {
			treeModel_->deleteObjectsAtIndices(selectedItemIndices);
			selectionModel()->Q_EMIT selectionChanged({}, {});
		},
		QKeySequence::Delete);
	actionDelete->setEnabled(canDeleteSelectedIndices);

	auto actionCopy = treeViewMenu->addAction(
		"Copy", [this, selectedItemIndices]() { 
		treeModel_->copyObjectsAtIndices(selectedItemIndices, false); 
	}, QKeySequence::Copy);
	actionCopy->setEnabled(canCopySelectedIndices);

	auto [pasteObjects, sourceProjectTopLevelObjectIds] = treeModel_->getObjectsAndRootIdsFromClipboardString(RaCoClipboard::get());
	QAction* actionPaste;
	if (treeModel_->canPasteIntoIndex(insertionTargetIndex, pasteObjects, sourceProjectTopLevelObjectIds)) {
		actionPaste = treeViewMenu->addAction(
			"Paste Here", [this, insertionTargetIndex]() { treeModel_->pasteObjectAtIndex(insertionTargetIndex); }, QKeySequence::Paste);
	} else if (treeModel_->canPasteIntoIndex({}, pasteObjects, sourceProjectTopLevelObjectIds)) {
		actionPaste = treeViewMenu->addAction(
			"Paste Into Project", [this]() { treeModel_->pasteObjectAtIndex(QModelIndex()); }, QKeySequence::Paste);
	} else {
		actionPaste = treeViewMenu->addAction("Paste", [](){}, QKeySequence::Paste);
		actionPaste->setEnabled(false);
	}

	auto actionDuplicate = treeViewMenu->addAction(
		"Duplicate", [this, selectedItemIndices] {
			treeModel_->duplicateObjectsAtIndices(selectedItemIndices);
		},
		QKeySequence("Ctrl+D"));
	actionDuplicate->setEnabled(treeModel_->canDuplicateAtIndices(selectedItemIndices));

	auto actionCut = treeViewMenu->addAction(
		"Cut", [this, selectedItemIndices]() {
		treeModel_->cutObjectsAtIndices(selectedItemIndices, false); 
	}, QKeySequence::Cut);
	actionCut->setEnabled(canDeleteSelectedIndices && canCopySelectedIndices);

	treeViewMenu->addSeparator();

	auto actionCopyDeep = treeViewMenu->addAction("Copy (Deep)", [this, selectedItemIndices]() { 
		treeModel_->copyObjectsAtIndices(selectedItemIndices, true); 
	});
	actionCopyDeep->setEnabled(canCopySelectedIndices);

	auto actionCutDeep = treeViewMenu->addAction("Cut (Deep)", [this, selectedItemIndices]() { 		
		treeModel_->cutObjectsAtIndices(selectedItemIndices, true); 
	});
	actionCutDeep->setEnabled(canDeleteSelectedIndices && canCopySelectedIndices);

	if (!externalProjectModel) {
		auto actionDeleteUnrefResources = treeViewMenu->addAction("Delete Unused Resources", [this] { treeModel_->deleteUnusedResources(); });
		actionDeleteUnrefResources->setEnabled(treeModel_->canDeleteUnusedResources());

		treeViewMenu->addSeparator();
		auto extrefPasteAction = treeViewMenu->addAction(
			"Paste As External Reference", [this]() {
				std::string error;
				if (!treeModel_->pasteObjectAtIndex({}, true, &error)) {
					QMessageBox::warning(this, "Paste As External Reference",
						fmt::format("Update of pasted external references failed!\n\n{}", error).c_str());
				}
			});

		// Pasting extrefs will ignore the current selection and always paste on top level.
		extrefPasteAction->setEnabled(treeModel_->canPasteIntoIndex({}, pasteObjects, sourceProjectTopLevelObjectIds, true));
	}

	if (externalProjectModel) {
		treeViewMenu->addSeparator();
		treeViewMenu->addAction("Add Project...", [this, externalProjectModel]() {
			auto projectFile = QFileDialog::getOpenFileName(this, tr("Import Project"), raco::components::RaCoPreferences::instance().userProjectsDirectory, tr("Ramses Composer Assembly (*.rca)"));
			if (projectFile.isEmpty()) {
				return;
			}
			if (projectFile.toStdString() == treeModel_->project()->currentPath()) {
				auto errorMessage = QString("Can't import external project with the same path as the currently open project %1.").arg(QString::fromStdString(treeModel_->project()->currentPath()));
				QMessageBox::critical(this, "Import Error", errorMessage);
				LOG_ERROR(log_system::OBJECT_TREE_VIEW, errorMessage.toStdString());
				return;
			}
			externalProjectModel->addProject(projectFile);
		});

		auto actionCloseImportedProject = treeViewMenu->addAction("Remove Project", [this, selectedItemIndices, externalProjectModel]() { externalProjectModel->removeProjectsAtIndices(selectedItemIndices); });
		actionCloseImportedProject->setEnabled(externalProjectModel->canRemoveProjectsAtIndices(selectedItemIndices));
	}

	return treeViewMenu;
}

void ObjectTreeView::dragMoveEvent(QDragMoveEvent *event) {
	setDropIndicatorShown(true);
	QTreeView::dragMoveEvent(event);

	// clear up QT drop indicator position confusion and don't allow below-item indicator for expanded items
	// because dropping an item above expanded items with the below-item indicator drops it to the wrong position
	auto indexBelowMousePos = indexAt(event->pos());
	if (isExpanded(indexBelowMousePos) && dropIndicatorPosition() == BelowItem) {
		event->setDropAction(Qt::DropAction::IgnoreAction);
		event->accept();
		setDropIndicatorShown(false);
	}
}

void ObjectTreeView::mousePressEvent(QMouseEvent *event) {
	QTreeView::mousePressEvent(event);
	if (!indexAt(event->pos()).isValid()) {
		resetSelection();
		Q_EMIT newObjectTreeItemsSelected({});
	}
}

std::vector<core::SEditorObject> ObjectTreeView::indicesToSEditorObjects(const QModelIndexList &indices) const {
	QModelIndexList itemIndices;
	if (proxyModel_) {
		for (const auto &index : indices) {
			itemIndices.append(proxyModel_->mapToSource(index));
		}
	} else {
		itemIndices = indices;
	}
	return treeModel_->indicesToSEditorObjects(itemIndices);
}

core::SEditorObject ObjectTreeView::indexToSEditorObject(const QModelIndex &index) const {
	auto itemIndex = index;
	if (proxyModel_) {
		itemIndex = proxyModel_->mapToSource(index);
	}
	return treeModel_->indexToSEditorObject(itemIndex);
}

std::string ObjectTreeView::indexToTreeNodeID(const QModelIndex &index) const {
	auto itemIndex = index;
	if (proxyModel_) {
		itemIndex = proxyModel_->mapToSource(index);
	}
	return treeModel_->indexToTreeNode(itemIndex)->getID();
}

QModelIndex ObjectTreeView::indexFromTreeNodeID(const std::string &id) const {
	auto index = treeModel_->indexFromTreeNodeID(id);
	if (proxyModel_) {
		index = proxyModel_->mapFromSource(index);
	}

	return index;
}

QModelIndexList ObjectTreeView::getSelectedIndices(bool sorted) const {
	auto selectedIndices = selectionModel()->selectedRows();

	if (proxyModel_) {
		for (auto& index : selectedIndices) {
			index = proxyModel_->mapToSource(index);
		}
	}

	// For operation like copy, cut and move, the order of selection matters
	if (sorted) {
		auto sortedList = selectedIndices.toVector();
		std::sort(sortedList.begin(), sortedList.end(), ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition);
		return QModelIndexList(sortedList.begin(), sortedList.end());
	}

	return selectedIndices;
}

QModelIndex ObjectTreeView::getSelectedInsertionTargetIndex() const {
	auto selectedIndices = getSelectedIndices();

	// For single selection, just insert unter the selection.
	if (selectedIndices.count() == 0) {
		return {};
	} else if (selectedIndices.count() == 1) {
		return selectedIndices.front();
	}

	// For multiselection, look for the index on the highest hierachy level which is topmost in its hierachy level.
	// Partially sort selectedIndices so that the first index of the list is the highest level topmost index of the tree.
	std::nth_element(selectedIndices.begin(), selectedIndices.begin(), selectedIndices.end(), ObjectTreeViewDefaultModel::isIndexAboveInHierachyOrPosition);
	
	// Now take this highest level topmost index from the list and insert into its parent, if it exists.
	auto canidate = selectedIndices.front();
	if (canidate.isValid()) {
		return canidate.parent();		
	} else {
		return canidate;
	}
}

void ObjectTreeView::restoreItemExpansionStates() {
	for (const auto &expandedObjectID : expandedItemIDs_) {
		auto expandedObjectIndex = indexFromTreeNodeID(expandedObjectID);
		if (expandedObjectIndex.isValid()) {
			blockSignals(true);
			expand(expandedObjectIndex);
			blockSignals(false);
		}
	}
}

void ObjectTreeView::restoreItemSelectionStates() {
	selectionModel()->reset();
	std::vector<QModelIndex> selectedObjects;

	auto selectionIt = selectedItemIDs_.begin();
	while (selectionIt != selectedItemIDs_.end()) {
		const auto &selectionID = *selectionIt;
		auto selectedObjectIndex = indexFromTreeNodeID(selectionID);
		if (selectedObjectIndex.isValid()) {
			selectionModel()->select(selectedObjectIndex, SELECTION_MODE);
			selectedObjects.emplace_back(selectedObjectIndex);
			expandAllParentsOfObject(selectedObjectIndex);
			++selectionIt;
		} else {
			selectionIt = selectedItemIDs_.erase(selectionIt);
		}
	}

	if (!selectedObjects.empty()) {
		scrollTo(selectedObjects.front());
	}
}

void ObjectTreeView::expandAllParentsOfObject(const QModelIndex &index) {
	for (auto parentIndex = index.parent(); parentIndex.isValid(); parentIndex = parentIndex.parent()) {
		if (!isExpanded(parentIndex)) {
			expand(parentIndex);
		}
    }
}

void ObjectTreeView::computeWorldMatrix(ValueHandle handle, QMatrix4x4 &chainMatrix) {
    QMatrix4x4 matrix =
        translation(handle)/*.transposed()*/ *
        rotationEuler(handle)/*.transposed()*/ *
        scaling(handle);
    chainMatrix *= matrix;
}

bool ObjectTreeView::getBasicProperty(raco::core::ValueHandle valueHandle, QString property, QVector3D &vector) {
    if (valueHandle) {
        double x{0.0}, y{0.0}, z{0.0};
        if (valueHandle.hasProperty(property.toStdString())) {
            raco::core::ValueHandle tempHandle = valueHandle.get(property.toStdString());
            x = tempHandle.get("x").asDouble();
            y = tempHandle.get("y").asDouble();
            z = tempHandle.get("z").asDouble();
            vector.setX(x);
            vector.setY(y);
            vector.setZ(z);
            return true;
        }
    }
    return false;
}

float ObjectTreeView::Deg2Rad(float val) {
    const float  PI_f = 3.1415926535897932384626433832795028841971693993751058209749f;

    return val * (PI_f / 180.f);
}

QMatrix4x4 ObjectTreeView::translation(ValueHandle handle) {
    QVector3D vector;
    if (getBasicProperty(handle, "translation", vector)) {
        double x{0.0},y{0.0},z{0.0};
        x = vector.x();
        y = vector.y();
        z = vector.z();
        return QMatrix4x4(
            1.0f, 0.0f, 0.0f, x,
            0.0f, 1.0f, 0.0f, y,
            0.0f, 0.0f, 1.0f, z,
            0.0f, 0.0f, 0.0f, 1.0f);
    }
    return QMatrix4x4();
}

QMatrix4x4 ObjectTreeView::rotationEuler(ValueHandle handle) {
    QVector3D vector;
    if (getBasicProperty(handle, "rotation", vector)) {

        double x{0.0},y{0.0},z{0.0};
        x = vector.x();
        y = vector.y();
        z = vector.z();
        // Get the rotation angles in radians
        const float rotX = Deg2Rad(x);
        const float rotY = Deg2Rad(y);
        const float rotZ = Deg2Rad(z);

        // Save some sin and cos values for reuse in the computations
        const float sx = std::sin(rotX);
        const float cx = std::cos(rotX);
        const float sy = std::sin(rotY);
        const float cy = std::cos(rotY);
        const float sz = std::sin(rotZ);
        const float cz = std::cos(rotZ);

        QMatrix4x4 xRotation(1.0f, 0.0f, 0.0f, 0.0f,
                             0.0f, cx, -sx, 0.0f,
                             0.0f, sx, cx, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
        QMatrix4x4 yRotation(cy, 0.0f, sy, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f,
                             -sy, 0.0f, cy, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);
        QMatrix4x4 zRotation(cz, -sz, 0.0f, 0.0f,
                             sz, cz, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);

//        return QMatrix4x4(
//            cz * cy                 , cz * sy * sx + sz * cx    , sz * sx - cz * sy * cx, 0.0f,
//            -sz * cy                , cz * cx - sz * sy * sx    , sz * sy * cx + cz * sx, 0.0f,
//            sy                      , -cy * sx                  , cy * cx               , 0.0f,
//            0.0f                    , 0.0f                      , 0.0f                  , 1.0f);
        return (zRotation * yRotation * xRotation);
    }
    return QMatrix4x4();
}

QMatrix4x4 ObjectTreeView::scaling(ValueHandle handle) {
    QVector3D vector;
    if (getBasicProperty(handle, "scale", vector)) {
        double x{0.0},y{0.0},z{0.0};
        x = vector.x();
        y = vector.y();
        z = vector.z();
        return QMatrix4x4(
            x,          0.0f,       0.0f,       0.0f,
            0.0f,       y,          0.0f,       0.0f,
            0.0f,       0.0f,       z,          0.0f,
            0.0f,       0.0f,       0.0f,       1.0f);
    }
    return QMatrix4x4();
}

}  // namespace raco::object_tree::view
