/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Undo.h"

#include "core/ChangeRecorder.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/Link.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"

#include <cassert>

namespace raco::core {

using namespace raco::data_storage;

void updateTableAsArray(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);
void updateTableByName(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);

void updateSingleValue(const ValueBase *src, ValueBase *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	PrimitiveType type = src->type();
	bool changed = false;
	if (type == PrimitiveType::Ref) {
		// assign with translation
		auto translatedSrc = translateRef(src->asRef());
		if (dest->asRef() != translatedSrc) {
			if (auto oldObj = dest->asRef()) {
				if (invokeHandler && destHandle) {
					oldObj->onBeforeRemoveReferenceToThis(destHandle);
				}
			}
			*dest = translatedSrc;
			changed = true;
		}
		dest->copyAnnotationData(*src);
	} else if (type == PrimitiveType::Table) {
		bool srcIsArray = src->query<ArraySemanticAnnotation>();
		bool destIsArray = dest->query<ArraySemanticAnnotation>();
		assert((srcIsArray && destIsArray) || (!srcIsArray && !destIsArray));
		if (srcIsArray) {
			updateTableAsArray(&src->asTable(), &dest->asTable(), destHandle, translateRef, outChanges, invokeHandler);
		} else {
			updateTableByName(&src->asTable(), &dest->asTable(), destHandle, translateRef, outChanges, invokeHandler);
		}
	} else {
		changed = dest->assign(*src);
		// Assume annotation data doesn't contain Ref type properties.
		dest->copyAnnotationData(*src);
	}
	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

// Update of Tables with ArraySemanticAnnotation
// - replace entire Table contents
void updateTableAsArray(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	bool changed = false;
	if (invokeHandler) {
		for (size_t index{0}; index < dest->size(); index++) {
			auto oldValue = dest->get(index);
			if (oldValue->type() == PrimitiveType::Ref) {
				auto oldObj = oldValue->asRef();
				if (oldObj && destHandle) {
					oldObj->onBeforeRemoveReferenceToThis(destHandle[index]);
				}
			}
		}
	}
	if (dest->size() > 0) {
		dest->clear();
		changed = true;
	}

	for (size_t index{0}; index < src->size(); index++) {
		dest->addProperty(src->get(index)->clone(&translateRef));
		changed = true;
	}

	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

// Update of Tables without ArraySemanticAnnotation
// - match properties by name and type and remove/add properties as necessary 
void updateTableByName(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	// Remove dest properties not present in src
	size_t index = 0;
	bool changed = false;
	while (index < dest->size()) {
		std::string name = dest->name(index);
		if (!src->hasProperty(name) || !ValueBase::classesEqual(*src->get(name), *dest->get(name))) {

			auto oldValue = dest->get(index);
			if (oldValue->type() == PrimitiveType::Ref) {
				if (auto oldObj = oldValue->asRef()) {
					if (invokeHandler && destHandle) {
						oldObj->onBeforeRemoveReferenceToThis(destHandle[index]);
					}
				}
			}

			dest->removeProperty(index);
			changed = true;
		} else {
			++index;
		}
	}

	// Add src properties not present in dest
	for (size_t index{0}; index < src->size(); index++) {
		std::string name = src->name(index);
		if (dest->hasProperty(name)) {
			updateSingleValue(src->get(name), dest->get(name), destHandle ? destHandle[index] : ValueHandle(), translateRef, outChanges, invokeHandler);
		} else {
			dest->addProperty(name, src->get(name)->clone(&translateRef));
			changed = true;
		}
	}
	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

void updateEditorObject(const EditorObject *src, SEditorObject dest, translateRefFunc translateRef, excludePropertyPredicateFunc excludeIf, UserObjectFactoryInterface &factory, DataChangeRecorder *outChanges, bool invokeHandler, bool updateObjectAnnotations) {
	if (updateObjectAnnotations) {
		auto destAnnoCopy{dest->annotations()};
		for (const auto &destAnno : destAnnoCopy) {
			if (!src->query(destAnno->serializationTypeName())) {
				dest->removeAnnotation(destAnno);
			}
		}

		for (const auto &srcAnno : src->annotations()) {
			auto typeName = srcAnno->serializationTypeName();
			auto destAnno = dest->query(typeName);
			if (!destAnno) {
				destAnno = factory.createAnnotation(typeName);
				dest->addAnnotation(destAnno);
			}

			for (size_t index = 0; index < srcAnno->size(); index++) {
				std::string name = srcAnno->name(index);
				assert(destAnno->hasProperty(name));
				assert(ValueBase::classesEqual(*destAnno->get(name), *srcAnno->get(name)));

				updateSingleValue(srcAnno->get(name), destAnno->get(name), ValueHandle(), translateRef, outChanges, invokeHandler);
			}
		}
	}

	for (size_t index = 0; index < src->size(); index++) {
		std::string name = src->name(index);
		assert(dest->hasProperty(name));
		if (!excludeIf(name)) {
			assert(ValueBase::classesEqual(*dest->get(name), *src->get(name)));
			
			updateSingleValue(src->get(name), dest->get(name), ValueHandle(dest, {index}), translateRef, outChanges, invokeHandler);
		}
	}
}


void UndoStack::saveProjectState(const Project *src, Project *dest, Project *ref, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory) {
	assert(dest->links().empty());
	assert(dest->linkStartPoints().empty());
	assert(dest->linkEndPoints().empty());
	assert(dest->instances().empty());

	std::set<SEditorObject> dirtyObjects;
	if (ref) {
		dirtyObjects = changes.getAllChangedObjects();
	} else {
		std::copy(src->instances().begin(), src->instances().end(), std::inserter(dirtyObjects, dirtyObjects.end()));
	}

	for (const auto &srcObj : src->instances()) {
		if (!ref || dirtyObjects.find(srcObj) != dirtyObjects.end()) {
			// changed -> create new object
			auto destObj = factory.createObject(srcObj->getTypeDescription().typeName, srcObj->objectName(), srcObj->objectID());
			dest->addInstance(destObj);
		} else {
			// unchanged -> use object from ref
			dest->addInstance(ref->getInstanceByID(srcObj->objectID()));
		}
	}

	auto translateRef = [dest](SEditorObject srcObj) -> SEditorObject {
		if (srcObj) {
			return dest->getInstanceByID(srcObj->objectID());
		}
		return nullptr;
	};

	for (const auto &srcObj : dirtyObjects) {
		auto destObj = dest->getInstanceByID(srcObj->objectID());
		updateEditorObject(
			srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, nullptr, false);
	}

	for (const auto &srcLink : src->links()) {
		dest->addLink(Link::cloneLinkWithTranslation(srcLink, translateRef));
	}

	// Update external project name map
	dest->externalProjectsMap_ = src->externalProjectsMap_;
}

void UndoStack::updateProjectState(const Project *src, Project *dest, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory) {
	std::set<SEditorObject> dirtyObjects = changes.getAllChangedObjects();

	auto translateRef = [dest](SEditorObject srcObj) -> SEditorObject {
		if (srcObj) {
			return dest->getInstanceByID(srcObj->objectID());
		}
		return nullptr;
	};

	for (const auto &srcObj : dirtyObjects) {
		auto destObj = dest->getInstanceByID(srcObj->objectID());
		updateEditorObject(srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, nullptr, false);
	}

	// Update external project name map
	dest->externalProjectsMap_ = src->externalProjectsMap_;
}

void UndoStack::restoreProjectState(Project *src, Project *dest, BaseContext &context, UserObjectFactoryInterface &factory) {
	DataChangeRecorder changes;

	const auto destLinks{dest->links()};
	const auto srcLinks{src->links()};

	// Remove dest links not present in src
	for (const auto &destLink : destLinks) {
		if (!src->findLinkByObjectID(destLink)) {
			changes.recordRemoveLink(destLink->descriptor());
			dest->removeLink(destLink);
		}
	}

	// Remove dest objects not present in src
	std::set<SEditorObject> toRemove;
	for (auto destObj : dest->instances()) {
		if (!src->getInstanceByID(destObj->objectID())) {
			toRemove.insert(destObj);
			changes.recordDeleteObject(destObj);
		}
	}
	BaseContext::deleteWithVolatileSideEffects(dest, toRemove, context.errors());

	// Create src object not present in dest
	for (const auto &srcObj : src->instances()) {
		if (!dest->getInstanceByID(srcObj->objectID())) {
			auto destObj = factory.createObject(srcObj->getTypeDescription().typeName, srcObj->objectName(), srcObj->objectID());
			dest->addInstance(destObj);
			changes.recordCreateObject(destObj);
		}
	}

	auto translateRef = [dest](SEditorObject srcObj) -> SEditorObject {
		if (srcObj) {
			return dest->getInstanceByID(srcObj->objectID());
		}
		return nullptr;
	};

	// Update objects
	for (const auto &destObj : dest->instances()) {
		auto srcObj = src->getInstanceByID(destObj->objectID());
		updateEditorObject(srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, &changes, true);
	}

	for (const auto &srcLink : srcLinks) {
		auto foundDestLink = dest->findLinkByObjectID(srcLink);
		if (!foundDestLink) {
			// Create src link not present in dest
			auto destLink = Link::cloneLinkWithTranslation(srcLink, translateRef);
			dest->addLink(destLink);
			changes.recordAddLink(destLink->descriptor());
		} else if (srcLink->isValid() != foundDestLink->isValid()) {
			// set validity of dest link to validity of src link
			foundDestLink->isValid_ = srcLink->isValid();
			changes.recordChangeValidityOfLink(foundDestLink->descriptor());
		}
	}

	// Update external project name map
	dest->externalProjectsMap_ = src->externalProjectsMap_;

	// Update volatile data for new or changed objects
	for (const auto &destObj : changes.getAllChangedObjects()) {
		destObj->onAfterDeserialization();
	}

	// Use the change recorder in the context from here on
	context_->uiChanges().mergeChanges(changes);
	context_->modelChanges().mergeChanges(changes);

	// Sync from external files for new or changed objects
	for (const auto &destObj : changes.getAllChangedObjects()) {
		destObj->onAfterContextActivated(context);
	}

	std::vector<std::string> stack;
	stack.emplace_back(dest->currentPath());
	context_->updateExternalReferences(stack);
}

UndoStack::UndoStack(BaseContext* context, const Callback& onChange) : context_(context), onChange_ { onChange } {
	auto initialState = &stack_.emplace_back("Initial").state;
	saveProjectState(context_->project(), initialState, nullptr, context_->modelChanges(), *context_->objectFactory());
}

void UndoStack::reset() {
	stack_.clear();
	index_ = 0;
	auto initialState = &stack_.emplace_back("Initial").state;
	context_->modelChanges().reset();
	saveProjectState(context_->project(), initialState, nullptr, context_->modelChanges(), *context_->objectFactory());
	onChange_();
}

void UndoStack::push(const std::string &description, std::string mergeId) {
	stack_.resize(index_ + 1);
	if (!mergeId.empty() && mergeId == stack_.back().mergeId) {
		// mergable -> In-place update of the last stack state
		updateProjectState(context_->project(), &stack_.back().state, context_->modelChanges(), *context_->objectFactory());
		stack_.back().description = description;
	} else {
		// not mergable -> create and fill new state
		auto nextState = &stack_.emplace_back(description, mergeId).state;
		++index_;
		saveProjectState(context_->project(), nextState, &stack_[index_ - 1].state, context_->modelChanges(), *context_->objectFactory());
	}

	onChange_();
	context_->modelChanges().reset();
}

size_t UndoStack::size() const {
	return stack_.size();
}

size_t UndoStack::getIndex() const {
	return index_;
}

size_t UndoStack::setIndex(size_t newIndex, bool force) {
	if (newIndex < size() && (newIndex != index_ || force)) {
		index_ = newIndex;
		try {
			restoreProjectState(&stack_[index_].state, context_->project(), *context_, *context_->objectFactory());
		} catch (ExtrefError &e) {
			context_->modelChanges().reset();
			onChange_();
			throw e;
		}
		context_->modelChanges().reset();
		onChange_();
	}
	return index_;
}

void UndoStack::undo() {
	if (index_ > 0) {
		setIndex(index_ - 1);
	}
}

void UndoStack::redo() {
	if (index_ < size() - 1) {
		setIndex(index_ + 1);
	}
}

UndoStack::Entry::Entry(std::string desc, std::string id) : description(desc), mergeId(id) {
}

const std::string& UndoStack::description(size_t index) const {
	return stack_.at(index).description;
}

bool UndoStack::canUndo() const noexcept {
	return getIndex() > 0;
}

bool UndoStack::canRedo() const noexcept {
	return getIndex() < (size()-1);
}

}  // namespace raco::core