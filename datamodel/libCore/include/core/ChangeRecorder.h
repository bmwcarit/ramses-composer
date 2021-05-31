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

#include "Handles.h"
#include "Link.h"

#include <set>
#include <vector>

namespace raco::core {

class DataChangeRecorderInterface {
public:
	virtual void reset() = 0;

	virtual void recordCreateObject(SEditorObject const& object) = 0;
	virtual void recordDeleteObject(SEditorObject const& object) = 0;

	virtual void recordValueChanged(ValueHandle const& value) = 0;
	
	virtual void recordAddLink(const LinkDescriptor& link) = 0;
	virtual void recordChangeValidityOfLink(const LinkDescriptor& link) = 0;
	virtual void recordRemoveLink(const LinkDescriptor& link) = 0;

	virtual void recordErrorChanged(ValueHandle const& value) = 0;

	virtual void recordPreviewDirty(SEditorObject const& object) = 0;

	virtual void recordExternalProjectMapChanged() = 0;
};

class DataChangeRecorder : public DataChangeRecorderInterface {
public:
	void reset() override;

	void recordCreateObject(SEditorObject const& object) override;
	void recordDeleteObject(SEditorObject const& object) override;

	void recordValueChanged(ValueHandle const& value) override;

	void recordAddLink(const LinkDescriptor& link) override;
	void recordChangeValidityOfLink(const LinkDescriptor& link) override;
	void recordRemoveLink(const LinkDescriptor& link) override;

	void recordErrorChanged(ValueHandle const& value) override;

	void recordPreviewDirty(SEditorObject const& object) override;
	
	void recordExternalProjectMapChanged() override;

	/**
	 * #reset() with return of all chanages.
	 */
	DataChangeRecorder release();

	std::set<SEditorObject> const& getCreatedObjects() const;
	std::set<SEditorObject> const& getDeletedObjects() const;

	// Get the set of all changes Values
	// - added/removed properties inside Tables will be recorded as change of the Table Value.
	//   No separate add/remove property notification is generated.
	std::set<ValueHandle> const& getChangedValues() const;

	std::vector<LinkDescriptor> const& getAddedLinks() const;
	std::vector<LinkDescriptor> const& getValidityChangedLinks() const;
	std::vector<LinkDescriptor> const& getRemovedLinks() const;

	// Construct set of all objects that have been changed in some way, i.e.
	// that have been created of which contain a changed Value.
	std::set<SEditorObject> getAllChangedObjects(bool includePreviewDirty = false, bool includeLinkStart = false, bool includeLinkEnd = false) const;

	std::set<ValueHandle> getChangedErrors() const;

	std::set<SEditorObject> getPreviewDirtyObjects() const;

	bool externalProjectMapChanged() const;

	void mergeChanges(const DataChangeRecorder& other);

private:
	std::set<SEditorObject> createdObjects_;
	std::set<SEditorObject> deletedObjects_;

	std::set<ValueHandle> changedValues_;

	std::vector<LinkDescriptor> addedLinks_;
	std::vector<LinkDescriptor> changedValidityLinks_;
	std::vector<LinkDescriptor> removedLinks_;

	std::set<ValueHandle> changedErrors_;
	std::set<SEditorObject> previewDirty_;

	bool externalProjectMapChanged_ = false;
};

class MultiplexedDataChangeRecorder : public DataChangeRecorderInterface {
public:
	void addRecorder(DataChangeRecorderInterface* recorder);
	void removeRecorder(DataChangeRecorderInterface* recorder);

	void reset() override;

	void recordCreateObject(SEditorObject const& object) override;
	void recordDeleteObject(SEditorObject const& object) override;

	void recordValueChanged(ValueHandle const& value) override;

	void recordAddLink(const LinkDescriptor& link) override;
	void recordChangeValidityOfLink(const LinkDescriptor& link) override;
	void recordRemoveLink(const LinkDescriptor& link) override;

	void recordErrorChanged(ValueHandle const& value) override;

	void recordPreviewDirty(SEditorObject const& object) override;

	void recordExternalProjectMapChanged() override;

private:
	std::vector<DataChangeRecorderInterface*> recorders_;
};

}  // namespace raco::core