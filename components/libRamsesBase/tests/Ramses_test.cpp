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

#include <ramses-framework-api/RamsesFramework.h>
#include <ramses-client-api/RamsesClient.h>
#include <ramses-client-api/Scene.h>
#include <ramses-client-api/Effect.h>
#include <ramses-client-api/EffectDescription.h>
#include <ramses-client-api/Node.h>
#include <ramses-framework-api/RamsesVersion.h>
#include "utils/stdfilesystem.h"

TEST(ramses, saveToFile_empty) {
    ramses::RamsesFramework ramsesFramework;
    ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
    ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "export scene");

    scene->saveToFile("saveToFile_empty.ramses", false);


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
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc, ramses::ResourceCacheFlag_DoNotCache);

	scene->saveToFile("saveToFile_withEffect.ramses", false);

	ASSERT_TRUE(std::filesystem::remove("saveToFile_withEffect.ramses"));
}

TEST(ramses, saveToFile_destroyNamedEffect) {
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc, ramses::ResourceCacheFlag_DoNotCache);
	effect->setName("Meow");
	scene->destroy(*effect);
	
	scene->saveToFile("saveToFile_destroyNamedEffect.ramses", false);

	
	ASSERT_TRUE(std::filesystem::remove("saveToFile_destroyNamedEffect.ramses"));
}

TEST(ramses, saveToFile_renameEffect) {
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc, ramses::ResourceCacheFlag_DoNotCache);
	effect->setName("Meow");

	scene->saveToFile("saveToFile_renameEffect.ramses", false);

	ASSERT_TRUE(std::filesystem::remove("saveToFile_renameEffect.ramses"));
}

TEST(ramses, saveToFile_destroyEffect) {
	ramses::RamsesFramework ramsesFramework;
	ramses::RamsesClient& client = *ramsesFramework.createClient("example client");
	ramses::Scene* scene = client.createScene(ramses::sceneId_t(123u), ramses::SceneConfig(), "export scene");

	ramses::EffectDescription desc{};
	desc.setVertexShader(emptyVertexShader);
	desc.setFragmentShader(emptyFragmentShader);
	ramses::Effect* effect = scene->createEffect(desc, ramses::ResourceCacheFlag_DoNotCache);
	scene->destroy(*effect);

	scene->saveToFile("saveToFile_destroyEffect.ramses", false);

	ASSERT_TRUE(std::filesystem::remove("saveToFile_destroyEffect.ramses"));
}