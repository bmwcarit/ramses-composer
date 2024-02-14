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

#include <filesystem>
#include <ramses/client/Effect.h>
#include <ramses/client/EffectDescription.h>
#include <ramses/client/Node.h>
#include <ramses/client/RamsesClient.h>
#include <ramses/client/Scene.h>
#include <ramses/framework/RamsesFramework.h>
#include <ramses/framework/RamsesVersion.h>

TEST(ramses, saveToFile_empty) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "export scene");

	ASSERT_TRUE(scene->saveToFile("saveToFile_empty.ramses"));

	ASSERT_TRUE(std::filesystem::remove("saveToFile_empty.ramses"));
}

constexpr const char* emptyVertexShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

constexpr const char* emptyFragmentShader =
	"#version 300 es\n\
		precision mediump float;\n\
		void main() {}";

TEST(ramses, saveToFile_withEffect) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc);

	ASSERT_TRUE(scene->saveToFile("saveToFile_withEffect.ramses"));

	ASSERT_TRUE(std::filesystem::remove("saveToFile_withEffect.ramses"));
}

TEST(ramses, saveToFile_destroyNamedEffect) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc);
	effect->setName("Meow");
	scene->destroy(*effect);

	ASSERT_TRUE(scene->saveToFile("saveToFile_destroyNamedEffect.ramses"));

	ASSERT_TRUE(std::filesystem::remove("saveToFile_destroyNamedEffect.ramses"));
}

TEST(ramses, saveToFile_renameEffect) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc);
	effect->setName("Meow");

	ASSERT_TRUE(scene->saveToFile("saveToFile_renameEffect.ramses"));

	ASSERT_TRUE(std::filesystem::remove("saveToFile_renameEffect.ramses"));
}

TEST(ramses, saveToFile_destroyEffect) {
	ramses::RamsesFramework ramsesFramework{ramses::RamsesFrameworkConfig{ramses::EFeatureLevel::EFeatureLevel_01}};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc);
	scene->destroy(*effect);

	ASSERT_TRUE(scene->saveToFile("saveToFile_destroyEffect.ramses"));

	ASSERT_TRUE(std::filesystem::remove("saveToFile_destroyEffect.ramses"));
}

TEST(ramses, log_export) {
	static ramses::RamsesFrameworkConfig config(ramses::EFeatureLevel::EFeatureLevel_01);
	config.setLogLevel(ramses::ELogLevel::Trace);
	ramses::RamsesFramework ramsesFramework{config};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "example scene");

	ramses::SaveFileConfig saveConfig;
	saveConfig.setMetadataString(R"({
 "generator" : "test" 
})");
	ASSERT_TRUE(scene->saveToFile("saveToFile_empty.ramses", saveConfig));
	ASSERT_TRUE(std::filesystem::remove("saveToFile_empty.ramses"));
}

TEST(ramses, log_create_node) {
	ramses::RamsesFrameworkConfig config{ramses::EFeatureLevel::EFeatureLevel_01};
	config.setLogLevel(ramses::ELogLevel::Trace);
	ramses::RamsesFramework ramsesFramework{config};
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), "example scene");

	scene->createNode("{}");
}
