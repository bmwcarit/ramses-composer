/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "user_types/Node.h"

using namespace raco::ramses_adaptor;
using raco::user_types::Node;
using namespace raco::core;

TEST(Rotation, initialSpatialProperties_areEqual) {
	raco::ramses_base::HeadlessEngineBackend backend;
	backend.setFeatureLevel(raco::ramses_base::BaseEngineBackend::maxFeatureLevel);
	auto testScene = backend.client().createScene(ramses::sceneId_t{1u});

	auto ramsesNode = testScene->createNode();
	auto dataNode = std::make_shared<Node>();

	EXPECT_EQ(getRamsesRotation(ramsesNode), getRacoRotation(dataNode));
	EXPECT_EQ(getRamsesTranslation(ramsesNode), getRacoTranslation(dataNode));
	EXPECT_EQ(getRamsesScaling(ramsesNode), getRacoScaling(dataNode));
}

TEST(Rotation, spatialProperties_areEqual_afterSync) {
	raco::ramses_base::HeadlessEngineBackend backend;
	backend.setFeatureLevel(raco::ramses_base::BaseEngineBackend::maxFeatureLevel);
	auto testScene = backend.client().createScene(ramses::sceneId_t{1u});

	auto ramsesNode = testScene->createNode();

	auto dataNode = std::make_shared<Node>();
	dataNode->scaling_->x = 10.0;
	dataNode->scaling_->y = 15.0;
	dataNode->scaling_->z = 4.0;
	dataNode->translation_->x = -10.0;
	dataNode->translation_->y = 8.0;
	dataNode->translation_->z = 3.0;
	dataNode->rotation_->x = 50.0;
	dataNode->rotation_->y = -3.0;
	dataNode->rotation_->z = 77.0;

	ramsesNode->setTranslation(getRacoTranslation(dataNode));
	ramsesNode->setScaling(getRacoScaling(dataNode));
	ramsesNode->setRotation(getRacoRotation(dataNode), ramses::ERotationType::Euler_XYZ);

	EXPECT_EQ(getRamsesRotation(ramsesNode), getRacoRotation(dataNode));
	EXPECT_EQ(getRamsesTranslation(ramsesNode), getRacoTranslation(dataNode));
	EXPECT_EQ(getRamsesScaling(ramsesNode), getRacoScaling(dataNode));
}
