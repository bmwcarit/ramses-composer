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

#include "ObjectTreeViewDefaultModel_test.h"
#include "core/Queries.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "user_types/Prefab.h"
#include "user_types/Node.h"


class ObjectTreeViewMultipleModelsTest : public ObjectTreeViewDefaultModelTest {
protected:
	raco::object_tree::model::ObjectTreeViewPrefabModel prefabModel_;
        raco::core::SEditorObject prefab_;
	ObjectTreeViewMultipleModelsTest()
		: ObjectTreeViewDefaultModelTest(),
		  prefabModel_(&commandInterface(), application.dataChangeDispatcher(), nullptr, {raco::user_types::Prefab::typeDescription.typeName}) {
	}

	void SetUp() override {
		prefab_ = createNodes(raco::user_types::Prefab::typeDescription.typeName, {"Prefab"}).front();
	}
};
