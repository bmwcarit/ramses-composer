/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/Handles.h"

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
		  nodeBinding_{raco::ramses_base::ramsesNodeBinding(this->ramsesObject(), &sceneAdaptor->logicEngine())},
		  subscriptions_{
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("visible"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("children"), [this]() { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("translation"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("scale"), [this](auto) { this->tagDirty(); }),
			  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject}.get("rotation"), [this](auto) { this->tagDirty(); })} {		
		
	}

	void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const override {
		logicNodes.push_back(nodeBinding_.get());
	}
	
	const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) override {
		using raco::user_types::Node;
		using raco::user_types::property_name;

		static std::map<std::string_view, std::string_view> propertyNameToEngineName{
			{property_name<&Node::translation_>::value, engine_property_name<&Node::translation_>::value},
			{property_name<&Node::visible_>::value, engine_property_name<&Node::visible_>::value},
			{property_name<&Node::rotation_>::value, engine_property_name<&Node::rotation_>::value},
			{property_name<&Node::scale_>::value, engine_property_name<&Node::scale_>::value},
		};
		std::string propName = propertyNamesVector[0];
		assert(propertyNamesVector.size() == 1);
		assert(propertyNameToEngineName.find(propName) != propertyNameToEngineName.end());
		return nodeBinding_->getInputs()->getChild(propertyNameToEngineName.at(propName));
	}
	
	void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) override {
		core::ValueHandle const valueHandle{ this->baseEditorObject() };
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
		return this->getRamsesObjectPointer();
	}

protected:
	std::vector<RamsesHandle<ramses::Node>> currentRamsesChildren_;

private:
	void syncChildren() {
		this->ramsesObject().removeAllChildren();
		currentRamsesChildren_.clear();

		for (const auto& child : *this->editorObject()) {
			SceneAdaptor* scene = this->sceneAdaptor_;
			auto castedChild = scene->lookup<ISceneObjectProvider>(child);
			if (castedChild) {
				auto handle{castedChild->sceneObject()};
				this->ramsesObject().addChild(*handle);
				currentRamsesChildren_.emplace_back(handle);

			}
		}
	}

	void syncNodeBinding() {
		nodeBinding_->setName(this->editorObject().get()->objectName() + "_NodeBinding");
	}

	void syncVisibility() {
		auto visible = core::ValueHandle{this->editorObject()}.get("visible").as<bool>();
		if ((this->ramsesObject().getVisibility() == ramses::EVisibilityMode::Visible) != visible) {
			this->ramsesObject().setVisibility(visible ? ramses::EVisibilityMode::Visible : ramses::EVisibilityMode::Invisible);
		}
	}

	void syncRotation() {
		if (Rotation::from(this->editorObject()) != Rotation::from(this->ramsesObject())) {
			Rotation::sync(this->editorObject(), this->ramsesObject());
		}
	}

	void syncTranslation() {
		if (Translation::from(this->editorObject()) != Translation::from(this->ramsesObject())) {
			Translation::sync(this->editorObject(), this->ramsesObject());
		}
	}

	void syncScaling() {
		if (Scaling::from(this->editorObject()) != Scaling::from(this->ramsesObject())) {
			Scaling::sync(this->editorObject(), this->ramsesObject());
		}
	}

	raco::ramses_base::UniqueRamsesNodeBinding nodeBinding_;
	std::array<components::Subscription, 5> subscriptions_;
};

class NodeAdaptor final : public SpatialAdaptor<user_types::Node, ramses::Node> {
public:
	explicit NodeAdaptor(SceneAdaptor* sceneAdaptor, user_types::SNode node);
};

};	// namespace raco::ramses_adaptor
