/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/LuaInterface.h"
#include "user_types/SyncTableWithEngineInterface.h"

namespace raco::ramses_adaptor {

namespace {

LinkAdaptor::UniqueEngineLink createEngineLink(ramses::LogicEngine* engine, ramses::Property& origin, ramses::Property& dest, bool isWeak) {
	if (isWeak ? engine->linkWeak(origin, dest) : engine->link(origin, dest)) {
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

}  // namespace

LinkAdaptor::LinkAdaptor(const core::LinkDescriptor& link, SceneAdaptor* sceneAdaptor) : editorLink_{link}, sceneAdaptor_{sceneAdaptor} {
	connect();
}

void LinkAdaptor::lift() {
	LOG_TRACE(log_system::RAMSES_ADAPTOR, "Lift editor link: {}", editorLink_);
	engineLinks_.clear();
}

void LinkAdaptor::connectHelper(const core::ValueHandle& start, const core::ValueHandle& end, bool isWeak) {
	if (!core::Queries::isEnginePrimitive(end)) {
		for (size_t index = 0; index < end.size(); index++) {
			auto endChild = end[index];
			connectHelper(start.get(endChild.getPropName()), endChild, isWeak);
		}
	} else {
		std::optional<core::PropertyDescriptor> startPropOpt = start.getDescriptor();
		if (sceneAdaptor_->optimizeForExport()) {
			startPropOpt = followLinkChain(sceneAdaptor_->project(), start.getDescriptor());
		}
		if (startPropOpt) {
			auto startProp = startPropOpt.value();
			if (auto startAdaptor = sceneAdaptor_->lookupAdaptor(startProp.object())) {
				const auto& names = startProp.propertyNames();
				if (auto startEngineProp = dynamic_cast<ILogicPropertyProvider*>(startAdaptor)->getProperty({names.begin(), names.end()})) {
					if (auto endAdaptor = sceneAdaptor_->lookupAdaptor(editorLink_.end.object())) {
						if (auto endEngineProp = dynamic_cast<ILogicPropertyProvider*>(endAdaptor)->getProperty(end.getPropertyNamesVector())) {
							if (auto engineLink = createEngineLink(&sceneAdaptor_->logicEngine(), *startEngineProp, *endEngineProp, isWeak)) {
								engineLinks_.emplace_back(std::move(engineLink));
							}
						}
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

	if (editorLink_.isValid) {
		connectHelper(core::ValueHandle(editorLink_.start), core::ValueHandle(editorLink_.end), editorLink_.isWeak);
	}
}

bool LinkAdaptor::readFromEngineRecursive(core::DataChangeRecorder& recorder, const core::ValueHandle& property) {
	bool changed = false;
	if (property) {
		if (!core::Queries::isEnginePrimitive(property)) {
			for (size_t index = 0; index < property.size(); index++) {
				changed = readFromEngineRecursive(recorder, property[index]) || changed;
			}
		} else {
			if (auto adaptor = sceneAdaptor_->lookupAdaptor(property.rootObject())) {
				if (auto engineProp = dynamic_cast<ILogicPropertyProvider*>(adaptor)->getProperty(property.getPropertyNamesVector())) {
					changed = getOutputFromEngine(*engineProp, property, recorder) || changed;
				}
			}
		}
	}
	return changed;
}

bool LinkAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	if (editorLink_.isValid) {
		return readFromEngineRecursive(recorder, core::ValueHandle(editorLink_.end));
	}
	return false;
}

}  // namespace raco::ramses_adaptor
