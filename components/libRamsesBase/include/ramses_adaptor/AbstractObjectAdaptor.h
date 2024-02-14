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

#include "data_storage/Value.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"
#include <ramses/framework/RamsesObject.h>
#include <ramses/framework/RamsesVersion.h>
#include <ramses/client/logic/Property.h>

#include <utility>
#include "ramses_base/RamsesFormatter.h"

namespace raco::ramses_adaptor {

class AbstractSceneAdaptor;

using ramses_base::RamsesHandle;


class AbstractISceneObjectProvider {
public:
	virtual RamsesHandle<ramses::Node> sceneObject() = 0;
};

/**
 * Base class for all EditorObject Adaptors
 */
class AbstractObjectAdaptor {
public:
	using SEditorObject = core::SEditorObject;

	explicit AbstractObjectAdaptor(AbstractSceneAdaptor* sceneAdaptor) : sceneAdaptor_{sceneAdaptor}, dirtyStatus_{true} {}
	virtual ~AbstractObjectAdaptor();

	virtual SEditorObject baseEditorObject() noexcept = 0;
	virtual const SEditorObject baseEditorObject() const noexcept = 0; 

	// Return true if externally visible ramses object pointerrs have been changed.
	// Sync is expected to clean the dirty status of the current adaptor object.
	virtual bool sync();

	bool isDirty() const;

	// Dirty objects need to be updated in ramses due to changes in the data model.
	void tagDirty(bool newStatus = true);

protected:
	AbstractSceneAdaptor* sceneAdaptor_;
	bool dirtyStatus_;
};
using UniqueAbstractObjectAdaptor = std::unique_ptr<AbstractObjectAdaptor>;

template<typename EditorType> 
class AbstractUserTypeObjectAdaptor : public AbstractObjectAdaptor {
public:
	AbstractUserTypeObjectAdaptor(AbstractSceneAdaptor* sceneAdaptor, std::shared_ptr<EditorType> editorObject) 
		: AbstractObjectAdaptor(sceneAdaptor), editorObject_(editorObject) {
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
class AbstractTypedObjectAdaptor : public AbstractUserTypeObjectAdaptor<EditorType> {
public:
	AbstractTypedObjectAdaptor(
		AbstractSceneAdaptor* sceneAdaptor,
		std::shared_ptr<EditorType> editorObject,
		RamsesHandle<RamsesType>&& ramsesObject) : AbstractUserTypeObjectAdaptor<EditorType>{sceneAdaptor, editorObject}, 
		ramsesObject_{std::move(ramsesObject)} {
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
	bool sync() override {
		this->tagDirty(false);
		return false;
	}

	void reset(RamsesHandle<RamsesType>&& newRamsesObject) {
		std::swap(ramsesObject_, newRamsesObject);
	}

private:
	RamsesHandle<RamsesType> ramsesObject_;
};

};	// namespace raco::ramses_adaptor
