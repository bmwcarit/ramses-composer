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
#include "core/Queries.h"
#include "log_system/log.h"
#include "ramses_adaptor/ObjectAdaptor.h"

namespace raco::ramses_adaptor {

namespace {

LinkAdaptor::UniqueEngineLink engineLink(rlogic::LogicEngine* engine, const rlogic::Property& origin, const rlogic::Property& dest) {
	if (engine->link(origin, dest)) {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "create: {}:{}->{}:{}", fmt::ptr(&origin), origin.getName(), fmt::ptr(&dest), dest.getName());
		return {new LinkAdaptor::EngineLink{&origin, &dest}, [engine](LinkAdaptor::EngineLink* link) {
					bool success = engine->unlink(*link->origin, *link->dest);
					LOG_TRACE(log_system::RAMSES_ADAPTOR, "destroy: {}->{} ({})", fmt::ptr(link->origin), fmt::ptr(link->dest), success);
					assert(success);
				}};
	} else {
		LOG_WARNING(log_system::RAMSES_ADAPTOR, "failed: {}->{}", fmt::ptr(&origin), fmt::ptr(&dest));
		assert(false);
		return {};
	}
}

inline const void eachLinkableProperty(const rlogic::Property& a, const rlogic::Property& b, const std::function<void(const rlogic::Property&, const rlogic::Property&)>& fn) {
	assert(a.getType() == b.getType());
	if (a.getType() == rlogic::EPropertyType::Struct) {
		assert(a.getChildCount() == b.getChildCount());
		for (int i = 0; i < a.getChildCount(); i++) {
			const auto& nextA{*a.getChild(i)};
			eachLinkableProperty(nextA, *b.getChild(nextA.getName()), fn);
		}
	} else if (a.getType() == rlogic::EPropertyType::Array) {
		assert(a.getChildCount() == b.getChildCount());
		for (int i = 0; i < a.getChildCount(); i++) {
			eachLinkableProperty(*a.getChild(i), *b.getChild(i), fn);
		}
	} else {
		fn(a, b);
	} 
}

}  // namespace

LinkAdaptor::LinkAdaptor(const core::LinkDescriptor& link, SceneAdaptor* sceneAdaptor) : editorLink_{link}, sceneAdaptor_{sceneAdaptor} {
	connect();
}

void LinkAdaptor::lift() {
	LOG_TRACE(log_system::RAMSES_ADAPTOR, "{}", editorLink_);
	engineLink_.clear();
}

void LinkAdaptor::connect() {
	LOG_TRACE(log_system::RAMSES_ADAPTOR, "{}", editorLink_);
	engineLink_.clear();

	auto originAdaptor{sceneAdaptor_->lookupAdaptor(editorLink_.start.object())};
	auto destAdaptor{sceneAdaptor_->lookupAdaptor(editorLink_.end.object())};

	if (originAdaptor && destAdaptor && editorLink_.isValid) {
		auto startProp = dynamic_cast<ILogicPropertyProvider*>(originAdaptor)->getProperty(editorLink_.start.propertyNames());
		auto endProp = dynamic_cast<ILogicPropertyProvider*>(destAdaptor)->getProperty(editorLink_.end.propertyNames());
		if (startProp && endProp) {
			eachLinkableProperty(*startProp, *endProp,
				[this](const rlogic::Property& a, const rlogic::Property& b) {
					engineLink_.push_back(engineLink(&sceneAdaptor_->logicEngine(), a, b));
				});
		}
	} else {
		LOG_TRACE(log_system::RAMSES_ADAPTOR, "Ramses logic link {}.{}->{}.{} could not be created", editorLink_.start.object()->objectName(), fmt::join(editorLink_.start.propertyNames(), "."), editorLink_.end.object()->objectName(), fmt::join(editorLink_.end.propertyNames(), "."));
	}
}

void LinkAdaptor::readDataFromEngine(core::DataChangeRecorder& recorder) {
	auto destAdaptor{sceneAdaptor_->lookupAdaptor(editorLink_.end.object())};
	raco::core::ValueHandle destHandle{editorLink_.end};
	if (destAdaptor && destHandle && editorLink_.isValid) {
		auto endProp = dynamic_cast<ILogicPropertyProvider*>(destAdaptor)->getProperty(editorLink_.end.propertyNames());
		if (endProp) {
			getLuaOutputFromEngine(*endProp, destHandle, recorder);
		}
	}
}

}  // namespace raco::ramses_adaptor
