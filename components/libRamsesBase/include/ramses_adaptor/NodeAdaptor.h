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

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"
#include "components/DataChangeDispatcher.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include <array>

namespace raco::ramses_adaptor {

class NodeAdaptor;

template <typename EditorType, typename RamsesType>
class SpatialAdaptor : public TypedObjectAdaptor<EditorType, RamsesType>, public ILogicPropertyProvider, public ISceneObjectProvider {
public:
	explicit SpatialAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<EditorType> editorObject, RamsesHandle<RamsesType>&& ramsesObject, NodeAdaptor* parent = nullptr)
		: TypedObjectAdaptor<EditorType, RamsesType>{sceneAdaptor, editorObject, std::move(ramsesObject)},
		  rotationType_{DEFAULT_VEC3_ROTATION_TYPE},
		  nodeBinding_{},
		  linksLifecycle_{sceneAdaptor->dispatcher()->registerOnLinksLifeCycleForEnd(
			  this->editorObject(),
			  [this](const core::LinkDescriptor& link) {
				  raco::core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
				  if (link.end == rotation) {
					  setupLinkStartSubscription();
						auto oldRotationType = this->rotationType_;
						this->rotationType_ = this->newRotationType(link);
						if (this->rotationType_ != oldRotationType) {
							this->tagDirty();
						}
				  }
			  },
			  [this](const core::LinkDescriptor& link) {
				  raco::core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
				  if (link.end == rotation) {
					  setupLinkStartSubscription();

					  if (rotationType_ != DEFAULT_VEC3_ROTATION_TYPE) {
						  rotationType_ = DEFAULT_VEC3_ROTATION_TYPE;
						  this->tagDirty();
					  }
				  }
			  })},
		  subscriptions_{
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("visibility"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("children"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("translation"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("scaling"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("rotation"), [this](auto) { this->tagDirty(); })} {
		raco::core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
		if (auto link = raco::core::Queries::getLink(sceneAdaptor->project(), rotation)) {
			rotationType_ = newRotationType(link->descriptor());
			setupLinkStartSubscription();
		}

		nodeBinding_ = raco::ramses_base::ramsesNodeBinding(*this->ramsesObject(), &sceneAdaptor->logicEngine(), rotationType_, this->editorObject()->objectIDAsRamsesLogicID());
	}

	void setupLinkStartSubscription() {
		raco::core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
		if (auto link = raco::core::Queries::getLink(this->sceneAdaptor_->project(), rotation)) {
			// TODO this is total overkill
			// but we dont have a subscription which will trigger when the property or any parent property is changed.
			linkStartSubscription_ = this->sceneAdaptor_->dispatcher()->registerOnChildren(raco::core::ValueHandle{*link->startObject_},
			[this](auto) {
					raco::core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
					if (auto link = raco::core::Queries::getLink(this->sceneAdaptor_->project(), rotation)) {
						auto oldRotationType = rotationType_;
						rotationType_ = newRotationType(link->descriptor());
						if (rotationType_ != oldRotationType) {
							this->tagDirty();
						}
					}
				});
		} else {
			linkStartSubscription_ = components::Subscription{};
		}
	}

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override {
		logicNodes.push_back(nodeBinding_.get());
	}
	
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override {
		if (nodeBinding_) {
			return nodeBinding_->getInputs()->getChild(propertyNamesVector[0]);
		}
		return nullptr;
	}
	
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override {
		core::ValueHandle const valueHandle{ this->editorObject() };
		if (errors.hasError(valueHandle)) {
			return;
		}
		errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR, level, valueHandle, message);
	}

	~SpatialAdaptor() {
		this->resetRamsesObject();
		currentRamsesChildren_.clear();
	}

	bool sync(core::Errors* errors) override {
		bool status = TypedObjectAdaptor<EditorType, RamsesType>::sync(errors);
		syncNodeBinding();
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
			SceneAdaptor* scene = this->sceneAdaptor_;
			auto castedChild = scene->lookup<ISceneObjectProvider>(child);
			if (castedChild) {
				auto handle{castedChild->sceneObject()};
				(*this->ramsesObject()).addChild(*handle);
				currentRamsesChildren_.emplace_back(handle);

			}
		}
	}

	void syncNodeBinding() {
		nodeBinding_ = raco::ramses_base::ramsesNodeBinding(*this->ramsesObject(), &this->sceneAdaptor_->logicEngine(), rotationType_, this->editorObject()->objectIDAsRamsesLogicID());
		nodeBinding_->setName(this->editorObject().get()->objectName() + "_NodeBinding");
	}

	void syncVisibility() {
		auto visible = core::ValueHandle{this->editorObject()}.get("visibility").as<bool>();
		if (((*this->ramsesObject()).getVisibility() == ramses::EVisibilityMode::Visible) != visible) {
			(*this->ramsesObject()).setVisibility(visible ? ramses::EVisibilityMode::Visible : ramses::EVisibilityMode::Invisible);
		}
	}

	void syncRotation() {
		if (Rotation::from(this->editorObject()) != Rotation::from(*this->ramsesObject())) {
			Rotation::sync(this->editorObject(), *this->ramsesObject());
		}

	}

	void syncTranslation() {
		if (Translation::from(this->editorObject()) != Translation::from(*this->ramsesObject())) {
			Translation::sync(this->editorObject(), *this->ramsesObject());
		}
	}

	void syncScaling() {
		if (Scaling::from(this->editorObject()) != Scaling::from(*this->ramsesObject())) {
			Scaling::sync(this->editorObject(), *this->ramsesObject());
		}
	}

	rlogic::ERotationType newRotationType(const raco::core::LinkDescriptor& linkDescriptor) {
		if (linkDescriptor.isValid) {
			auto startHandle = raco::core::ValueHandle{linkDescriptor.start};

			if (startHandle && startHandle.isVec4f()) {
				return rlogic::ERotationType::Quaternion;
			}
		}

		return DEFAULT_VEC3_ROTATION_TYPE;
	}

	constexpr static inline rlogic::ERotationType DEFAULT_VEC3_ROTATION_TYPE = rlogic::ERotationType::Euler_ZYX;

	rlogic::ERotationType rotationType_;
	raco::ramses_base::UniqueRamsesNodeBinding nodeBinding_;
	std::array<components::Subscription, 5> subscriptions_;
	components::Subscription linksLifecycle_;
	components::Subscription linkStartSubscription_;
};

class NodeAdaptor final : public SpatialAdaptor<user_types::Node, ramses_base::RamsesNodeHandle> {
public:
	explicit NodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SNode node);
};

};	// namespace raco::ramses_adaptor
