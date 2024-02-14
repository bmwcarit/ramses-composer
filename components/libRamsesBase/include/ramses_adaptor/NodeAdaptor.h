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
				  core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
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
				  core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
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
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("enabled"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("children"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("translation"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("scaling"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("rotation"), [this](auto) { this->tagDirty(); })} {
		core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
		if (auto link = core::Queries::getLink(sceneAdaptor->project(), rotation)) {
			rotationType_ = newRotationType(link->descriptor());
			setupLinkStartSubscription();
		}

		nodeBinding_ = ramses_base::ramsesNodeBinding(this->getRamsesObjectPointer(), &sceneAdaptor->logicEngine(), rotationType_, this->editorObject()->objectIDAsRamsesLogicID());
	}

	void setupLinkStartSubscription() {
		core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
		if (auto link = core::Queries::getLink(this->sceneAdaptor_->project(), rotation)) {
			// TODO this is total overkill
			// but we dont have a subscription which will trigger when the property or any parent property is changed.
			linkStartSubscription_ = this->sceneAdaptor_->dispatcher()->registerOnChildren(core::ValueHandle{*link->startObject_},
			[this](auto) {
					core::PropertyDescriptor rotation{this->editorObject(), std::vector<std::string>{"rotation"}};
					if (auto link = core::Queries::getLink(this->sceneAdaptor_->project(), rotation)) {
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

	void getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const override {
		logicNodes.push_back(nodeBinding_.get());
	}
	
	ramses::Property* getProperty(const std::vector<std::string_view>& propertyNamesVector) override {
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
		errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
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

	ramses_base::RamsesNodeBinding nodeBinding() const {
		return nodeBinding_;
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
		nodeBinding_ = ramses_base::ramsesNodeBinding(this->getRamsesObjectPointer(), &this->sceneAdaptor_->logicEngine(), rotationType_, this->editorObject()->objectIDAsRamsesLogicID());
		nodeBinding_->setName(this->editorObject().get()->objectName() + "_NodeBinding");
	}

	void syncVisibility() {
		auto visibility = core::ValueHandle{this->editorObject()}.get("visibility").as<bool>();
		nodeBinding_->getInputs()->getChild("visibility")->set(visibility);

		auto enabled = core::ValueHandle{this->editorObject()}.get("enabled").as<bool>();
		nodeBinding_->getInputs()->getChild("enabled")->set(enabled);
	}

	void syncRotation() {
		if ((*this->ramsesObject()).getRotationType() != ramses_adaptor::RAMSES_ROTATION_CONVENTION ||
			getRacoRotation(this->editorObject()) != getRamsesRotation(this->ramsesObject().get())) {
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

	ramses::ERotationType newRotationType(const core::LinkDescriptor& linkDescriptor) {
		if (linkDescriptor.isValid) {
			auto startHandle = core::ValueHandle{linkDescriptor.start};

			if (startHandle && startHandle.isVec4f()) {
				return ramses::ERotationType::Quaternion;
			}
		}

		return DEFAULT_VEC3_ROTATION_TYPE;
	}

	constexpr static inline ramses::ERotationType DEFAULT_VEC3_ROTATION_TYPE = ramses::ERotationType::Euler_ZYX;

	ramses::ERotationType rotationType_;
	ramses_base::RamsesNodeBinding nodeBinding_;
	std::array<components::Subscription, 6> subscriptions_;
	components::Subscription linksLifecycle_;
	components::Subscription linkStartSubscription_;
};

class NodeAdaptor final : public SpatialAdaptor<user_types::Node, ramses_base::RamsesNodeHandle> {
public:
	explicit NodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SNode node);
	std::vector<ExportInformation> getExportInformation() const override;
};

};	// namespace raco::ramses_adaptor
