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

#include "core/Context.h"

#include <functional>
#include <memory>
#include <vector>

namespace raco::components {

class BaseListener {
public:
	virtual ~BaseListener() {}
};
class DataChangeDispatcher;

class Subscription {
	friend class DataChangeDispatcher;

public:
	Subscription(const Subscription&) = delete;
	Subscription& operator=(const Subscription&) = delete;
	explicit Subscription() = default;
	/** Container only subscription which will destroy all subscriptions when it gets destroyed */
	explicit Subscription(std::vector<Subscription>&& subSubscriptions);

	using DeregisterCallback = std::function<void()>;
	explicit Subscription(DataChangeDispatcher* owner, const std::shared_ptr<BaseListener>& listener, DeregisterCallback callback);

	~Subscription();
	Subscription(Subscription&& other) {
		std::swap(other.dataChangeDispatcher_, dataChangeDispatcher_);
		std::swap(other.listener_, listener_);
		std::swap(other.subSubscriptions_, subSubscriptions_);
		std::swap(other.deregisterFunc_, deregisterFunc_);
	}
	Subscription& operator=(Subscription&& other) {
		std::swap(other.dataChangeDispatcher_, dataChangeDispatcher_);
		std::swap(other.listener_, listener_);
		std::swap(other.subSubscriptions_, subSubscriptions_);
		std::swap(other.deregisterFunc_, deregisterFunc_);
		return *this;
	}

private:
	DataChangeDispatcher* dataChangeDispatcher_{nullptr};
	std::shared_ptr<BaseListener> listener_{nullptr};
	std::vector<Subscription> subSubscriptions_{};
	DeregisterCallback deregisterFunc_;
};

class LinkLifecycleListener;
class LinkListener;
class ObjectLifecycleListener;
class EditorObjectListener;
class PropertyChangeListener;
class ValueHandleListener;
class UndoListener;
class ChildrenListener;

class DataChangeDispatcher {
	DataChangeDispatcher(const DataChangeDispatcher&) = delete;
	DataChangeDispatcher& operator=(const DataChangeDispatcher&) = delete;

public:
	using Callback = std::function<void()>;
	using EditorObjectCallback = std::function<void(core::SEditorObject)>;
	using ValueHandleCallback = std::function<void(const core::ValueHandle&)>;
	using BulkChangeCallback = std::function<void(const core::SEditorObjectSet&)>;
	using LinkCallback = std::function<void(const core::LinkDescriptor&)>;

	explicit DataChangeDispatcher();

	Subscription registerOn(core::ValueHandle valueHandle, Callback callback) noexcept;
	Subscription registerOn(core::ValueHandles handles, ValueHandleCallback callback) noexcept;
	Subscription registerOnChildren(core::ValueHandle valueHandle, ValueHandleCallback callback) noexcept;
	Subscription registerOnPropertyChange(const std::string& propertyName, ValueHandleCallback callback) noexcept;
	Subscription registerOnObjectsLifeCycle(EditorObjectCallback onCreation, EditorObjectCallback onDeletion) noexcept;

	/**
	 * @brief Subscribe to link lifecycle changes, i.e. link creation and deletion, for all links.
	 *		
	 * @param onCreation Callback to be invoked when a link is created.
	 * @param onDeletion Callback to be invoked when a link is removed.
	 * @return Subscription object. 
	*/
	Subscription registerOnLinksLifeCycle(LinkCallback onCreation, LinkCallback onDeletion) noexcept;

	/**
	 * @brief Subscribe to link lifecycle changes, i.e. link creation and deletion, for links ending on an object.
	 *		
	 * @param endObject  Restrict notifications to links ending on this object. Must not be nullptr.
	 * @param onCreation Callback to be invoked when a link is created.
	 * @param onDeletion Callback to be invoked when a link is removed.
	 * @return Subscription object. 
	*/
	Subscription registerOnLinksLifeCycleForEnd(core::SEditorObject endObject, LinkCallback onCreation, LinkCallback onDeletion) noexcept;

	/**
	 * @brief Subscribe to link lifecycle changes, i.e. link creation and deletion, for links starting on an object. 
	 *		
	 * @param startObject Restrict notifications to links starting on this object. Must not be nullptr.
	 * @param onCreation Callback to be invoked when a link is created.
	 * @param onDeletion Callback to be invoked when a link is removed.
	 * @return Subscription object. 
	*/
	Subscription registerOnLinksLifeCycleForStart(core::SEditorObject startObject, LinkCallback onCreation, LinkCallback onDeletion) noexcept;

