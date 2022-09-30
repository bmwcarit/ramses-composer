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

#include "core/Errors.h"
#include "data_storage/Value.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include <ramses-client-api/RamsesObject.h>
#include <ramses-framework-api/RamsesVersion.h>
#include <ramses-logic/Property.h>

#include <utility>
#include "ramses_base/RamsesFormatter.h"

namespace raco::ramses_adaptor {

using raco::ramses_base::RamsesHandle;

class ILogicPropertyProvider {
public:
	virtual void getLogicNodes(std::vector<rlogic::LogicNode*>& logicNodes) const = 0;
	virtual const rlogic::Property* getProperty(const std::vector<std::string>& propertyNamesVector) = 0;
	virtual void onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) = 0;

	static const rlogic::Property* getPropertyRecursive(const rlogic::Property* property, const std::vector<std::string>& propertyNames, size_t startIndex = 0) {
		for (size_t index = startIndex; index < propertyNames.size(); index++) {
			if (property->getType() == rlogic::EPropertyType::Array) {
				// convert 1-bases Editor index back to 0-based index
				property = property->getChild(std::stoi(propertyNames[index]) - 1);
			} else {
				property = property->getChild(propertyNames[index]);
			}
		}
		return property;
	}
};


class ISceneObjectProvider {
public:
	virtual RamsesHandle<ramses::Node> sceneObject() = 0;
};

struct ExportInformation {
	std::string type;
	const std::string name;

	explicit ExportInformation(std::string type, std::string name) : type(std::move(type)), name(std::move(name)) {}
	explicit ExportInformation(const ramses::ERamsesObjectType type, std::string name) : name(std::move(name)) {
		this->type = fmt::format("{}", type);
	}
};

/**
 * Base class for all EditorObject Adaptors
 */
class ObjectAdaptor {
public:
	using SEditorObject = raco::core::SEditorObject;

	explicit ObjectAdaptor(SceneAdaptor* sceneAdaptor) : sceneAdaptor_{sceneAdaptor}, dirtyStatus_{true} {}
	virtual ~ObjectAdaptor();

	virtual SEditorObject baseEditorObject() noexcept = 0;
	virtual const SEditorObject baseEditorObject() const noexcept = 0; 

	// Return true if externally visible ramses object pointerrs have been changed.
	// Sync is expected to clean the dirty status of the current adaptor object.
	virtual bool sync(core::Errors* errors);

	bool isDirty() const;

	// Dirty objects need to be updated in ramses due to changes in the data model.
	void tagDirty(bool newStatus = true);

	virtual std::vector<ExportInformation> getExportInformation() const = 0;

protected:
	SceneAdaptor* sceneAdaptor_;
	bool dirtyStatus_;
};
using UniqueObjectAdaptor = std::unique_ptr<ObjectAdaptor>;

template<typename EditorType> 
class UserTypeObjectAdaptor : public ObjectAdaptor {
public:
	UserTypeObjectAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<EditorType> editorObject) 
		: ObjectAdaptor(sceneAdaptor), editorObject_(editorObject) {
	}
	
	SEditorObject baseEditorObject() noexcept override {
		return editorObject_;
	}

	const SEditorObject baseEditorObject() const noexcept override {
		return editorObject_;
	}

	
	std::shared_ptr<EditorType> editorObject() noexcept {
		return editorObject_;
	}

	const std::shared_ptr<const EditorType>& editorObject() const noexcept {
		return editorObject_;
	}

protected:
	std::shared_ptr<EditorType> editorObject_;
};

template <typename EditorType, typename RamsesType>
class TypedObjectAdaptor : public UserTypeObjectAdaptor<EditorType> {
public:
	TypedObjectAdaptor(
		SceneAdaptor* sceneAdaptor,
		std::shared_ptr<EditorType> editorObject,
		RamsesHandle<RamsesType>&& ramsesObject) : UserTypeObjectAdaptor<EditorType>{sceneAdaptor, editorObject}, 
		ramsesObject_{std::move(ramsesObject)}, 
		nameSubscription_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("objectName"), [this]() { this->tagDirty(); 
		})} 
	{
		syncName();
	}
	
	RamsesType& ramsesObject() noexcept { 
		return *ramsesObject_.get(); 
	}
	const RamsesType& ramsesObject() const noexcept { 
		return *ramsesObject_.get(); 
	}	
	RamsesHandle<RamsesType> getRamsesObjectPointer() const {
		return ramsesObject_; 
	}
	void resetRamsesObject() { 
		ramsesObject_.reset(); 
	}

protected:
	void syncName() {
		if (ramsesObject_ && ramsesObject_->getName() != this->editorObject()->objectName().c_str()) {
			ramsesObject_->setName(this->editorObject()->objectName().c_str());
		}
	}
	
	bool sync(core::Errors* errors) override {
		syncName();
		this->tagDirty(false);
		return false;
	}

	void reset(RamsesHandle<RamsesType>&& newRamsesObject) {
		std::swap(ramsesObject_, newRamsesObject);
		syncName();
	}

private:
	RamsesHandle<RamsesType> ramsesObject_;

	components::Subscription nameSubscription_;
};

};	// namespace raco::ramses_adaptor
