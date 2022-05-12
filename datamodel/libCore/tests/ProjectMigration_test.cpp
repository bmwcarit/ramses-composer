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

#include "core/DynamicEditorObject.h"
#include "core/ProjectMigration.h"
#include "core/ProjectMigrationToV23.h"
#include "core/ProxyObjectFactory.h"
#include "utils/u8path.h"

#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"

#include "core/Link.h"
#include "core/PathManager.h"

#include "core/SerializationKeys.h"
#include "ramses_adaptor/SceneBackend.h"

#include "user_types/Animation.h"
#include "user_types/Enumerations.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderPass.h"
#include "user_types/Texture.h"

#include "testing/TestEnvironmentCore.h"

#include <gtest/gtest.h>

constexpr bool GENERATE_DIFF{false};

using namespace raco::core;

const char testName_old[] = "Test";
const char testName_new[] = "Test";

static_assert(!std::is_same<raco::serialization::proxy::Proxy<testName_old>, raco::serialization::proxy::Proxy<testName_new>>::value);

struct MigrationTest : public TestEnvironmentCore {
	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication application{backend};

	// Check if the property types coming out of the migration code agree with the types
	// in the current version of the user types.
	// Failure indicates missing migration code.
	void checkPropertyTypes(const raco::serialization::ProjectDeserializationInfoIR& deserializedIR) {
		auto userTypesPropMap = raco::serialization::makeUserTypePropertyMap();

		for (const auto obj : deserializedIR.objects) {
			const auto& typesMap = userTypesPropMap.at(obj->getTypeDescription().typeName);

			for (size_t i = 0; i < obj->size(); i++) {
				auto propName = obj->name(i);
				auto it = typesMap.find(propName);
				ASSERT_TRUE(it != typesMap.end());
				EXPECT_EQ(it->second, obj->get(i)->typeName()) << fmt::format("property name: '{}'", propName);
			}
		}
	}

	std::unique_ptr<raco::application::RaCoProject> loadAndCheckJson(QString filename, int* outFileVersion = nullptr) {
		QFile file{filename};
		EXPECT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
		auto document{QJsonDocument::fromJson(file.readAll())};
		file.close();
		auto fileVersion{raco::serialization::deserializeFileVersion(document)};
		EXPECT_TRUE(fileVersion <= raco::serialization::RAMSES_PROJECT_FILE_VERSION);
		if (outFileVersion) {
			*outFileVersion = fileVersion;
		}

		// Perform deserialization to IR and migration by hand to check output of migration code:
		auto deserializedIR{raco::serialization::deserializeProjectToIR(document, filename.toStdString())};
		raco::serialization::migrateProject(deserializedIR);
		checkPropertyTypes(deserializedIR);

		std::vector<std::string> pathStack;
		auto racoproject = raco::application::RaCoProject::loadFromFile(filename, &application, pathStack);
		EXPECT_TRUE(racoproject != nullptr);

		return racoproject;
	}
};

TEST_F(MigrationTest, migrate_from_V1) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V1.rca").string()));

	ASSERT_EQ(racoproject->project()->settings()->sceneId_.asInt(), 123);
	ASSERT_NE(racoproject->project()->settings()->objectID(), "b5535e97-4e60-4d72-99a9-b137b2ed52a5");	// this was the magic hardcoded ID originally used by the migration code.
}

TEST_F(MigrationTest, migrate_from_V9) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V9.rca").string()));

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
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V10.rca").string()));

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

	ValueHandle uniforms{material, &raco::user_types::Material::uniforms_};
	ASSERT_EQ(uniforms.get("scalar").asDouble(), 42.0);
	ASSERT_EQ(uniforms.get("count_").asInt(), 42);
	ASSERT_EQ(*uniforms.get("vec").asVec3f().x, 0.1);
	ASSERT_EQ(*uniforms.get("ambient").asVec4f().w, 0.4);
	ASSERT_EQ(*uniforms.get("iv2").asVec2i().i2_, 2);
}

