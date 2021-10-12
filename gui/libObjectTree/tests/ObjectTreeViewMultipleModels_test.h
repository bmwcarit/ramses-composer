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

#include "ObjectTreeViewDefaultModel_test.h"
#include "core/Queries.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "user_types/Prefab.h"
#include "user_types/Node.h"


class ObjectTreeViewMultipleModelsTest : public ObjectTreeViewDefaultModelTest {
protected:
	raco::object_tree::model::ObjectTreeViewPrefabModel prefabModel_;
        raco::core::SEditorObject prefab_;
	ObjectTreeViewMultipleModelsTest() : ObjectTreeViewDefaultModelTest(), prefabModel_(&commandInterface, dataChangeDispatcher_, nullptr) {
	}

	void SetUp() override {
		viewModel_->setProjectObjectFilterFunction([](const std::vector<raco::core::SEditorObject>& objects) -> std::vector<raco::core::SEditorObject> {
			return raco::core::Queries::filterByTypeName(objects, {raco::user_types::Node::typeDescription.typeName});
		});

		prefabModel_.setProjectObjectFilterFunction([](const std::vector<raco::core::SEditorObject>& objects) -> std::vector<raco::core::SEditorObject> {
			std::vector<raco::core::SEditorObject> result{};
			std::copy_if(objects.begin(), objects.end(), std::back_inserter(result),
				[](const raco::core::SEditorObject& object) {
					for (auto parent = object; parent; parent = parent->getParent()) {
						if (parent->getTypeDescription().typeName == raco::user_types::Prefab::typeDescription.typeName) {
							return true;
						}
					}
					return false;
				});
			return result;
		});

		prefab_ = createNodes(raco::user_types::Prefab::typeDescription.typeName, {"Prefab"}).front();
	}
};
