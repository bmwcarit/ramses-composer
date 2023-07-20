/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/ChangeRecorder.h"
#include "core/EditorObject.h"


#include <algorithm>
#include <iterator>
#include <set>

#include <cassert>

namespace raco::core {

bool DataChangeRecorder::LinkMap::contains(SLink link) const {
	auto linkEndObjIt = linkMap_.find((*link->endObject_)->objectID());
	if (linkEndObjIt != linkMap_.end()) {
		auto desc = link->descriptor();
		auto linkEndObjs = linkEndObjIt->second;
		return linkEndObjs.find(desc) != linkEndObjs.end();
	}
	return false;
}


bool DataChangeRecorder::LinkMap::eraseLink(const LinkDescriptor& link) {
	const auto& linkEndObjId = link.end.object()->objectID();
	auto linkEndObjIt = linkMap_.find(linkEndObjId);
	if (linkEndObjIt != linkMap_.end()) {
		auto& endObjLinks = linkEndObjIt->second;

		auto linkIt = endObjLinks.find(link);
		if (linkIt != endObjLinks.end()) {
			endObjLinks.erase(linkIt);
			if (endObjLinks.empty()) {
				linkMap_.erase(linkEndObjIt);
			}
			return true;
		}
	}

	return false;
}

void DataChangeRecorder::LinkMap::insertLinkEndPointObjects(bool includeLinkStart, bool includeLinkEnd, SEditorObjectSet& objects,
	const SEditorObjectSet& excludeObjects) const {
	for (auto const& [linkEndObjId, links] : linkMap_) {
		if (includeLinkEnd && 
			excludeObjects.find(links.begin()->end.object()) == excludeObjects.end()) {
			objects.insert(links.begin()->end.object());
		}
		if (includeLinkStart) {
			for (const auto& link : links) {
				if (excludeObjects.find(link.start.object()) == excludeObjects.end()) {
					objects.insert(link.start.object());
				}
			}
		}
	}
}

void DataChangeRecorder::LinkMap::insertOrUpdateLink(const LinkDescriptor& link) {
	const auto& linkEndObjId = link.end.object()->objectID();
	auto linkEndObjIt = linkMap_.find(linkEndObjId);
	if (linkEndObjIt == linkMap_.end()) {
		linkMap_[linkEndObjId].insert(link);
	} else {
		auto& addedLinksForEndObj = linkEndObjIt->second;

		auto addedLinkIt = addedLinksForEndObj.find(link);
		if (addedLinkIt != addedLinksForEndObj.end()) {
			auto savedLink = addedLinksForEndObj.extract(addedLinkIt);
			savedLink.value().isValid = link.isValid;
			savedLink.value().isWeak = link.isWeak;
			addedLinksForEndObj.insert(std::move(savedLink));
		} else {
			linkMap_[linkEndObjId].insert(link);
		}
	}
}

bool DataChangeRecorder::LinkMap::updateLinkIfSaved(const LinkDescriptor& link) {
	const auto& linkEndObjId = link.end.object()->objectID();
	auto linkEndObjIt = linkMap_.find(linkEndObjId);
	if (linkEndObjIt != linkMap_.end()) {
		auto& addedLinksForEndObj = linkEndObjIt->second;

		auto addedLinkIt = addedLinksForEndObj.find(link);
		if (addedLinkIt != addedLinksForEndObj.end()) {
			auto savedLink = addedLinksForEndObj.extract(addedLinkIt);
			savedLink.value().isValid = link.isValid;
			savedLink.value().isWeak = link.isWeak;
			addedLinksForEndObj.insert(std::move(savedLink));

			return true;
		}
	}
	return false;
}

void DataChangeRecorder::reset() {
	createdObjects_.clear();
	deletedObjects_.clear();
	changedValues_.clear();
	changedErrors_.clear();
	previewDirty_.clear();
	addedLinks_.clear();
	removedLinks_.clear();
	changedValidityLinks_.clear();
	externalProjectMapChanged_ = false;
	rootOrderChanged_ = false;
}

DataChangeRecorder DataChangeRecorder::release() {
	DataChangeRecorder copy{*this};
	reset();
	return copy;
}

void DataChangeRecorder::recordCreateObject(SEditorObject const& object) {
	createdObjects_.insert(object);
}

void DataChangeRecorder::recordDeleteObject(SEditorObject const& object) {
	// Remove object from created objects
	auto it = createdObjects_.find(object);
	if (it != createdObjects_.end()) {
		createdObjects_.erase(it);
	} else {
		deletedObjects_.insert(object);
	}

	// Remove all value changed items for object
	changedValues_.erase(object->objectID());
}

void DataChangeRecorder::recordValueChanged(ValueHandle const& value) {
	assert(value.isProperty());
	// Remove existing changed values nested inside value
	const auto& objectID = value.rootObject()->objectID();

	auto contIt = changedValues_.find(objectID);
	if (contIt != changedValues_.end()) {
		auto& cont = contIt->second;
		auto it = cont.begin();
		while (it != cont.end()) {
			if (value.contains(*it)) {
				it = cont.erase(it);
			} else if (it->contains(value)) {
				// Discard values nested inside existing change records
				return;
			} else {
				++it;
			}
		}
	}

	changedValues_[objectID].insert(value);
}

void DataChangeRecorder::recordAddLink(const LinkDescriptor& link) {
	addedLinks_.insertOrUpdateLink(link);
}

void DataChangeRecorder::recordChangeValidityOfLink(const LinkDescriptor& link) {
	// edge case: Link is already recorded as added. Don't rerecord link, just modify the added link.
	if (addedLinks_.updateLinkIfSaved(link)) {
		return;
	}

	changedValidityLinks_.insertOrUpdateLink(link);
}

void DataChangeRecorder::recordRemoveLink(const LinkDescriptor& link) {
	// Remove validityChangedLinks entry starting and ending on the same properties
	// There can be multiple invalid links ending on the same property but starting on different properties,
	// so we search for identical start and end points.
	changedValidityLinks_.eraseLink(link);

	// Remove addedLinks entry starting and ending on the same property
	// There can also be multiple added links ending on the same property
	// (e.g. add link -> make link in addedLinks_ invalid -> add another link on different start but same end property)
	// so we also search for identical start and end points.
	if (addedLinks_.eraseLink(link)) {
		// Since add link + remove link = no operation we don't create a remove entry.
		return;
	}

	removedLinks_.insertOrUpdateLink(link);
}

void DataChangeRecorder::recordErrorChanged(const ValueHandle& value) {
	changedErrors_.insert(value);
}

void DataChangeRecorder::recordPreviewDirty(const SEditorObject& object) {
	previewDirty_.insert(object);
}

void DataChangeRecorder::recordExternalProjectMapChanged() {
	externalProjectMapChanged_ = true;
}

void DataChangeRecorder::recordRootOrderChanged() {
	rootOrderChanged_ = true;
}

void DataChangeRecorder::mergeChanges(const DataChangeRecorder& other) {
	for (auto obj : other.createdObjects_) {
		recordCreateObject(obj);
	}
	for (auto obj : other.deletedObjects_) {
		recordDeleteObject(obj);
	}
	for (const auto& [id, cont] : other.changedValues_) {
		for (const auto& handle : cont) {
			recordValueChanged(handle);
		}
	}
	for (auto handle : other.changedErrors_) {
		recordErrorChanged(handle);
	}
	for (auto handle : other.previewDirty_) {
		recordPreviewDirty(handle);
	}
	for (const auto& [linkEndObjId, remLinks] : other.removedLinks_.savedLinks()) {
		for (const auto& remLink : remLinks) {
			recordRemoveLink(remLink);
		}
	}
	for (const auto& [linkEndObjId, addedLinks] : other.addedLinks_.savedLinks()) {
		for (const auto& addedLink : addedLinks) {
			recordAddLink(addedLink);
		}
	}
	for (const auto& [linkEndObjId, changedValidityLinks] : other.changedValidityLinks_.savedLinks()) {
		for (const auto& changedValidityLink : changedValidityLinks) {
			recordChangeValidityOfLink(changedValidityLink);
		}
	}
	if (other.externalProjectMapChanged()) {
		externalProjectMapChanged_ = true;
	}
	if (other.rootOrderChanged()) {
		rootOrderChanged_ = true;
	}
}

SEditorObjectSet const& DataChangeRecorder::getCreatedObjects() const {
	return createdObjects_;
}

SEditorObjectSet const& DataChangeRecorder::getDeletedObjects() const {
	return deletedObjects_;
}

std::map<std::string, std::set<ValueHandle>> const& DataChangeRecorder::getChangedValues() const {
	return changedValues_;
}

bool DataChangeRecorder::hasValueChanged(const ValueHandle& handle) const {
	auto contIt = changedValues_.find(handle.rootObject()->objectID());
	if (contIt != changedValues_.end()) {
		return contIt->second.find(handle) != contIt->second.end();
	}
	return false;
}

std::map<std::string, std::set<LinkDescriptor>> const& DataChangeRecorder::getAddedLinks() const {
	return addedLinks_.savedLinks();
}

std::map<std::string, std::set<LinkDescriptor>> const& DataChangeRecorder::getValidityChangedLinks() const {
	return changedValidityLinks_.savedLinks();
}

std::map<std::string, std::set<LinkDescriptor>> const& DataChangeRecorder::getRemovedLinks() const {
	return removedLinks_.savedLinks();
}

bool DataChangeRecorder::isLinkAdded(SLink link) const {
	return addedLinks_.contains(link);
}

bool DataChangeRecorder::isLinkValidityChanged(SLink link) const {
	return changedValidityLinks_.contains(link);
}

SEditorObjectSet DataChangeRecorder::getAllChangedObjects(bool includePreviewDirty, bool includeLinkStart, bool includeLinkEnd) const {
	SEditorObjectSet objects;
	std::transform(createdObjects_.begin(), createdObjects_.end(), std::inserter(objects, objects.end()),
		[](const ValueHandle& handle) -> SEditorObject {
			return handle.rootObject();
		});
	std::transform(changedValues_.begin(), changedValues_.end(), std::inserter(objects, objects.end()),
		[](const auto& item) -> SEditorObject {
			return item.second.begin()->rootObject();
		});
	if (includePreviewDirty) {
		std::transform(previewDirty_.begin(), previewDirty_.end(), std::inserter(objects, objects.end()),
			[](const ValueHandle& handle) -> SEditorObject {
				return handle.rootObject();
			});
	}

	if (includeLinkStart || includeLinkEnd) {
		addedLinks_.insertLinkEndPointObjects(includeLinkStart, includeLinkEnd, objects, {});
		changedValidityLinks_.insertLinkEndPointObjects(includeLinkStart, includeLinkEnd, objects, {});
		removedLinks_.insertLinkEndPointObjects(includeLinkStart, includeLinkEnd, objects, deletedObjects_);
	}

	return objects;
}

std::set<ValueHandle> DataChangeRecorder::getChangedErrors() const {
	return changedErrors_;
}

SEditorObjectSet DataChangeRecorder::getPreviewDirtyObjects() const {
	return previewDirty_;
}

bool DataChangeRecorder::externalProjectMapChanged() const {
	return externalProjectMapChanged_;
}

bool DataChangeRecorder::rootOrderChanged() const {
	return rootOrderChanged_;
}


void MultiplexedDataChangeRecorder::addRecorder(DataChangeRecorderInterface* recorder) {
	recorders_.push_back(recorder);
}

void MultiplexedDataChangeRecorder::removeRecorder(DataChangeRecorderInterface* recorder) {
	auto it = std::find(recorders_.begin(), recorders_.end(), recorder);
	if (it != recorders_.end()) {
		recorders_.erase(it);
	}
}

void MultiplexedDataChangeRecorder::reset() {
	for (auto recorder : recorders_) {
		recorder->reset();
	}
}

void MultiplexedDataChangeRecorder::recordCreateObject(SEditorObject const& object) {
	for (auto recorder : recorders_) {
		recorder->recordCreateObject(object);
	}
}

void MultiplexedDataChangeRecorder::recordDeleteObject(SEditorObject const& object) {
	for (auto recorder : recorders_) {
		recorder->recordDeleteObject(object);
	}
}

void MultiplexedDataChangeRecorder::recordValueChanged(ValueHandle const& value) {
	for (auto recorder : recorders_) {
		recorder->recordValueChanged(value);
	}
}

void MultiplexedDataChangeRecorder::recordAddLink(const LinkDescriptor& link) {
	for (auto recorder : recorders_) {
		recorder->recordAddLink(link);
	}
}

void MultiplexedDataChangeRecorder::recordChangeValidityOfLink(const LinkDescriptor& link) {
	for (auto recorder : recorders_) {
		recorder->recordChangeValidityOfLink(link);
	}
}

void MultiplexedDataChangeRecorder::recordRemoveLink(const LinkDescriptor& link) {
	for (auto recorder : recorders_) {
		recorder->recordRemoveLink(link);
	}
}
void MultiplexedDataChangeRecorder::recordErrorChanged(ValueHandle const& object) {
	for (auto recorder : recorders_) {
		recorder->recordErrorChanged(object);
	}
}
void MultiplexedDataChangeRecorder::recordPreviewDirty(SEditorObject const& object) {
	for (auto recorder : recorders_) {
		recorder->recordPreviewDirty(object);
	}
}

void MultiplexedDataChangeRecorder::recordExternalProjectMapChanged() {
	for (auto recorder : recorders_) {
		recorder->recordExternalProjectMapChanged();
	}
}

void MultiplexedDataChangeRecorder::recordRootOrderChanged() {
	for (auto recorder : recorders_) {
		recorder->recordRootOrderChanged();
	}
}

}  // namespace raco::core