TEST_F(MigrationTest, migrate_from_V12) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V12.rca").string()));

	auto pcam = raco::core::Queries::findByName(racoproject->project()->instances(), "PerspectiveCamera")->as<raco::user_types::PerspectiveCamera>();
	ASSERT_EQ(*pcam->viewport_->offsetX_, 1);
	ASSERT_EQ(*pcam->viewport_->offsetY_, 3);  // linked
	ASSERT_EQ(*pcam->viewport_->width_, 1441);
	ASSERT_EQ(*pcam->viewport_->height_, 721);

	ASSERT_EQ(*pcam->frustum_->near_, 4.5);	 /// linked
	ASSERT_EQ(*pcam->frustum_->far_, 1001.0);
	ASSERT_EQ(*pcam->frustum_->fov_, 36.0);
	ASSERT_EQ(*pcam->frustum_->aspect_, 3.0);

	auto ocam = raco::core::Queries::findByName(racoproject->project()->instances(), "OrthographicCamera")->as<raco::user_types::OrthographicCamera>();
	ASSERT_EQ(*ocam->viewport_->offsetX_, 2);
	ASSERT_EQ(*ocam->viewport_->offsetY_, 3);  // linked
	ASSERT_EQ(*ocam->viewport_->width_, 1442);
	ASSERT_EQ(*ocam->viewport_->height_, 722);

	ASSERT_EQ(*ocam->frustum_->near_, 2.1);
	ASSERT_EQ(*ocam->frustum_->far_, 1002.0);
	ASSERT_EQ(*ocam->frustum_->left_, 4.5);	 // linked
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
	checkLinks(*racoproject->project(), {{{lua, {"luaOutputs", "int"}}, {pcam, {"viewport", "offsetY"}}},
											{{lua, {"luaOutputs", "float"}}, {pcam, {"frustum", "nearPlane"}}},
											{{lua, {"luaOutputs", "int"}}, {ocam, {"viewport", "offsetY"}}},
											{{lua, {"luaOutputs", "float"}}, {ocam, {"frustum", "leftPlane"}}}});
}

TEST_F(MigrationTest, migrate_from_V13) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V13.rca").string()));

	auto textureNotFlipped = raco::core::Queries::findByName(racoproject->project()->instances(), "DuckTextureNotFlipped")->as<raco::user_types::Texture>();
	ASSERT_FALSE(*textureNotFlipped->flipTexture_);

	auto textureFlipped = raco::core::Queries::findByName(racoproject->project()->instances(), "DuckTextureFlipped")->as<raco::user_types::Texture>();
	ASSERT_TRUE(*textureFlipped->flipTexture_);
}

TEST_F(MigrationTest, migrate_from_V14) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V14.rca").string()));

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
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V14b.rca").string()));

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
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V14c.rca").string()));

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

TEST_F(MigrationTest, migrate_from_V16) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V16.rca").string()));

	auto renderlayeropt = raco::core::Queries::findByName(racoproject->project()->instances(), "RenderLayerOptimized")->as<raco::user_types::RenderLayer>();
	ASSERT_EQ(renderlayeropt->sortOrder_.asInt(), static_cast<int>(raco::user_types::ERenderLayerOrder::Manual));
	auto renderlayermanual = raco::core::Queries::findByName(racoproject->project()->instances(), "RenderLayerManual")->as<raco::user_types::RenderLayer>();
	ASSERT_EQ(renderlayermanual->sortOrder_.asInt(), static_cast<int>(raco::user_types::ERenderLayerOrder::Manual));
	auto renderlayerscenegraph = raco::core::Queries::findByName(racoproject->project()->instances(), "RenderLayerSceneGraph")->as<raco::user_types::RenderLayer>();
	ASSERT_EQ(renderlayerscenegraph->sortOrder_.asInt(), static_cast<int>(raco::user_types::ERenderLayerOrder::SceneGraph));
}

