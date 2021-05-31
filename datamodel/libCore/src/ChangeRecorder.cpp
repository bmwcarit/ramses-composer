/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/ChangeRecorder.h"

#include <algorithm>
#include <iterator>
#include <set>

namespace raco::core {

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
	auto value = changedValues_.begin();
	while (value != changedValues_.end()) {
		if (value->rootObject() == object) {
			value = changedValues_.erase(value);
		} else {
			++value;
		}
	}
}

void DataChangeRecorder::recordValueChanged(ValueHandle const& value) {
	// Remove existing changed values nested inside value
	auto it = changedValues_.begin();
	while (it != changedValues_.end()) {
		if (value.contains(*it)) {
			it = changedValues_.erase(it);
		} else if (it->contains(value)) {
			// Discard values nested inside existing change records
			return;
		} else {
			++it;
		}
	}

	changedValues_.insert(value);
}

void DataChangeRecorder::recordAddLink(const LinkDescriptor& link) {
	auto addedLinksIt = std::find(addedLinks_.begin(), addedLinks_.end(), link);
	if (addedLinksIt == addedLinks_.end()) {
		addedLinks_.emplace_back(link);
	} else {
		addedLinksIt->isValid = link.isValid;
	}
}

void DataChangeRecorder::recordChangeValidityOfLink(const LinkDescriptor& link) {
	// edge case: Link is already recorded as added. Don't rerecord link, just modify the added link.
	auto linkIt = std::find(addedLinks_.begin(), addedLinks_.end(), link);
	if (linkIt != addedLinks_.end()) {
		linkIt->isValid = link.isValid;
		return;
	}

	linkIt = std::find(changedValidityLinks_.begin(), changedValidityLinks_.end(), link);
	if (linkIt == changedValidityLinks_.end()) {
		changedValidityLinks_.emplace_back(link);
	} else {
		linkIt->isValid = link.isValid;
	}
}

void DataChangeRecorder::recordRemoveLink(const LinkDescriptor& link) {
	// Remove validityChangedLinks entry starting and ending on the same properties
	// There can be multiple invalid links ending on the same property but starting on different properties,
	// so we search for identical start and end points.
	auto it = std::find(changedValidityLinks_.begin(), changedValidityLinks_.end(), link);
	if (it != changedValidityLinks_.end()) {
		changedValidityLinks_.erase(it);
	}

	// Remove addedLinks entry starting and ending on the same property
	// There can also be multiple added links ending on the same property
	// (e.g. add link -> make link in addedLinks_ invalid -> add another link on different start but same end property)
	// so we also search for identical start and end points.
	it = std::find(addedLinks_.begin(), addedLinks_.end(), link);
	if (it != addedLinks_.end()) {
		addedLinks_.erase(it);
		// Since add link + remove link = no operation we don't create a remove entry.
		return;
	}

	if (std::find(removedLinks_.begin(), removedLinks_.end(), link) == removedLinks_.end()) {
		removedLinks_.emplace_back(link);
	}
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

void DataChangeRecorder::mergeChanges(const DataChangeRecorder& other) {
	for (auto obj : other.createdObjects_) {
		recordCreateObject(obj);
	}
	for (auto obj : other.deletedObjects_) {
		recordDeleteObject(obj);
	}
	for (auto handle : other.changedValues_) {
		recordValueChanged(handle);
	}
	for (auto handle : other.changedErrors_) {
		recordErrorChanged(handle);
	}
	for (auto handle : other.previewDirty_) {
		recordPreviewDirty(handle);
	}
	for (auto remLink : other.removedLinks_) {
		recordRemoveLink(remLink);
	}
	for (auto link : other.addedLinks_) {
		recordAddLink(link);
	}
	for (auto link : other.changedValidityLinks_) {
		recordChangeValidityOfLink(link);
	}
	if (other.externalProjectMapChanged()) {
		externalProjectMapChanged_ = true;
	}
}

std::set<SEditorObject> const& DataChangeRecorder::getCreatedObjects() const {
	return createdObjects_;
}

std::set<SEditorObject> const& DataChangeRecorder::getDeletedObjects() const {
	return deletedObjects_;
}

std::set<ValueHandle> const& DataChangeRecorder::getChangedValues() const {
	return changedValues_;
}

std::vector<LinkDescriptor> const& DataChangeRecorder::getAddedLinks() const {
	return addedLinks_;
}

std::vector<LinkDescriptor> const& DataChangeRecorder::getValidityChangedLinks() const {
	return changedValidityLinks_;
}

std::vector<LinkDescriptor> const& DataChangeRecorder::getRemovedLinks() const {
	return removedLinks_;
}

std::set<SEditorObject> DataChangeRecorder::getAllChangedObjects(bool includePreviewDirty, bool includeLinkStart, bool includeLinkEnd) const {
	std::set<SEditorObject> objects;
	std::transform(createdObjects_.begin(), createdObjects_.end(), std::inserter(objects, objects.end()),
		[](const ValueHandle& handle) -> SEditorObject {
			return handle.rootObject();
		});
	std::transform(changedValues_.begin(), changedValues_.end(), std::inserter(objects, objects.end()),
		[](const ValueHandle& handle) -> SEditorObject {
			return handle.rootObject();
		});
	if (includePreviewDirty) {
		std::transform(previewDirty_.begin(), previewDirty_.end(), std::inserter(objects, objects.end()),
			[](const ValueHandle& handle) -> SEditorObject {
				return handle.rootObject();
			});
	}

	if (includeLinkStart || includeLinkEnd) {
		for (auto const& link : addedLinks_) {
			if (includeLinkStart) {
				objects.insert(link.start.object());
			}
			if (includeLinkEnd) {
				objects.insert(link.end.object());
			}
		}
		for (auto const& link : changedValidityLinks_) {
			if (includeLinkStart) {
				objects.insert(link.start.object());
			}
			if (includeLinkEnd) {
				objects.insert(link.end.object());
			}
		}
		for (auto const& link : removedLinks_) {
			if (includeLinkStart) {
				objects.insert(link.start.object());
			}
			if (includeLinkEnd) {
				objects.insert(link.end.object());
			}
		}
	}

	return objects;
}

std::set<ValueHandle> DataChangeRecorder::getChangedErrors() const {
	return changedErrors_;
}

std::set<SEditorObject> DataChangeRecorder::getPreviewDirtyObjects() const {
	return previewDirty_;
}

bool DataChangeRecorder::externalProjectMapChanged() const {
	return externalProjectMapChanged_;
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

}  // namespace raco::core