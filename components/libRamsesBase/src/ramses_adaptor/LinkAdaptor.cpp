/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/LinkAdaptor.h"

#include "core/CoreFormatter.h"
#include "core/PrefabOperations.h"
#include "core/Queries.h"
#include "log_system/log.h"
#include "ramses_adaptor/ObjectAdaptor.h"
#include "user_types/SyncTableWithEngineInterface.h"
#include "user_types/LuaInterface.h"

namespace raco::ramses_adaptor {

namespace {

LinkAdaptor::UniqueEngineLink createEngineLink(rlogic::LogicEngine* engine, const rlogic::Property& origin, const rlogic::Property& dest) {
	if (engine->link(origin, dest)) {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "Create LogicEngine link: {}:{}->{}:{}", fmt::ptr(&origin), origin.getName(), fmt::ptr(&dest), dest.getName());
		return {new LinkAdaptor::EngineLink{&origin, &dest}, [engine](LinkAdaptor::EngineLink* link) {
					bool success = engine->unlink(*link->origin, *link->dest);
					LOG_TRACE(log_system::RAMSES_ADAPTOR, "Destroy LogicEngine link: {}->{} ({})", fmt::ptr(link->origin), fmt::ptr(link->dest), success);
					assert(success);
				}};
	} else {
		LOG_WARNING(log_system::RAMSES_ADAPTOR, "Create LogicEngine link failed: {}->{}", fmt::ptr(&origin), fmt::ptr(&dest));
		assert(false);
		return {};
	}
}

std::optional<core::PropertyDescriptor> followLinkChain(const core::Project& project, core::PropertyDescriptor prop) {
	while (prop.object()->isType<user_types::LuaInterface>()) {
		core::ValueHandle current{prop};
		auto link = core::Queries::getLink(project, current.getDescriptor());

		while (!link && current && current.depth() > 0) {
			current = current.parent();
			if (current && current.depth() > 0) {
				link = core::Queries::getLink(project, current.getDescriptor());
			}
		}

		if (!link) {
			return prop;
		}
		if (!link->isValid()) {
			return std::nullopt;
		}

		std::vector<std::string> propNames = link->startPropertyNamesVector();
		std::copy(prop.propertyNames().begin() + current.depth(), prop.propertyNames().end(), std::back_inserter(propNames));

		prop = core::PropertyDescriptor(*link->startObject_, propNames);

		if (!core::ValueHandle(prop)) {
			return std::nullopt;
		}
	}
	return prop;
}

}// namespace

LinkAdaptor::LinkAdaptor(const core::LinkDescriptor& link, SceneAdaptor* sceneAdaptor) : editorLink_{link}, sceneAdaptor_{sceneAdaptor} {
	connect();
}

void LinkAdaptor::lift() {
	LOG_TRACE(log_system::RAMSES_ADAPTOR, "Lift editor link: {}", editorLink_);
	engineLinks_.clear();
}

void LinkAdaptor::connectHelper(const core::PropertyDescriptor& start, const rlogic::Property& endEngineProp) {
	core::ValueHandle startHandle(start);

	auto engineType = endEngineProp.getType();
	if (engineType == rlogic::EPropertyType::Struct || engineType == rlogic::EPropertyType::Array) {
		for (size_t index = 0; index < endEngineProp.getChildCount(); index++) {
			auto endChild = endEngineProp.getChild(index);
			std::string propName = user_types::dataModelPropNameForLogicEnginePropName(std::string(endChild->getName()), index);
			connectHelper(start.child(propName), *endChild);
		}
	} else {
		std::optional<core::PropertyDescriptor> startPropOpt = start;
		if (sceneAdaptor_->optimizeForExport()) {
			startPropOpt = followLinkChain(sceneAdaptor_->project(), start);
		}
		if (startPropOpt) {
			auto startProp = startPropOpt.value();
			auto startAdaptor(sceneAdaptor_->lookupAdaptor(startProp.object()));
			if (startAdaptor) {
				auto startEngineProp = dynamic_cast<ILogicPropertyProvider*>(startAdaptor)->getProperty(startProp.propertyNames());
				if (startEngineProp) {
					auto engineLink = createEngineLink(&sceneAdaptor_->logicEngine(), *startEngineProp, endEngineProp);
					if (engineLink) {
						engineLinks_.emplace_back(std::move(engineLink));
					}
				}
			}
		}
	}
}

void LinkAdaptor::connect() {
	LOG_TRACE(log_system::RAMSES_ADAPTOR, "Connect editor link: {}", editorLink_);
	engineLinks_.clear();

	if (sceneAdaptor_->optimizeForExport() && editorLink_.end.object()->isType<user_types::LuaInterface>()) {
		return;
	}

	auto destAdaptor{sceneAdaptor_->lookupAdaptor(editorLink_.end.object())};

	if (editorLink_.isValid && destAdaptor) {
		auto endEngineProp = dynamic_cast<ILogicPropertyProvider*>(destAdaptor)->getProperty(editorLink_.end.propertyNames());
		if (endEngineProp) {
			connectHelper(editorLink_.start, *endEngineProp);
		}
	}
}

void LinkAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	auto destAdaptor{sceneAdaptor_->lookupAdaptor(editorLink_.end.object())};
	raco::core::ValueHandle destHandle{editorLink_.end};
	if (destAdaptor && destHandle && editorLink_.isValid) {
		auto endProp = dynamic_cast<ILogicPropertyProvider*>(destAdaptor)->getProperty(editorLink_.end.propertyNames());
		if (endProp) {
			getOutputFromEngine(*endProp, destHandle, recorder);
		}
	}
}

}  // namespace raco::ramses_adaptor