TEST_F(MigrationTest, migrate_from_V18) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V18.rca").string()));

	auto bgColor = racoproject->project()->settings()->backgroundColor_;
	ASSERT_EQ(bgColor->typeDescription.typeName, Vec4f::typeDescription.typeName);

	ASSERT_EQ(bgColor->x.asDouble(), 0.3);
	ASSERT_EQ(bgColor->y.asDouble(), 0.2);
	ASSERT_EQ(bgColor->z.asDouble(), 0.1);
	ASSERT_EQ(bgColor->w.asDouble(), 1.0);
}

TEST_F(MigrationTest, migrate_from_V21) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V21.rca").string()));

	auto resourceFolders = racoproject->project()->settings()->defaultResourceDirectories_;
	ASSERT_EQ(resourceFolders->typeDescription.typeName, ProjectSettings::DefaultResourceDirectories::typeDescription.typeName);

	ASSERT_EQ(resourceFolders->imageSubdirectory_.asString(), "images");
	ASSERT_EQ(resourceFolders->meshSubdirectory_.asString(), "meshes");
	ASSERT_EQ(resourceFolders->scriptSubdirectory_.asString(), "scripts");
	ASSERT_EQ(resourceFolders->shaderSubdirectory_.asString(), "shaders");
}

TEST_F(MigrationTest, migrate_from_V21_custom_paths) {
	std::string imageSubdirectory = "imgs";
	std::string meshSubdirectory = "mshs";
	std::string scriptSubdirectory = "spts";
	std::string shaderSubdirectory = "shds";

	auto preferencesFile = raco::core::PathManager::preferenceFilePath();
	if (preferencesFile.exists()) {
		std::filesystem::remove(preferencesFile);
	}
	
	{
		// use scope to force saving QSettings when leaving the scope
		QSettings settings(preferencesFile.string().c_str(), QSettings::IniFormat);
		settings.setValue("imageSubdirectory", imageSubdirectory.c_str());
		settings.setValue("meshSubdirectory", meshSubdirectory.c_str());
		settings.setValue("scriptSubdirectory", scriptSubdirectory.c_str());
		settings.setValue("shaderSubdirectory", shaderSubdirectory.c_str());
	}

	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V21.rca").string()));

	auto resourceFolders = racoproject->project()->settings()->defaultResourceDirectories_;
	ASSERT_EQ(resourceFolders->typeDescription.typeName, ProjectSettings::DefaultResourceDirectories::typeDescription.typeName);

	ASSERT_EQ(resourceFolders->imageSubdirectory_.asString(), imageSubdirectory);
	ASSERT_EQ(resourceFolders->meshSubdirectory_.asString(), meshSubdirectory);
	ASSERT_EQ(resourceFolders->scriptSubdirectory_.asString(), scriptSubdirectory);
	ASSERT_EQ(resourceFolders->shaderSubdirectory_.asString(), shaderSubdirectory);

	if (preferencesFile.exists()) {
		std::filesystem::remove(preferencesFile);
	}
}

TEST_F(MigrationTest, migrate_from_V23) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V23.rca").string()));

	auto prefab = raco::core::Queries::findByName(racoproject->project()->instances(), "Prefab")->as<raco::user_types::Prefab>();
	auto inst = raco::core::Queries::findByName(racoproject->project()->instances(), "PrefabInstance")->as<raco::user_types::PrefabInstance>();
	auto prefab_node = prefab->children_->asVector<SEditorObject>()[0]->as<raco::user_types::Node>();
	auto inst_node = inst->children_->asVector<SEditorObject>()[0]->as<raco::user_types::Node>();

	EXPECT_EQ(inst_node->objectID(), EditorObject::XorObjectIDs(prefab_node->objectID(), inst->objectID()));
}

