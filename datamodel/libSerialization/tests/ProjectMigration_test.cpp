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

#include "components/RamsesProjectMigration.h"

#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"

#include "core/Link.h"

#include "ramses_adaptor/SceneBackend.h"
#include "serialization/SerializationKeys.h"

#include "user_types/MeshNode.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Material.h"
#include "user_types/Texture.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderPass.h"

#include "testing/TestEnvironmentCore.h"

#include <gtest/gtest.h>

constexpr bool GENERATE_DIFF{false};

using namespace raco::core;

struct MigrationTest : public TestEnvironmentCore {
	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication application{backend};

	std::unique_ptr<raco::application::RaCoProject> loadAndCheckJson(QString filename, int* outFileVersion = nullptr) {
		QFile file{filename};
		EXPECT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
		auto document{QJsonDocument::fromJson(file.readAll())};
		file.close();
		auto fileVersion{raco::serialization::deserializeFileVersion(document)};
		EXPECT_TRUE(fileVersion <= raco::components::RAMSES_PROJECT_FILE_VERSION);
		if (outFileVersion) {
			*outFileVersion = fileVersion;
		}
		auto projectInfo = raco::serialization::deserializeProjectVersionInfo(document);
		std::unordered_map<std::string, std::string> migrationObjWarnings;
		auto migratedDoc{raco::components::migrateProject(document, migrationObjWarnings)};
		std::string migratedJson{migratedDoc.toJson().toStdString()};

		std::vector<std::string> pathStack;
		auto racoproject = raco::application::RaCoProject::loadFromJson(migratedDoc, filename, &application, pathStack);

		std::unordered_map<std::string, std::vector<int>> currentVersions = {
			{raco::serialization::keys::FILE_VERSION, {fileVersion}},
			{raco::serialization::keys::RAMSES_VERSION, {projectInfo.ramsesVersion.major, projectInfo.ramsesVersion.minor, projectInfo.ramsesVersion.patch}},
			{raco::serialization::keys::RAMSES_LOGIC_ENGINE_VERSION, {projectInfo.ramsesLogicEngineVersion.major, projectInfo.ramsesLogicEngineVersion.minor, projectInfo.ramsesLogicEngineVersion.patch}},
			{raco::serialization::keys::RAMSES_COMPOSER_VERSION, {projectInfo.raCoVersion.major, projectInfo.raCoVersion.minor, projectInfo.raCoVersion.patch}}};

		auto serialized = racoproject->serializeProject(currentVersions);
		auto serializedJson = serialized.toJson().toStdString();

		if (GENERATE_DIFF) {
			// This show a diff but is _very_ slow when run under the visual studio
			EXPECT_EQ(migratedJson, serializedJson);
		} else {
			// alternatively use this to show failure but this doesn't show the diff
			EXPECT_TRUE(migratedJson == serializedJson);
		}

		return racoproject;
	}
};

TEST_F(MigrationTest, migrate_from_V1) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V1.rca").string()));

	ASSERT_EQ(racoproject->project()->settings()->sceneId_.asInt(), 123);
	ASSERT_NE(racoproject->project()->settings()->objectID(), "b5535e97-4e60-4d72-99a9-b137b2ed52a5");	// this was the magic hardcoded ID originally used by the migration code.
}

TEST_F(MigrationTest, migrate_from_V9) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V9.rca").string()));

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
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V10.rca").string()));

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
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V12.rca").string()));

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

	auto meshnode_no_mesh = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_no_mesh")->as<raco::user_types::MeshNode>();
	ASSERT_EQ(meshnode_no_mesh->materials_->size(), 0);
	
	auto meshnode_mesh_no_mat = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_mesh_no_mat")->as<raco::user_types::MeshNode>();
	ASSERT_EQ(meshnode_mesh_no_mat->materials_->size(), 1);

	auto lua = raco::core::Queries::findByName(racoproject->project()->instances(), "LuaScript")->as<raco::user_types::LuaScript>();
	checkLinks(*racoproject->project(), {
		{{lua, {"luaOutputs", "int"}}, {pcam, {"viewport", "offsetY"}}},
		{{lua, {"luaOutputs", "float"}}, {pcam, {"frustum", "nearPlane"}}},
		{{lua, {"luaOutputs", "int"}}, {ocam, {"viewport", "offsetY"}}},
		{{lua, {"luaOutputs", "float"}}, {ocam, {"frustum", "leftPlane"}}}});

}

TEST_F(MigrationTest, migrate_from_V13) {
	std::vector<std::string> pathStack;

	auto racoproject = raco::application::RaCoProject::loadFromFile(QString::fromStdString((cwd_path() / "migrationTestData" / "V13.rca").string()), &application, pathStack);

	auto textureNotFlipped = raco::core::Queries::findByName(racoproject->project()->instances(), "DuckTextureNotFlipped")->as<raco::user_types::Texture>();
	ASSERT_FALSE(*textureNotFlipped->flipTexture_);

	auto textureFlipped = raco::core::Queries::findByName(racoproject->project()->instances(), "DuckTextureFlipped")->as<raco::user_types::Texture>();
	ASSERT_TRUE(*textureFlipped->flipTexture_);
}

