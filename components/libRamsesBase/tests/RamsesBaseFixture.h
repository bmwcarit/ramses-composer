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

#include "components/DataChangeDispatcher.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "user_types/UserObjectFactory.h"
#include <ramses/client/SceneObjectIterator.h>
#include <ramses/client/logic/LogicEngine.h>
#include <type_traits>

template <typename T = ramses::RamsesObject, bool StrictTypeMatch = true>
inline std::vector<T*> select(const ramses::Scene& scene, ramses::ERamsesObjectType type = ramses::ERamsesObjectType::RamsesObject) {
	std::vector<T*> result{};
	auto it = ramses::SceneObjectIterator{scene, type};
	while (auto object = it.getNext()) {
		if constexpr (StrictTypeMatch) {
			if (object->getType() == type) {
				result.push_back(static_cast<T*>(object));
			}
		} else {
			result.push_back(static_cast<T*>(object));
		}
	}
	return result;
}

template <typename T>
inline const T* select(const ramses::LogicEngine& engine, const char* name) {
	return engine.findObject<T>(name);
}

template <typename T = ramses::RamsesObject>
inline const T* select(const ramses::Scene& scene, const char* name) {
	auto result = scene.findObject<T>(name);
	EXPECT_TRUE(result != nullptr);
	return result;
}

inline std::vector<ramses::RamsesObject*> select_all(const ramses::Scene& scene) {
	return select<ramses::RamsesObject, false>(scene);
}

template <class BaseClass = ::testing::Test>
class RamsesBaseFixture : public TestEnvironmentCoreT<BaseClass> {
public:
	using DataChangeDispatcher = components::DataChangeDispatcher;

	RamsesBaseFixture(bool optimizeForExport = false, ramses::EFeatureLevel featureLevel = ramses_base::BaseEngineBackend::maxFeatureLevel)
		: TestEnvironmentCoreT<BaseClass>(&user_types::UserObjectFactory::getInstance(), featureLevel),
		  dataChangeDispatcher{std::make_shared<DataChangeDispatcher>()},
		  sceneContext{&this->backend.client(), ramses::sceneId_t{1u}, &this->project, dataChangeDispatcher, &this->errors, optimizeForExport} {}

	std::shared_ptr<DataChangeDispatcher> dataChangeDispatcher;
	ramses_adaptor::SceneAdaptor sceneContext;

	bool dispatch() {
		auto dataChanges = this->recorder.release();
		dataChangeDispatcher->dispatch(dataChanges);
		auto status = sceneContext.logicEngine().update();
		sceneContext.readDataFromEngine(dataChanges);
		return status;
	}

	template<typename T>
	inline const T* selectCheck(core::SEditorObject obj) {
		auto ramsesObj = select<T>(*sceneContext.scene(), obj->objectName().c_str());
		EXPECT_TRUE(ramsesObj != nullptr);
		EXPECT_EQ(ramsesObj->getUserId(), obj->objectIDAsRamsesLogicID());
		return ramsesObj;
	}

	template <typename T>
	inline const T* selectCheckLogic(core::SEditorObject obj) {
		auto ramsesObj = select<T>(sceneContext.logicEngine(), obj->objectName().c_str());
		EXPECT_TRUE(ramsesObj != nullptr);
		EXPECT_EQ(ramsesObj->getUserId(), obj->objectIDAsRamsesLogicID());
		return ramsesObj;
	}

	template <typename T>
	inline void dontFind(const char* name) {
		EXPECT_TRUE(sceneContext.scene()->findObject<T>(name) == nullptr);
	}

	template <typename T>
	inline void dontFindLogic(const char* name) {
		EXPECT_TRUE(sceneContext.logicEngine().findObject<T>(name) == nullptr);
	}

protected:
	template <typename RamsesType>
	bool isRamsesNameInArray(std::string_view name, const std::vector<RamsesType*>& arrayOfArrays) {
		return std::find_if(arrayOfArrays.begin(), arrayOfArrays.end(), [name](RamsesType* array) {
			return array->getName() == name;
		}) != arrayOfArrays.end();
	};
};