TEST_F(MigrationTest, migrate_from_V29) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V29.rca").string()));

	auto animation = raco::core::Queries::findByName(racoproject->project()->instances(), "Animation")->as<raco::user_types::Animation>();
	auto lua = raco::core::Queries::findByName(racoproject->project()->instances(), "LuaScript")->as<raco::user_types::LuaScript>();

	EXPECT_TRUE(lua->luaOutputs_->hasProperty("flag"));
	EXPECT_EQ(racoproject->project()->links().size(), 0);

	EXPECT_FALSE(animation->hasProperty("play"));
	EXPECT_FALSE(animation->hasProperty("loop"));
	EXPECT_FALSE(animation->hasProperty("rewindOnStop"));
}

TEST_F(MigrationTest, migrate_V29_to_V33) {
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V29_tags.rca").string()));

	auto node = raco::core::Queries::findByName(racoproject->project()->instances(), "Node")->as<raco::user_types::Node>();
	auto mat_front = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_front")->as<raco::user_types::Material>();
	auto mat_back = raco::core::Queries::findByName(racoproject->project()->instances(), "mat_back")->as<raco::user_types::Material>();

	auto renderlayermanual = raco::core::Queries::findByName(racoproject->project()->instances(), "RenderLayerManual")->as<raco::user_types::RenderLayer>();

	EXPECT_EQ(node->tags_->asVector<std::string>(), std::vector<std::string>({"render_main"}));

	EXPECT_EQ(mat_front->tags_->asVector<std::string>(), std::vector<std::string>({"mat_front"}));
	EXPECT_EQ(mat_back->tags_->asVector<std::string>(), std::vector<std::string>({"mat_back"}));

	EXPECT_EQ(renderlayermanual->tags_->asVector<std::string>(), std::vector<std::string>({"debug"}));
	EXPECT_EQ(renderlayermanual->materialFilterTags_->asVector<std::string>(), std::vector<std::string>({"mat_front", "mat_back"}));

	EXPECT_EQ(renderlayermanual->renderableTags_->size(), 1);
	EXPECT_TRUE(renderlayermanual->renderableTags_->get("render_main") != nullptr);
	EXPECT_EQ(renderlayermanual->renderableTags_->get("render_main")->asInt(), 2);
}

TEST_F(MigrationTest, migrate_V30_to_V34) {
	// We would like a V30 file but we can't produce that after merging V28/V29 from master anymore.
	// So we use a V29 file instead.
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "V29_renderlayer.rca").string()));

	auto layer_excl = raco::core::Queries::findByName(racoproject->project()->instances(), "layer_exclusive")->as<raco::user_types::RenderLayer>();
	auto layer_incl = raco::core::Queries::findByName(racoproject->project()->instances(), "layer_inclusive")->as<raco::user_types::RenderLayer>();

	EXPECT_EQ(*layer_excl->materialFilterMode_, static_cast<int>(raco::user_types::ERenderLayerMaterialFilterMode::Exclusive));
	EXPECT_EQ(*layer_incl->materialFilterMode_, static_cast<int>(raco::user_types::ERenderLayerMaterialFilterMode::Inclusive));
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
	auto racoproject = loadAndCheckJson(QString::fromStdString((test_path() / "migrationTestData" / "version-current.rca").string()), &fileVersion);
	ASSERT_EQ(fileVersion, raco::serialization::RAMSES_PROJECT_FILE_VERSION);

	// check that all user types present in file
	auto& instances = racoproject->project()->instances();
	for (auto& item : objectFactory()->getTypes()) {
		auto name = item.first;
		EXPECT_TRUE(std::find_if(instances.begin(), instances.end(), [name](SEditorObject obj) {
			return name == obj->getTypeDescription().typeName;
		}) != instances.end());
	}
}

