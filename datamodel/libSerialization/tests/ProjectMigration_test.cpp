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

#include "core/Link.h"

#include "ramses_adaptor/SceneBackend.h"

#include "user_types/MeshNode.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Material.h"

#include "testing/TestEnvironmentCore.h"

#include <gtest/gtest.h>

using namespace raco::core;

struct MigrationTest : public TestEnvironmentCore {
	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication application{backend};
};

TEST_F(MigrationTest, migrate_from_V1) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "V1.rca").string()), &application, pathStack);

	ASSERT_EQ(racoproject->project()->settings()->sceneId_.asInt(), 123);
	ASSERT_NE(racoproject->project()->settings()->objectID(), "b5535e97-4e60-4d72-99a9-b137b2ed52a5");	// this was the magic hardcoded ID originally used by the migration code.
}

TEST_F(MigrationTest, migrate_from_V9) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "V9.rca").string()), &application, pathStack);

	auto p = std::dynamic_pointer_cast<raco::user_types::PerspectiveCamera>(raco::core::Queries::findByName(racoproject->project()->instances(), "PerspectiveCamera"));
	ASSERT_EQ(p->viewport_->offsetX_.asInt(), 1);
	ASSERT_EQ(p->viewport_->offsetY_.asInt(), 1);
	ASSERT_EQ(p->viewport_->width_.asInt(), 1441);
	ASSERT_EQ(p->viewport_->height_.asInt(), 721);

	auto o = std::dynamic_pointer_cast<raco::user_types::OrthographicCamera>(raco::core::Queries::findByName(racoproject->project()->instances(), "OrthographicCamera"));
	ASSERT_EQ(o->viewport_->offsetX_.asInt(), 2);
	ASSERT_EQ(o->viewport_->offsetY_.asInt(), 2);
	ASSERT_EQ(o->viewport_->width_.asInt(), 1442);
	ASSERT_EQ(o->viewport_->height_.asInt(), 722);
}

TEST_F(MigrationTest, migrate_from_V10) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "V10.rca").string()), &application, pathStack);

	auto meshnode = raco::core::Queries::findByName(racoproject->project()->instances(), "MeshNode")->as<raco::user_types::MeshNode>();

	ASSERT_TRUE(meshnode != nullptr);
	auto options = meshnode->getMaterialOptionsHandle(0);
	ASSERT_TRUE(options);
	ASSERT_FALSE(options.hasProperty("depthfunction"));
	ASSERT_TRUE(options.hasProperty("depthFunction"));
	ASSERT_EQ(options.get("depthFunction").asInt(), 1);

	ASSERT_TRUE(meshnode->getMaterialPrivateHandle(0));
	ASSERT_TRUE(meshnode->getMaterialPrivateHandle(0).asBool());

	auto material = raco::core::Queries::findByName(racoproject->project()->instances(), "Material")->as<raco::user_types::Material>();

	ASSERT_TRUE(material != nullptr);
	ASSERT_TRUE(material->uniforms_->size() > 0);
	for (size_t i = 0; i < material->uniforms_->size(); i++) {
		auto engineType = material->uniforms_->get(i)->query<raco::user_types::EngineTypeAnnotation>()->type();
		bool hasLinkAnno = material->uniforms_->get(i)->query<raco::core::LinkEndAnnotation>() != nullptr;
		ASSERT_TRUE((raco::core::PropertyInterface::primitiveType(engineType) != raco::data_storage::PrimitiveType::Ref) == hasLinkAnno);
	}

	auto& uniforms = *material->uniforms_;
	ASSERT_EQ(uniforms.get("scalar")->asDouble(), 42.0);
	ASSERT_EQ(uniforms.get("count_")->asInt(), 42);
	ASSERT_EQ(*uniforms.get("vec")->asVec3f().x, 0.1);
	ASSERT_EQ(*uniforms.get("ambient")->asVec4f().w, 0.4);
	ASSERT_EQ(*uniforms.get("iv2")->asVec2i().i2_, 2);
}

