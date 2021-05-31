/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Queries.h"

#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"

#include "ramses_adaptor/SceneBackend.h"

#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"

#include "testing/TestEnvironmentCore.h"



#include <gtest/gtest.h>

struct MigrationTest : public TestEnvironmentCore {
	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication application{backend};
};

TEST_F(MigrationTest, migrate_to_V2) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "toV2.rca").string()), &application, pathStack);

	ASSERT_EQ(racoproject->project()->settings()->sceneId_.asInt(), 123);
	ASSERT_NE(racoproject->project()->settings()->objectID(), "b5535e97-4e60-4d72-99a9-b137b2ed52a5"); // this was the magic hardcoded ID originally used by the migration code.
}

TEST_F(MigrationTest, migrate_to_V10) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "toV10.rca").string()), &application, pathStack);

	auto p = std::dynamic_pointer_cast<raco::user_types::PerspectiveCamera>(raco::core::Queries::findByName(racoproject->project()->instances(), "PerspectiveCamera"));
	ASSERT_EQ(p->viewportOffsetX_.asInt(), 1);
	ASSERT_EQ(p->viewportOffsetY_.asInt(), 1);
	ASSERT_EQ(p->viewportWidth_.asInt(), 1441);
	ASSERT_EQ(p->viewportHeight_.asInt(), 721);

	auto o = std::dynamic_pointer_cast<raco::user_types::OrthographicCamera>(raco::core::Queries::findByName(racoproject->project()->instances(), "OrthographicCamera"));
	ASSERT_EQ(o->viewportOffsetX_.asInt(), 2);
	ASSERT_EQ(o->viewportOffsetY_.asInt(), 2);
	ASSERT_EQ(o->viewportWidth_.asInt(), 1442);
	ASSERT_EQ(o->viewportHeight_.asInt(), 722);
}