TEST_F(MigrationTest, check_current_type_maps) {
	// Check that the type maps for user object and structs types in the "version-current.rca" are
	// identical to the ones generated when saving a project.
	//
	// If this fails the file format has changed: we need to increase the file version number
	// and potentially write migration code.

	QString filename = QString::fromStdString((test_path() / "migrationTestData" / "version-current.rca").string());
	QFile file{filename};
	EXPECT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
	auto document{QJsonDocument::fromJson(file.readAll())};
	file.close();

	auto fileUserPropTypeMap = raco::serialization::deserializeUserTypePropertyMap(document[raco::serialization::keys::USER_TYPE_PROP_MAP]);
	auto fileStructTypeMap = raco::serialization::deserializeUserTypePropertyMap(document[raco::serialization::keys::STRUCT_PROP_MAP]);

	auto currentUserPropTypeMap = raco::serialization::makeUserTypePropertyMap();
	auto currentStructTypeMap = raco::serialization::makeStructPropertyMap();

	EXPECT_EQ(fileUserPropTypeMap, currentUserPropTypeMap);
	EXPECT_EQ(fileStructTypeMap, currentStructTypeMap);
}

TEST_F(MigrationTest, check_proxy_factory_has_all_objects_types) {
	// Check that all types in the UserObjectFactory constructory via makeTypeMap call
	// also have the corresponding proxy type added in the ProxyObjectFactory constructor.
	// If this fails add the type in the ProxyObjectFactory constructor makeTypeMap call.

	auto& proxyFactory{raco::serialization::proxy::ProxyObjectFactory::getInstance()};
	auto& proxyTypeMap{proxyFactory.getTypes()};

	for (auto& item : objectFactory()->getTypes()) {
		auto name = item.first;
		EXPECT_TRUE(proxyTypeMap.find(name) != proxyTypeMap.end()) << fmt::format("type name: '{}'", name);
	}
}

TEST_F(MigrationTest, check_proxy_factory_has_all_dynamic_property_types) {
	// Check that all dynamic properties contained in UserObjectFactory::PropertyTypeMapType
	// have their corresponding properties added in ProxyObjectFactory::PropertyTypeMapType too.
	// If this fails add the property in ProxyObjectFactory::PropertyTypeMapType.

	auto& proxyFactory{raco::serialization::proxy::ProxyObjectFactory::getInstance()};
	auto& userFactory{UserObjectFactory::getInstance()};
	auto& proxyProperties{proxyFactory.getProperties()};

	for (auto& item : userFactory.getProperties()) {
		auto name = item.first;
		EXPECT_TRUE(proxyProperties.find(name) != proxyProperties.end()) << fmt::format("property name: '{}'", name);
	}
}
TEST_F(MigrationTest, check_proxy_factory_can_create_all_static_properties) {
	// Check that the ProxyObjectFactory can create all statically known properties.
	// If this fails add the failing property to the ProxyObjectFactory::PropertyTypeMapType.

	auto& proxyFactory{raco::serialization::proxy::ProxyObjectFactory::getInstance()};
	auto& userFactory{UserObjectFactory::getInstance()};

	for (auto& item :userFactory.getTypes()) {
		auto name = item.first;
		auto object = objectFactory()->createObject(name);
		ASSERT_TRUE(object != nullptr);
		for (size_t index = 0; index < object->size(); index++) {
			auto propTypeName = object->get(index)->typeName();
			auto proxyProperty = proxyFactory.createValue(propTypeName);
			ASSERT_TRUE(proxyProperty != nullptr) << fmt::format("property type name: '{}'", propTypeName);
			ASSERT_EQ(proxyProperty->typeName(), propTypeName) << fmt::format("property type name: '{}'", propTypeName);
		}
	}

	for (auto& item : userFactory.getStructTypes()) {
		auto name = item.first;
		auto object = userFactory.createStruct(name);
		ASSERT_TRUE(object != nullptr);
		for (size_t index = 0; index < object->size(); index++) {
			auto propTypeName = object->get(index)->typeName();
			auto proxyProperty = proxyFactory.createValue(propTypeName);
			ASSERT_TRUE(proxyProperty != nullptr) << fmt::format("property type name: '{}'", propTypeName);
			ASSERT_EQ(proxyProperty->typeName(), propTypeName) << fmt::format("property type name: '{}'", propTypeName);
		}
	}
}