TEST_F(MigrationTest, migrate_from_V12) {
	std::vector<std::string> pathStack;
	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "V12.rca").string()), &application, pathStack);

	auto pcam = raco::core::Queries::findByName(racoproject->project()->instances(), "PerspectiveCamera")->as<raco::user_types::PerspectiveCamera>();
	ASSERT_EQ(*pcam->viewport_->offsetX_, 1);
	ASSERT_EQ(*pcam->viewport_->offsetY_, 3);  // linked
	ASSERT_EQ(*pcam->viewport_->width_, 1441);
	ASSERT_EQ(*pcam->viewport_->height_, 721);

	ASSERT_EQ(*pcam->frustum_->near_, 4.5); /// linked
	ASSERT_EQ(*pcam->frustum_->far_, 1001.0);
	ASSERT_EQ(*pcam->frustum_->fov_, 36.0);
	ASSERT_EQ(*pcam->frustum_->aspect_, 3.0);

	auto ocam = raco::core::Queries::findByName(racoproject->project()->instances(), "OrthographicCamera")->as<raco::user_types::OrthographicCamera>();
	ASSERT_EQ(*ocam->viewport_->offsetX_, 2);
	ASSERT_EQ(*ocam->viewport_->offsetY_, 3); // linked
	ASSERT_EQ(*ocam->viewport_->width_, 1442);
	ASSERT_EQ(*ocam->viewport_->height_, 722);

	ASSERT_EQ(*ocam->frustum_->near_, 2.1);
	ASSERT_EQ(*ocam->frustum_->far_, 1002.0);
	ASSERT_EQ(*ocam->frustum_->left_, 4.5); // linked
	ASSERT_EQ(*ocam->frustum_->right_, 12.0);
	ASSERT_EQ(*ocam->frustum_->bottom_, -8.0);
	ASSERT_EQ(*ocam->frustum_->top_, 12.0);

	auto material = raco::core::Queries::findByName(racoproject->project()->instances(), "Material")->as<raco::user_types::Material>();
	ASSERT_EQ(*material->options_->blendOperationColor_, ramses::EBlendOperation_Max);
	ASSERT_EQ(*material->options_->blendOperationAlpha_, ramses::EBlendOperation_Add);

	ASSERT_EQ(*material->options_->blendFactorSrcColor_, ramses::EBlendFactor_One);
	ASSERT_EQ(*material->options_->blendFactorDestColor_, ramses::EBlendFactor_AlphaSaturate);
	ASSERT_EQ(*material->options_->blendFactorSrcAlpha_, ramses::EBlendFactor_Zero);
	ASSERT_EQ(*material->options_->blendFactorDestAlpha_, ramses::EBlendFactor_AlphaSaturate);

	ASSERT_EQ(*material->options_->depthwrite_, false);
	ASSERT_EQ(*material->options_->depthFunction_, ramses::EDepthFunc_Never);
	ASSERT_EQ(*material->options_->cullmode_, ramses::ECullMode_FrontAndBackFacing);

	ASSERT_EQ(*material->options_->blendColor_->x, 1.0);
	ASSERT_EQ(*material->options_->blendColor_->y, 2.0);
	ASSERT_EQ(*material->options_->blendColor_->z, 3.0);
	ASSERT_EQ(*material->options_->blendColor_->w, 4.0);

	auto meshnode = raco::core::Queries::findByName(racoproject->project()->instances(), "MeshNode")->as<raco::user_types::MeshNode>();
	auto options = dynamic_cast<const raco::user_types::BlendOptions*>(&meshnode->materials_->get(0)->asTable().get("options")->asStruct());

	ASSERT_EQ(*options->blendOperationColor_, ramses::EBlendOperation_Add);
	ASSERT_EQ(*options->blendOperationAlpha_, ramses::EBlendOperation_Max);

	ASSERT_EQ(*options->blendFactorSrcColor_, ramses::EBlendFactor_AlphaSaturate);
	ASSERT_EQ(*options->blendFactorDestColor_, ramses::EBlendFactor_One);
	ASSERT_EQ(*options->blendFactorSrcAlpha_, ramses::EBlendFactor_AlphaSaturate);
	ASSERT_EQ(*options->blendFactorDestAlpha_, ramses::EBlendFactor_Zero);

	ASSERT_EQ(*options->depthwrite_, false);
	ASSERT_EQ(*options->depthFunction_, ramses::EDepthFunc_Never);
	ASSERT_EQ(*options->cullmode_, ramses::ECullMode_FrontAndBackFacing);

	ASSERT_EQ(*options->blendColor_->x, 4.0);
	ASSERT_EQ(*options->blendColor_->y, 3.0);
	ASSERT_EQ(*options->blendColor_->z, 2.0);
	ASSERT_EQ(*options->blendColor_->w, 1.0);

	
	auto lua = raco::core::Queries::findByName(racoproject->project()->instances(), "LuaScript")->as<raco::user_types::LuaScript>();
	checkLinks(*racoproject->project(), {
		{{lua, {"luaOutputs", "int"}}, {pcam, {"viewport", "offsetY"}}},
		{{lua, {"luaOutputs", "float"}}, {pcam, {"frustum", "nearPlane"}}},
		{{lua, {"luaOutputs", "int"}}, {ocam, {"viewport", "offsetY"}}},
		{{lua, {"luaOutputs", "float"}}, {ocam, {"frustum", "leftPlane"}}}});

}