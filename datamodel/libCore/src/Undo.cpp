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
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/Link.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"
#include "VisualCurveData/VisualCurvePosManager.h"
#include "FolderData/FolderDataManager.h"
#include "CurveData/CurveManager.h"
#include "signal/SignalProxy.h"
#include <cassert>

namespace raco::core {
using namespace raco::data_storage;

void UndoHelpers::updateSingleValue(const ValueBase *src, ValueBase *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
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
	} else if (type == PrimitiveType::Struct) {
		assert(ValueBase::classesEqual(*src, *dest));
		updateStruct(&src->asStruct(), &dest->asStruct(), destHandle, translateRef, outChanges, invokeHandler);
		// Assume annotation data doesn't contain Ref type properties.
		dest->copyAnnotationData(*src);
	} else {
		changed = dest->assign(*src);
		// Assume annotation data doesn't contain Ref type properties.
		dest->copyAnnotationData(*src);
	}
	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

void UndoHelpers::callOnBeforeRemoveReferenceHandler(raco::data_storage::Table *dest, const size_t &index, raco::core::ValueHandle &destHandle) {
	auto oldValue = dest->get(index);
	if (oldValue->type() == PrimitiveType::Ref) {
		if (auto oldObj = oldValue->asRef()) {
			oldObj->onBeforeRemoveReferenceToThis(destHandle[index]);
		}
	} else if (hasTypeSubstructure(oldValue->type())) {
		BaseContext::callReferenceToThisHandlerForAllTableEntries<&EditorObject::onBeforeRemoveReferenceToThis>(destHandle);
	}
}

// Update of Tables with ArraySemanticAnnotation
// - replace entire Table contents
void UndoHelpers::updateTableAsArray(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	bool changed = false;
	if (ReflectionInterface::compare(*src, *dest, translateRef)) {
		return;
	}

	if (invokeHandler && destHandle) {
		for (size_t index{0}; index < dest->size(); index++) {
			UndoHelpers::callOnBeforeRemoveReferenceHandler(dest, index, destHandle);
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

void UndoHelpers::updateStruct(const ClassWithReflectedMembers *src, ClassWithReflectedMembers *dest, ValueHandle destHandle,  translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	for (size_t index{0}; index < src->size(); index++) {
		std::string name = src->name(index);
		assert(dest->hasProperty(name));
		UndoHelpers::updateSingleValue(src->get(name), dest->get(name), destHandle ? destHandle[index] : ValueHandle(), translateRef, outChanges, invokeHandler);
	}
}

void UndoHelpers::updateMissingTableProperties(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	bool changed = false;
	for (size_t index{0}; index < src->size(); index++) {
		std::string name = src->name(index);
		if (!dest->hasProperty(name)) {
			dest->addProperty(name, src->get(name)->clone(&translateRef), index);
			changed = true;
		}
	}
	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

// Update of Tables without ArraySemanticAnnotation
// - match properties by name and type and remove/add properties as necessary 
void UndoHelpers::updateTableByName(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler) {
	// Remove dest properties not present in src
	size_t index = 0;
	bool changed = false;
	while (index < dest->size()) {
		std::string name = dest->name(index);
		if (!src->hasProperty(name) || !ValueBase::classesEqual(*src->get(name), *dest->get(name))) {

			if (invokeHandler && destHandle) {
				UndoHelpers::callOnBeforeRemoveReferenceHandler(dest, index, destHandle);
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
			auto destIndex = dest->index(name);
			if (destIndex != index) {
				dest->swapProperties(index, destIndex);
				changed = true;
			}
			UndoHelpers::updateSingleValue(src->get(name), dest->get(name), destHandle ? destHandle[index] : ValueHandle(), translateRef, outChanges, invokeHandler);
		} else {
			dest->addProperty(name, src->get(name)->clone(&translateRef), index);
			changed = true;
		}
	}
	if (changed && outChanges && destHandle) {
		outChanges->recordValueChanged(destHandle);
	}
}

void UndoHelpers::updateEditorObject(const EditorObject *src, SEditorObject dest, translateRefFunc translateRef, excludePropertyPredicateFunc excludeIf, UserObjectFactoryInterface &factory, DataChangeRecorder *outChanges, bool invokeHandler, bool updateObjectAnnotations) {
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

				UndoHelpers::updateSingleValue(srcAnno->get(name), destAnno->get(name), ValueHandle(), translateRef, outChanges, invokeHandler);
			}
		}
	}

	for (size_t index = 0; index < src->size(); index++) {
		std::string name = src->name(index);
		assert(dest->hasProperty(name));
		if (!excludeIf(name)) {
			assert(ValueBase::classesEqual(*dest->get(name), *src->get(name)));
			
			UndoHelpers::updateSingleValue(src->get(name), dest->get(name), ValueHandle(dest, {index}), translateRef, outChanges, invokeHandler);
		}
	}
}

void UndoStack::saveProjectState(const Project *src, Project *dest, Project *ref, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory) {
	assert(dest->links().empty());
	assert(dest->linkStartPoints().empty());
	assert(dest->linkEndPoints().empty());
	assert(dest->instances().empty());

	SEditorObjectSet dirtyObjects;
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
		UndoHelpers::updateEditorObject(
			srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, nullptr, false);
	}

	for (const auto &srcLink : src->links()) {
		if (!ref || changes.isLinkAdded(srcLink) || changes.isLinkValidityChanged(srcLink)) {
			dest->addLink(Link::cloneLinkWithTranslation(srcLink, translateRef));
		} else {
			dest->addLink(ref->findLinkByObjectID(srcLink));
		}
	}

	// Update external project name map
	dest->externalProjectsMap_ = src->externalProjectsMap_;
}

void UndoStack::updateProjectState(const Project *src, Project *dest, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory) {
	SEditorObjectSet dirtyObjects = changes.getAllChangedObjects();

	auto translateRef = [dest](SEditorObject srcObj) -> SEditorObject {
		if (srcObj) {
			return dest->getInstanceByID(srcObj->objectID());
		}
		return nullptr;
	};

	for (const auto &srcObj : dirtyObjects) {
		auto destObj = dest->getInstanceByID(srcObj->objectID());
		UndoHelpers::updateEditorObject(
			srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, nullptr, false);
	}

	// Update external project name map
	dest->externalProjectsMap_ = src->externalProjectsMap_;
}

void UndoStack::restoreProjectState(Project *src, Project *dest, BaseContext &context, UserObjectFactoryInterface &factory) {
	DataChangeRecorder changes;
	bool extrefDirty = false;

	const auto destLinks{dest->links()};
	const auto srcLinks{src->links()};

	// Remove dest links not present in src
	for (const auto &destLink : destLinks) {
		if (!src->findLinkByObjectID(destLink)) {
			changes.recordRemoveLink(destLink->descriptor());
			dest->removeLink(destLink);
			extrefDirty = extrefDirty || (*destLink->endObject_)->query<ExternalReferenceAnnotation>();
		}
	}

	// Remove dest objects not present in src
	SEditorObjectSet toRemove;
	for (auto destObj : dest->instances()) {
		if (!src->getInstanceByID(destObj->objectID())) {
			toRemove.insert(destObj);
			changes.recordDeleteObject(destObj);
			extrefDirty = extrefDirty || destObj->query<ExternalReferenceAnnotation>();
		}
	}
	BaseContext::deleteWithVolatileSideEffects(dest, toRemove, context.errors());

	// Create src object not present in dest
	for (const auto &srcObj : src->instances()) {
		if (!dest->getInstanceByID(srcObj->objectID())) {
			auto destObj = factory.createObject(srcObj->getTypeDescription().typeName, srcObj->objectName(), srcObj->objectID());
			dest->addInstance(destObj);
			changes.recordCreateObject(destObj);
			extrefDirty = extrefDirty || srcObj->query<ExternalReferenceAnnotation>();
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
		UndoHelpers::updateEditorObject(
			srcObj.get(), destObj, translateRef, [](const std::string &) { return false; }, factory, &changes, true);
	}

	auto findExtref = [](const std::map<std::string, std::set<ValueHandle>>& changes) {
		for (const auto &[id, handles] : changes) {
			for (const auto &handle : handles) {
				if (handle.rootObject()->query<ExternalReferenceAnnotation>()) {
					return true;
				}
			}
		}
		return false;
	};
	extrefDirty = extrefDirty || findExtref(changes.getChangedValues());

	for (const auto &srcLink : srcLinks) {
		auto foundDestLink = dest->findLinkByObjectID(srcLink);
		if (!foundDestLink) {
			// Create src link not present in dest
			auto destLink = Link::cloneLinkWithTranslation(srcLink, translateRef);
			dest->addLink(destLink);
			changes.recordAddLink(destLink->descriptor());
			extrefDirty = extrefDirty || (*srcLink->endObject_)->query<ExternalReferenceAnnotation>();
		} else if (srcLink->isValid() != foundDestLink->isValid()) {
			// set validity of dest link to validity of src link
			foundDestLink->isValid_ = srcLink->isValid();
			changes.recordChangeValidityOfLink(foundDestLink->descriptor());
			extrefDirty = extrefDirty || (*srcLink->endObject_)->query<ExternalReferenceAnnotation>();
		}
	}

	// Update external project name map
	if (dest->externalProjectsMap_ != src->externalProjectsMap_) {
		dest->externalProjectsMap_ = src->externalProjectsMap_;
		extrefDirty = true;
	}

	// Update volatile data for new or changed objects
    for (const auto &destObj : changes.getAllChangedObjects()) {
        destObj->onAfterDeserialization();
    }

    // Use the change recorder in the context from here on
    context_->uiChanges().mergeChanges(changes);

	// Reset model changes here to make sure the next undo stack push will see 
    // all changes relative to the last undo stack entry
    context_->modelChanges().reset();

	// Sync from external files for new or changed objects.
	// Also update broken link error messages in Node::onAfterContextActivated, so we need to include
	//   the link endpoints in the changed object set.
    auto changedObjects = changes.getAllChangedObjects(false, false, true);
    context_->performExternalFileReload({changedObjects.begin(), changedObjects.end()});

    if (extrefDirty) {
		std::vector<std::string> stack;
		stack.emplace_back(dest->currentPath());
		// TODO needed to remove model change reset here since this allows the undo stack to get into inconsistent state.
		// See commment above.
		// Reset model changes here to make sure that the prefab update which runs at the end of the external reference update
		// will only see the changed external reference objects and will not perform unnecessary prefab updates.
		//context_->modelChanges().reset();
		try {
			context_->updateExternalReferences(stack);
		} catch (ExtrefError &e) {
			// Do nothing here:
			// updateExternalReferences will create an error message that will be shown in the Error View in addition
			// to throwing this exception.
		}
    }
}

void UndoStack::restoreAnimationState(UndoState state) {
    QVariant curveData;
    curveData.setValue(state.curveData().list);
    guiData::CurveManager::GetInstance().merge(curveData);

    QVariant folderData;
    folderData.setValue(state.folderData().folder);
    FolderDataManager::GetInstance().merge(folderData);

    QVariant posData;
    posData.setValue(state.visualPosData().pos);
    guiData::VisualCurvePosManager::GetInstance().merge(posData);
    Q_EMIT raco::signal::signalProxy::GetInstance().sigRepaintAfterUndoOpreation();
}

UndoStack::UndoStack(BaseContext* context, const Callback& onChange) : context_(context), onChange_ { onChange } {
	auto initialState = &stack_.emplace_back(new Entry("Initial"))->state;
    saveProjectState(context_->project(), initialState, nullptr, context_->modelChanges(), *context_->objectFactory());
}

void UndoStack::reset() {
	stack_.clear();
    index_ = 0;
	auto initialState = &stack_.emplace_back(new Entry("Initial"))->state;
	context_->modelChanges().reset();
	saveProjectState(context_->project(), initialState, nullptr, context_->modelChanges(), *context_->objectFactory());

    raco::core::UndoState undoState;
    undoState.push(FolderDataManager::GetInstance().converFolderData());
    undoState.push(raco::guiData::CurveManager::GetInstance().convertCurveData());
    stack_.back()->undoState = undoState;
    onChange_();
}

void UndoStack::resetUndoState(STRUCT_VISUAL_CURVE_POS pos) {
    stack_.front()->undoState.push(pos);
}

bool UndoStack::canMerge(const DataChangeRecorder &changes) {
	return changes.getCreatedObjects().empty() && changes.getDeletedObjects().empty() && changes.getAddedLinks().empty() && changes.getRemovedLinks().empty() && changes.getValidityChangedLinks().empty() && !changes.externalProjectMapChanged();
}

void UndoStack::push(const std::string &description, std::string mergeId) {
	stack_.resize(index_ + 1);
	if (!mergeId.empty() && mergeId == stack_.back()->mergeId && canMerge(context_->modelChanges())) {
		// mergable -> In-place update of the last stack state
		updateProjectState(context_->project(), &stack_.back()->state, context_->modelChanges(), *context_->objectFactory());
		stack_.back()->description = description;
	} else {
		// not mergable -> create and fill new state
        UndoState undoState = stack_.back()->undoState;
		auto nextState = &stack_.emplace_back(new Entry(description, mergeId))->state;
        stack_.back()->undoState = undoState;
        ++index_;
        saveProjectState(context_->project(), nextState, &stack_[index_ - 1]->state, context_->modelChanges(), *context_->objectFactory());
	}

	onChange_();
    context_->modelChanges().reset();
}

void UndoStack::push(const std::string &description, UndoState state) {
    if (description == stack_.back()->description) {
        return;
    }
    Entry *entry = new Entry(description);
    entry->state = stack_.back()->state;
    stack_.resize(index_ + 1);
    entry->undoState = state;
    stack_.emplace_back(std::unique_ptr<Entry>(entry));
    ++index_;
    onChange_();
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
        restoreProjectState(&stack_[index_]->state, context_->project(), *context_, *context_->objectFactory());
        restoreAnimationState(stack_[index_]->undoState);
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
	return stack_.at(index)->description;
}

bool UndoStack::canUndo() const noexcept {
	return getIndex() > 0;
}

bool UndoStack::canRedo() const noexcept {
    return getIndex() < (size()-1);
}

}  // namespace raco::core