TEST_F(MigrationTest, migrate_from_V14) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V14.rca").string()));

	auto camera = raco::core::Queries::findByName(racoproject->project()->instances(), "PerspectiveCamera")->as<raco::user_types::PerspectiveCamera>();
	auto renderpass = raco::core::Queries::findByName(racoproject->project()->instances(), "MainRenderPass")->as<raco::user_types::RenderPass>();
	ASSERT_EQ(*renderpass->camera_, camera);

	auto texture = raco::core::Queries::findByName(racoproject->project()->instances(), "Texture")->as<raco::user_types::Texture>();
	auto mat_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_no_tex")->as<raco::user_types::Material>();
	auto mat_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_with_tex")->as<raco::user_types::Material>();

	ASSERT_EQ(mat_no_tex->uniforms_->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(mat_with_tex->uniforms_->get("u_Tex")->asRef(), texture);

	auto buffer = create<raco::user_types::RenderBuffer>("buffer");
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(buffer));

	auto meshnode_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_no_tex")->as<raco::user_types::MeshNode>();
	auto meshnode_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_with_tex")->as<raco::user_types::MeshNode>();

	ASSERT_EQ(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->asRef(), texture);

	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
}

TEST_F(MigrationTest, migrate_from_V14b) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V14b.rca").string()));

	auto camera = raco::core::Queries::findByName(racoproject->project()->instances(), "OrthographicCamera")->as<raco::user_types::OrthographicCamera>();
	auto renderpass = raco::core::Queries::findByName(racoproject->project()->instances(), "MainRenderPass")->as<raco::user_types::RenderPass>();
	ASSERT_EQ(*renderpass->camera_, camera);

	auto texture = raco::core::Queries::findByName(racoproject->project()->instances(), "Texture")->as<raco::user_types::Texture>();
	auto mat_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_no_tex")->as<raco::user_types::Material>();
	auto mat_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_with_tex")->as<raco::user_types::Material>();

	ASSERT_EQ(mat_no_tex->uniforms_->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(mat_with_tex->uniforms_->get("u_Tex")->asRef(), texture);

	auto buffer = create<raco::user_types::RenderBuffer>("buffer");
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(buffer));

	auto meshnode_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_no_tex")->as<raco::user_types::MeshNode>();
	auto meshnode_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_with_tex")->as<raco::user_types::MeshNode>();

	ASSERT_EQ(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->asRef(), texture);

	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
}

TEST_F(MigrationTest, migrate_from_V14c) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "V14c.rca").string()));

	auto renderpass = raco::core::Queries::findByName(racoproject->project()->instances(), "MainRenderPass")->as<raco::user_types::RenderPass>();
	ASSERT_EQ(*renderpass->camera_, nullptr);

	auto texture = raco::core::Queries::findByName(racoproject->project()->instances(), "Texture")->as<raco::user_types::Texture>();
	auto mat_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_no_tex")->as<raco::user_types::Material>();
	auto mat_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_with_tex")->as<raco::user_types::Material>();

	ASSERT_EQ(mat_no_tex->uniforms_->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(mat_with_tex->uniforms_->get("u_Tex")->asRef(), texture);

	auto buffer = create<raco::user_types::RenderBuffer>("buffer");
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(mat_no_tex->uniforms_->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(mat_with_tex->uniforms_->get("u_Tex")->canSetRef(buffer));

	auto meshnode_no_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_no_tex")->as<raco::user_types::MeshNode>();
	auto meshnode_with_tex = raco::core::Queries::findByName(racoproject->project()->instances(), "meshnode_with_tex")->as<raco::user_types::MeshNode>();

	ASSERT_EQ(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->asRef(), nullptr);
	ASSERT_EQ(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->asRef(), texture);

	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(texture));
	ASSERT_TRUE(meshnode_no_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
	ASSERT_TRUE(meshnode_with_tex->getUniformContainer(0)->get("u_Tex")->canSetRef(buffer));
}

TEST_F(MigrationTest, migrate_from_current) {
	// Check for changes in serialized JSON in newest version. 
	// Should detect changes in data model with missing migration code.
	// Also checks that all object types are present in file.
	// 
	// The "version-current.rca" project needs to be updated when the data model has 
	// been changed in a way that changes the serialized JSON, e.g.
	// - static properties added
	// - annotations added to static properties
	// - added new object types

	int fileVersion;
	auto racoproject = loadAndCheckJson(QString::fromStdString((cwd_path() / "migrationTestData" / "version-current.rca").string()), &fileVersion);
	ASSERT_EQ(fileVersion, raco::components::RAMSES_PROJECT_FILE_VERSION);

	// check that all user types present in file
	auto& instances = racoproject->project()->instances();
	for (auto& item : objectFactory()->getTypes()) {
		auto name = item.first;
		EXPECT_TRUE(std::find_if(instances.begin(), instances.end(), [name](SEditorObject obj) {
			return name == obj->getTypeDescription().typeName;
		}) != instances.end());
	}
}

