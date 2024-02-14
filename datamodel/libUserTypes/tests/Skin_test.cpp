/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"

#include "user_types/Skin.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class SkinTest : public TestEnvironmentCore {};

TEST_F(SkinTest, uri_empty) {
	auto skin = create<Skin>("skin");
	ValueHandle uriHandle{skin, &Skin::uri_};

	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), core::ErrorLevel::WARNING);
}

TEST_F(SkinTest, uri_invalid) {
	auto skin = create<Skin>("skin");
	ValueHandle uriHandle{skin, &Skin::uri_};
	commandInterface.set(uriHandle, std::string("invalid"));

	EXPECT_TRUE(commandInterface.errors().hasError(uriHandle));
	EXPECT_EQ(commandInterface.errors().getError(uriHandle).level(), core::ErrorLevel::ERROR);
}

TEST_F(SkinTest, uri_valid) {
	auto skin = create<Skin>("skin");
	ValueHandle uriHandle{skin, &Skin::uri_};

	EXPECT_EQ(skin->joints_->size(), 0);
	commandInterface.set(uriHandle, (test_path() / "meshes/SimpleSkin/SimpleSkin.gltf").string());
	EXPECT_EQ(skin->joints_->size(), 2);

	EXPECT_FALSE(commandInterface.errors().hasError(uriHandle));

	EXPECT_EQ(skin->skinData()->numSkins, 1);
	EXPECT_EQ(skin->skinData()->inverseBindMatrices.size(), 2);
}

TEST_F(SkinTest, skin_index_range_errors) {
	auto skin = create<Skin>("skin");
	ValueHandle uriHandle{skin, &Skin::uri_};
	EXPECT_EQ(skin->joints_->size(), 0);
	commandInterface.set(uriHandle, (test_path() / "meshes/SimpleSkin/SimpleSkin.gltf").string());
	EXPECT_EQ(skin->joints_->size(), 2);
	ValueHandle indexHandle{skin, &Skin::skinIndex_};
	commandInterface.set(indexHandle, 1);

	EXPECT_EQ(skin->joints_->size(), 0);
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	EXPECT_EQ(commandInterface.errors().getError({skin}).level(), core::ErrorLevel::ERROR);
	EXPECT_EQ(skin->skinData(), nullptr);

	commandInterface.set(indexHandle, 0);

	EXPECT_EQ(skin->joints_->size(), 2);
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	EXPECT_EQ(commandInterface.errors().getError({skin}).level(), core::ErrorLevel::INFORMATION);
	EXPECT_NE(skin->skinData(), nullptr);
	EXPECT_EQ(skin->skinData()->numSkins, 1);
	EXPECT_EQ(skin->skinData()->inverseBindMatrices.size(), 2);
}
