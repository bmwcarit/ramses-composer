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
#include "gtest/gtest.h"

#include "core/Context.h"
#include "core/Project.h"
#include "core/Undo.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/TestEnvironmentCore.h"
#include "user_types/CubeMap.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/PrefabInstance.h"
#include "user_types/UserObjectFactory.h"
#include "user_types/Texture.h"
#include "user_types/Animation.h"


class ObjectTreeViewDefaultModelTest : public TestEnvironmentCore {
   public:
	std::vector<raco::core::SEditorObject> createNodes(const std::string &type, const std::vector<std::string> &nodeNames) {
		std::vector<raco::core::SEditorObject> createdNodes;

		for (const auto &name : nodeNames) {
			createdNodes.emplace_back(context.createObject(type, name, std::string(type + name)));
			dataChangeDispatcher_->dispatch(recorder.release());
		}

		return createdNodes;
	}

	void moveScenegraphChildren(std::vector<raco::core::SEditorObject> const &objects, raco::core::SEditorObject parent, int row = -1) {
		viewModel_->moveScenegraphChildren(objects, parent, row);
		dataChangeDispatcher_->dispatch(recorder.release());
	}

	size_t deleteObjectsAtIndices(const QModelIndexList& index) {
		auto delObjAmount = viewModel_->deleteObjectsAtIndices({index});
		dataChangeDispatcher_->dispatch(recorder.release());
		return delObjAmount;
	}

	std::string modelIndexToString(raco::object_tree::model::ObjectTreeViewDefaultModel &viewModel, const QModelIndex &currentIndex, Qt::ItemDataRole role = Qt::DisplayRole) {
		return viewModel.data((currentIndex), role).toString().toStdString();
	}

   protected:
	raco::components::SDataChangeDispatcher dataChangeDispatcher_{std::make_shared<raco::components::DataChangeDispatcher>()};
	std::vector<std::string> nodeNames_;
	std::unique_ptr<raco::object_tree::model::ObjectTreeViewDefaultModel> viewModel_;

	ObjectTreeViewDefaultModelTest();

	void compareValuesInTree(const raco::core::SEditorObject &obj, const QModelIndex &objIndex, const raco::object_tree::model::ObjectTreeViewDefaultModel &viewModel);

	std::vector<raco::core::SEditorObject> createAllSceneGraphObjects();
	std::vector<raco::core::SEditorObject> createAllResources();
};
