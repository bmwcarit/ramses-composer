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

#include "core/Handles.h"
#include "core/Queries.h"

#include "ramses_adaptor/AbstractObjectAdaptor.h"
#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/Node.h"
#include <array>

namespace raco::ramses_adaptor {

class AbstractNodeAdaptor;

template <typename EditorType, typename RamsesType>
class AbstractSpatialAdaptor : public AbstractTypedObjectAdaptor<EditorType, RamsesType>, public AbstractISceneObjectProvider {
public:
	explicit AbstractSpatialAdaptor(AbstractSceneAdaptor* sceneAdaptor, std::shared_ptr<EditorType> editorObject, RamsesHandle<RamsesType>&& ramsesObject, AbstractNodeAdaptor* parent = nullptr)
		: AbstractTypedObjectAdaptor<EditorType, RamsesType>{sceneAdaptor, editorObject, std::move(ramsesObject)},
		  subscriptions_{
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("children"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("translation"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("scaling"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("rotation"), [this](auto) { this->tagDirty(); })} {
	}

	~AbstractSpatialAdaptor() {
		this->resetRamsesObject();
		currentRamsesChildren_.clear();
	}

	bool sync() override {
		bool status = AbstractTypedObjectAdaptor<EditorType, RamsesType>::sync();
		syncVisibility();
		syncRotation();
		syncTranslation();
		syncScaling();
		syncChildren();
		this->tagDirty(false);
		return status;
	}

	RamsesHandle<ramses::Node> sceneObject() override {
		auto handlePtr = this->getRamsesObjectPointer();
		return std::shared_ptr<ramses::Node>(handlePtr, handlePtr->get()); 
	}

protected:
	std::vector<RamsesHandle<ramses::Node>> currentRamsesChildren_;

private:
	void syncChildren() {
		(*this->ramsesObject()).removeAllChildren();
		currentRamsesChildren_.clear();

		for (const auto& child : *this->editorObject()) {
			AbstractSceneAdaptor* scene = this->sceneAdaptor_;
			auto castedChild = scene->lookup<AbstractISceneObjectProvider>(child);
			if (castedChild) {
				auto handle{castedChild->sceneObject()};
				(*this->ramsesObject()).addChild(*handle);
				currentRamsesChildren_.emplace_back(handle);

			}
		}
	}

	void syncVisibility() {
		// scene view objects are currently always visible
	}

	void syncRotation() {
		if (getRacoRotation(this->editorObject()) != getRamsesRotation(this->ramsesObject().get())) {
			(*this->ramsesObject()).setRotation(getRacoRotation(this->editorObject()), ramses_adaptor::RAMSES_ROTATION_CONVENTION);
		}
	}

	void syncTranslation() {
		if (getRacoTranslation(this->editorObject()) != getRamsesTranslation(this->ramsesObject().get())) {
			(*this->ramsesObject()).setTranslation(getRacoTranslation(this->editorObject()));
		}
	}

	void syncScaling() {
		if (getRacoScaling(this->editorObject()) != getRamsesScaling(this->ramsesObject().get())) {
			(*this->ramsesObject()).setScaling(getRacoScaling(this->editorObject()));
		}
	}

	std::array<components::Subscription, 4> subscriptions_;
};

class AbstractNodeAdaptor final : public AbstractSpatialAdaptor<user_types::Node, ramses_base::RamsesNodeHandle> {
public:
	explicit AbstractNodeAdaptor(AbstractSceneAdaptor* sceneAdaptor, user_types::SNode node);
};

};	// namespace raco::ramses_adaptor
