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

#include <testing/TestEnvironmentCore.h>
#include "application/RaCoApplication.h"
#include "ramses_base/BaseEngineBackend.h"

#include "user_types/Node.h"
#include "user_types/MeshNode.h"
#include "user_types/PrefabInstance.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Animation.h"
#include "user_types/LuaInterface.h"
#include "user_types/Skin.h"

#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewSortProxyModels.h"

#include <QGuiApplication>

class ObjectTreeFilterTestFixture : public RacoBaseTest<::testing::TestWithParam<std::tuple<bool, QString, QStringList>>> {

public:
	ObjectTreeFilterTestFixture() : RacoBaseTest < ::testing::TestWithParam < std::tuple <bool, QString, QStringList >>> () {
		static const std::vector<std::string> allowedCreateableUserTypes{
			user_types::Node::typeDescription.typeName,
			user_types::MeshNode::typeDescription.typeName,
			user_types::PrefabInstance::typeDescription.typeName,
			user_types::OrthographicCamera::typeDescription.typeName,
			user_types::PerspectiveCamera::typeDescription.typeName,
			user_types::Animation::typeDescription.typeName,
			user_types::LuaInterface::typeDescription.typeName,
			user_types::Skin::typeDescription.typeName};
		
		sourceModel_ = std::make_unique<object_tree::model::ObjectTreeViewDefaultModel>(commandInterface(), dataChangeDispatcher(), externalProjects(), allowedCreateableUserTypes);
		
		// Create data for the sample source model
		const std::vector<std::string> names = {"name1", "name1", "name2", "name2", "name3", "name3", "name4", "name4", "'name5 (1)'", "'name 6(1)'"};
		const std::vector<std::string> types = {"Node", "MeshNode", "PrefabInstance", "PerspectiveCamera", "Node", "MeshNode", "PrefabInstance", "PerspectiveCamera", "OrthographicCamera", "OrthographicCamera"};
		const std::vector<std::string> ids = {"id_01", "id_02", "id_03", "id_04", "id_05", "id_06", "id_07", "id_08", "id_09", "id_10"};
		const std::vector<std::vector<std::string>> tags = { {"tag1", "tag4"}, {"tag2", "tag4"}, {"tag3", "tag2"}, {"tag4", "tag2"}, {"tag1", "tag3"}, {"tag2", "tag3"}, {"tag3", "tag1"}, {"tag4", "tag1"}, {"tag5", "tag6"}, {"tag5", "'tag7 (3)'"}};

		// Fill the sample source model with the data
		for (int i = 0; i < names.size(); ++i) {
			const auto object = commandInterface()->createObject(types.at(i), names.at(i));
			
			const auto tagsHandle = core::ValueHandle(object, {"userTags"});
			const auto idHandle = core::ValueHandle(object, {"objectID"});

			commandInterface()->setTags(tagsHandle, tags.at(i));
			commandInterface()->set(idHandle, ids.at(i));
			
			application.dataChangeDispatcher()->dispatch(recorder().release());
		}

		proxyModel_.setSourceModel(sourceModel_.get());
	}

protected:
	std::unique_ptr<object_tree::model::ObjectTreeViewDefaultModel> sourceModel_;
	object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel proxyModel_;

private:
	// ObjectTreeViewDefaultModel icons initialization uses QPixmap, which requires existing QGuiApplication.
	int argc_ = 0;
	QGuiApplication fakeGuiApp_{argc_, nullptr};

	ramses_base::HeadlessEngineBackend backend{};
	application::RaCoApplication application{backend, {{}, false, false, -1, -1, false}};

	core::ExternalProjectsStoreInterface* externalProjects() {
		return application.externalProjects();
	}

	core::CommandInterface* commandInterface() {
		return application.activeRaCoProject().commandInterface();
	}

	core::DataChangeRecorder& recorder() {
		return *application.activeRaCoProject().recorder();
	}

	components::SDataChangeDispatcher dataChangeDispatcher() {
		return application.dataChangeDispatcher();
	}
};