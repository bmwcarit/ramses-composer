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
#include "gtest/gtest.h"

#include "testing/RaCoApplicationTest.h"

#include "application/RaCoApplication.h"
#include "core/Context.h"
#include "core/Project.h"
#include "core/Undo.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "user_types/Animation.h"
#include "user_types/CubeMap.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Texture.h"
#include "user_types/UserObjectFactory.h"

#include <QGuiApplication>

class ObjectTreeViewDefaultModelTest : public RaCoApplicationTest {
public:
	std::vector<raco::core::SEditorObject> createNodes(const std::string &type, const std::vector<std::string> &nodeNames) {
		std::vector<raco::core::SEditorObject> createdNodes;

		for (const auto &name : nodeNames) {
			createdNodes.emplace_back(commandInterface().createObject(type, name));
			application.dataChangeDispatcher()->dispatch(recorder().release());
		}

		return createdNodes;
	}

	void moveScenegraphChildren(std::vector<raco::core::SEditorObject> const &objects, raco::core::SEditorObject parent, int row = -1) {
		viewModel_->moveScenegraphChildren(objects, parent, row);
		application.dataChangeDispatcher()->dispatch(recorder().release());
	}

	size_t deleteObjectsAtIndices(const QModelIndexList &index) {
		auto delObjAmount = viewModel_->deleteObjectsAtIndices({index});
		application.dataChangeDispatcher()->dispatch(recorder().release());
		return delObjAmount;
	}

	std::string modelIndexToString(raco::object_tree::model::ObjectTreeViewDefaultModel &viewModel, const QModelIndex &currentIndex, Qt::ItemDataRole role = Qt::DisplayRole) {
		return viewModel.data((currentIndex), role).toString().toStdString();
	}

	void dispatch() {
		application.dataChangeDispatcher()->dispatch(recorder().release());
	}

	std::vector<std::string> getTypes() {
		std::vector<std::string> names;
		for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
			if (viewModel_->objectFactory()->isUserCreatable(typeName, raco::ramses_base::BaseEngineBackend::maxFeatureLevel)) {
				names.emplace_back(typeName);
			}
		}
		return names;
	}

protected:
	// ObjectTreeViewDefaultModel icons initialization uses QPixmap, which requires existing QGuiApplication.
	int argc_ = 0;
	QGuiApplication fakeGuiApp_{argc_, nullptr};

	std::vector<std::string> nodeNames_;
	std::unique_ptr<raco::object_tree::model::ObjectTreeViewDefaultModel> viewModel_;

	ObjectTreeViewDefaultModelTest();

	void compareValuesInTree(const raco::core::SEditorObject &obj, const QModelIndex &objIndex, const raco::object_tree::model::ObjectTreeViewDefaultModel &viewModel);

	std::vector<raco::core::SEditorObject> createAllSceneGraphObjects();
	std::vector<raco::core::SEditorObject> createAllResources();
};