	Subscription registerOnLinkValidityChange(LinkCallback callback) noexcept;
	Subscription registerOnErrorChanged(core::ValueHandle valueHandle, Callback callback) noexcept;
	Subscription registerOnErrorChangedInScene(Callback callback) noexcept;
	Subscription registerOnPreviewDirty(core::SEditorObject obj, Callback callback) noexcept;
	Subscription registerOnUndoChanged(Callback callback) noexcept;

	Subscription registerOnExternalProjectChanged(Callback callback) noexcept;
	Subscription registerOnExternalProjectMapChanged(Callback callback) noexcept;

	Subscription registerOnRootOrderChanged(Callback callback) noexcept;

	// This will regisiter a callback which is invoked by dispatch() after all other changes have been dispatched.
	Subscription registerOnAfterDispatch(Callback callback);

	void registerBulkChangeCallback(BulkChangeCallback callback);
	void resetBulkChangeCallback();

	void dispatch(const core::DataChangeRecorder& dataChanges);

	void assertEmpty();

	void setUndoChanged() {
		undoChanged_ = true;
	}

	void setExternalProjectChanged() {
		externalProjectChanged_ = true;
	}

private:
	void emitUpdateFor(const std::map<std::string, std::set<core::ValueHandle>>& valueHandles) const;
	void emitErrorChanged(const core::ValueHandle& valueHandle) const;
	void emitErrorChangedInScene() const;
	void emitCreated(core::SEditorObject obj) const;
	void emitDeleted(core::SEditorObject obj) const;
	void emitPreviewDirty(core::SEditorObject obj) const;
	void emitBulkChange(const core::SEditorObjectSet& changedObjects) const;
	void emitLinksValidityChanged(std::map<std::string, std::set<core::LinkDescriptor>> const& validityChangedLinks) const;
	void emitLinksAdded(std::map<std::string, std::set<core::LinkDescriptor>> const& addedLinks) const;
	void emitLinksRemoved(std::map<std::string, std::set<core::LinkDescriptor>> const& removedLinks) const;

	std::set<std::weak_ptr<ObjectLifecycleListener>, std::owner_less<std::weak_ptr<ObjectLifecycleListener>>> objectLifecycleListeners_{};
	std::set<std::weak_ptr<LinkLifecycleListener>, std::owner_less<std::weak_ptr<LinkLifecycleListener>>> linkLifecycleListeners_{};
	std::map<std::string, std::set<std::weak_ptr<LinkLifecycleListener>, std::owner_less<std::weak_ptr<LinkLifecycleListener>>>> linkLifecycleListenersForEnd_{};
	std::map<std::string, std::set<std::weak_ptr<LinkLifecycleListener>, std::owner_less<std::weak_ptr<LinkLifecycleListener>>>> linkLifecycleListenersForStart_{};
	std::set<std::weak_ptr<LinkListener>, std::owner_less<std::weak_ptr<LinkListener>>> linkValidityChangeListeners_{};
	std::map<std::string, std::set<std::weak_ptr<ValueHandleListener>, std::owner_less<std::weak_ptr<ValueHandleListener>>>> listeners_{};
	std::map<std::string, std::set<std::weak_ptr<ChildrenListener>, std::owner_less<std::weak_ptr<ChildrenListener>>>> childrenListeners_{};
	std::set<std::weak_ptr<EditorObjectListener>, std::owner_less<std::weak_ptr<EditorObjectListener>>> previewDirtyListeners_{};
	std::set<std::weak_ptr<ValueHandleListener>, std::owner_less<std::weak_ptr<ValueHandleListener>>> errorChangedListeners_{};
	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> errorChangedInSceneListeners_{};
	std::map<std::string, std::set<std::weak_ptr<PropertyChangeListener>, std::owner_less<std::weak_ptr<PropertyChangeListener>>>> propertyChangeListeners_{};

	bool undoChanged_{false};
	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> undoChangeListeners_{};

	bool externalProjectChanged_{false};
	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> externalProjectChangedListeners_{};
	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> externalProjectMapChangedListeners_{};

	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> rootOrderChangedListeners_{};

	std::set<std::weak_ptr<UndoListener>, std::owner_less<std::weak_ptr<UndoListener>>> onAfterDispatchListeners_{};

	BulkChangeCallback bulkChangeCallback_;
};

using SDataChangeDispatcher = std::shared_ptr<DataChangeDispatcher>;

}  // namespace raco::components
