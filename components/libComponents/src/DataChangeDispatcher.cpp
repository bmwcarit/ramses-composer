/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/DataChangeDispatcher.h"

#include "components/DebugInstanceCounter.h"

#include "core/EditorObject.h"

#include "log_system/log.h"

#include <algorithm>

namespace raco::components {

using namespace raco::core;

class UndoListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(UndoListener);

public:
	using Callback = std::function<void()>;
	explicit UndoListener(Callback callback) : callback_{callback} {}
	void call() const noexcept {
		callback_();
	}

private:
	Callback callback_;
};

class ValueHandleListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(ValueHandleListener);

public:
	using Callback = DataChangeDispatcher::Callback;
	explicit ValueHandleListener(ValueHandle valueHandle, Callback callback) : valueHandle_{std::move(valueHandle)}, callback_{std::move(callback)} {
		assert(static_cast<bool>(valueHandle_));
	}
	void call() const noexcept {
		callback_();
	}
	const ValueHandle& valueHandle() const noexcept {
		return valueHandle_;
	}

private:
	ValueHandle valueHandle_;
	Callback callback_;
};

class ChildrenListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(ChildrenListener);

public:
	using Callback = DataChangeDispatcher::ValueHandleCallback;
	explicit ChildrenListener(ValueHandle valueHandle, Callback callback) : valueHandle_(std::move(valueHandle)), callback_(std::move(callback)) {
		assert(static_cast<bool>(valueHandle_));
	}
	void call(const ValueHandle& handle) const noexcept {
		callback_(handle);
	}
	const ValueHandle& valueHandle() const noexcept {
		return valueHandle_;
	}

private:
	ValueHandle valueHandle_;
	Callback callback_;
};

class EditorObjectListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(EditorObjectListener);

public:
	using Callback = DataChangeDispatcher::Callback;
	explicit EditorObjectListener(SEditorObject object, Callback callback) : object_{std::move(object)}, callback_{std::move(callback)} {}
	void call() const noexcept {
		callback_();
	}
	const SEditorObject& editorObject() const noexcept {
		return object_;
	}

private:
	SEditorObject object_;
	Callback callback_;
};

class PropertyChangeListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(PropertyChangeListener);

public:
	using Callback = DataChangeDispatcher::ValueHandleCallback;
	explicit PropertyChangeListener(const std::string& propertyName, Callback callback) : propertyName_(propertyName), callback_(callback) {}
	void call(const ValueHandle& handle) const noexcept {
		callback_(handle);
	}
	const std::string& property() const noexcept {
		return propertyName_;
	}

private:
	std::string propertyName_;
	Callback callback_;
};

class ObjectLifecycleListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(ObjectLifecycleListener);

public:
	using Callback = DataChangeDispatcher::EditorObjectCallback;
	explicit ObjectLifecycleListener(Callback onCreation, Callback onDeletion) : onCreation_{onCreation}, onDeletion_{onDeletion} {}
	void onCreation(SEditorObject obj) const {
		onCreation_(obj);
	}
	void onDeletion(SEditorObject obj) const {
		onDeletion_(obj);
	}

private:
	Callback onCreation_;
	Callback onDeletion_;
};

class LinkListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(LinkLifecycleListener);

public:
	explicit LinkListener(DataChangeDispatcher::LinkCallback callback) : callback_{callback} {}
	void onLinkChange(const LinkDescriptor& linkDesc) {
		callback_(linkDesc);
	}

private:
	DataChangeDispatcher::LinkCallback callback_;
};

class LinkLifecycleListener final : public BaseListener {
	DEBUG_INSTANCE_COUNTER(LinkLifecycleListener);

public:
	explicit LinkLifecycleListener(DataChangeDispatcher::LinkCallback onCreation, DataChangeDispatcher::LinkCallback onDeletion) : onCreation_{onCreation}, onDeletion_{onDeletion} {}
	DataChangeDispatcher::LinkCallback onCreation_;
	DataChangeDispatcher::LinkCallback onDeletion_;
};

Subscription::Subscription(DataChangeDispatcher* owner, const std::shared_ptr<BaseListener>& listener, DeregisterCallback callback)
	: dataChangeDispatcher_{owner}, listener_{listener}, deregisterFunc_(callback) {}

Subscription::Subscription(std::vector<Subscription>&& subSubscriptions)
	: dataChangeDispatcher_{nullptr}, listener_{nullptr}, subSubscriptions_{std::move(subSubscriptions)} {}

Subscription::~Subscription() {
	if (dataChangeDispatcher_) {
		assert(deregisterFunc_);
		deregisterFunc_();
	}
}


DataChangeDispatcher::DataChangeDispatcher() {}

void DataChangeDispatcher::dispatch(const DataChangeRecorder& dataChanges) {
	// Sync with and reset change recorder:

	for (auto& [endObjId, links] : dataChanges.getValidityChangedLinks()) {
		for (auto& link : links) {
			auto copy{linkValidityChangeListeners_};
			for (const auto& ptr : copy) {
				if (!ptr.expired()) {
					auto listener{ptr.lock()};
					listener->onLinkChange(link);
				}
			}
		}
	}

	for (auto& [endObjId, links] : dataChanges.getRemovedLinks()) {
		auto copy{linkLifecycleListeners_[endObjId]};
		std::copy(linkLifecycleListeners_[std::string()].begin(), linkLifecycleListeners_[std::string()].end(), std::inserter(copy, copy.end()));
		for (auto& link : links) {
			for (const auto& ptr : copy) {
				if (!ptr.expired()) {
					auto listener{ptr.lock()};
					listener->onDeletion_(link);
				}
			}
		}
	}

	// generic dispatcher code does
	// - register links
	// - set dirty flag


	for (auto& createdObject : dataChanges.getCreatedObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitCreated {}", createdObject->objectName());
		emitCreated(createdObject);
	}

	emitUpdateFor(dataChanges.getChangedValues());

	auto changedErrors = dataChanges.getChangedErrors();
	for (auto& changedValueHandle : changedErrors) {
		LOG_TRACE_IF(log_system::DATA_CHANGE, changedValueHandle  && !changedValueHandle.isObject(), "emit errorChanged for Property {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.getPropName());
		LOG_TRACE_IF(log_system::DATA_CHANGE, changedValueHandle && changedValueHandle.isObject(), "emit errorChanged for Object {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.rootObject()->objectID());
		LOG_TRACE_IF(log_system::DATA_CHANGE, !changedValueHandle, "emit errorChanged for project-global ValueHandle");
		emitErrorChanged(changedValueHandle);
	}

	if (!changedErrors.empty()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit errorChangedInScene");
		emitErrorChangedInScene();
	}


	for (auto& object : dataChanges.getPreviewDirtyObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitPreviewDirty {}", object->objectName());
		emitPreviewDirty(object);
	}

	for (auto& [endObjId, links] : dataChanges.getAddedLinks()) {
		auto copy{linkLifecycleListeners_[endObjId]};
		std::copy(linkLifecycleListeners_[std::string()].begin(), linkLifecycleListeners_[std::string()].end(), std::inserter(copy, copy.end()));
		for (auto& link : links) {
			for (const auto& ptr : copy) {
				if (!ptr.expired()) {
					auto listener{ptr.lock()};
					listener->onCreation_(link);
				}
			}
		}
	}

	// Bulk update notification will be used by the SceneAdaptor to perform the actual engine update.
	emitBulkChange(dataChanges.getAllChangedObjects(true));

	for (auto& deletedObject : dataChanges.getDeletedObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitDeleted {}", deletedObject->objectName());
		emitDeleted(deletedObject);
	}

	if (undoChanged_) {
		for (auto& undoListener : undoChangeListeners_) {
			if (!undoListener.expired())
				undoListener.lock()->call();
		}
		undoChanged_ = false;
	}

	if (externalProjectChanged_) {
		for (auto& listener : externalProjectChangedListeners_) {
			if (!listener.expired())
				listener.lock()->call();
		}
		externalProjectChanged_ = false;
	}

	if (dataChanges.externalProjectMapChanged()) {
		for (auto& listener : externalProjectMapChangedListeners_) {
			if (!listener.expired())
				listener.lock()->call();
		}
	}

	for (auto& listener : onAfterDispatchListeners_) {
		if (!listener.expired()) {
			listener.lock()->call();
		}
	}
}

Subscription DataChangeDispatcher::registerOn(ValueHandle valueHandle, Callback callback) noexcept {
	auto listener{std::make_shared<ValueHandleListener>(std::move(valueHandle), std::move(callback))};
	listeners_[listener->valueHandle().rootObject()->objectID()].insert(listener);
	return Subscription{
		this, listener, [this, listener]() {
			const auto& objectID = listener->valueHandle().rootObject()->objectID();
			auto it = listeners_.find(objectID);
			it->second.erase(listener);
			if (it->second.empty()) {
				listeners_.erase(objectID);
			}
		}};
}

Subscription DataChangeDispatcher::registerOn(ValueHandles handles, ValueHandleCallback callback) noexcept {
	std::vector<Subscription> childSubscriptions{};
	childSubscriptions.reserve(handles.size());
	for (int i = 0; i < handles.size(); i++) {
		childSubscriptions.push_back(registerOnChildren(handles[i], callback));
	}
	return Subscription{std::move(childSubscriptions)};
}

Subscription DataChangeDispatcher::registerOnPropertyChange(const std::string& propertyName, ValueHandleCallback callback) noexcept {
	auto listener{std::make_shared<PropertyChangeListener>(propertyName, callback)};
	propertyChangeListeners_[propertyName].insert(listener);
	return Subscription{
		this, listener, [this, propertyName, listener]() {
			auto it = propertyChangeListeners_.find(propertyName);
			it->second.erase(listener);
			if (it->second.empty()) {
				propertyChangeListeners_.erase(propertyName);
			}
		}};
}

Subscription DataChangeDispatcher::registerOnChildren(ValueHandle valueHandle, ValueHandleCallback callback) noexcept {
	auto listener{std::make_shared<ChildrenListener>(std::move(valueHandle), std::move(callback))};
	childrenListeners_[listener->valueHandle().rootObject()->objectID()].insert(listener);
	return Subscription{
		this, listener, [this, listener]() {
			const auto& objectID = listener->valueHandle().rootObject()->objectID();
			auto it = childrenListeners_.find(objectID);
			it->second.erase(listener);
			if (it->second.empty()) {
				childrenListeners_.erase(objectID);
			}
		}};
}


Subscription DataChangeDispatcher::registerOnObjectsLifeCycle(EditorObjectCallback onCreation, EditorObjectCallback onDeletion) noexcept {
	auto listener{std::make_shared<ObjectLifecycleListener>(onCreation, onDeletion)};
	objectLifecycleListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		objectLifecycleListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnLinksLifeCycle(SEditorObject endObject, LinkCallback onCreation, LinkCallback onDeletion) noexcept {
	auto listener{std::make_shared<LinkLifecycleListener>(onCreation, onDeletion)};
	std::string endObjectID;
	if (endObject) {
		endObjectID = endObject->objectID();
	}
	linkLifecycleListeners_[endObjectID].insert(listener);
	return Subscription {
		this, listener, [this, endObjectID, listener]() {
			auto it = linkLifecycleListeners_.find(endObjectID);
			it->second.erase(listener);
			if (it->second.empty()) {
				linkLifecycleListeners_.erase(endObjectID);
			}
		}
	};
}

Subscription DataChangeDispatcher::registerOnLinkValidityChange(LinkCallback callback) noexcept {
	auto listener{std::make_shared<LinkListener>(callback)};
	linkValidityChangeListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		linkValidityChangeListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnErrorChanged(ValueHandle valueHandle, Callback callback) noexcept {
	auto listener{std::make_shared<ValueHandleListener>(std::move(valueHandle), std::move(callback))};
	errorChangedListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		errorChangedListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnErrorChangedInScene(Callback callback) noexcept {
	auto listener{std::make_shared<UndoListener>(std::move(callback))};
	errorChangedInSceneListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() {
		errorChangedInSceneListeners_.erase(listener);
	}};
}

Subscription DataChangeDispatcher::registerOnPreviewDirty(SEditorObject obj, Callback callback) noexcept {
	assert(obj);
	auto listener{std::make_shared<EditorObjectListener>(obj, std::move(callback))};
	previewDirtyListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		previewDirtyListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnUndoChanged(Callback callback) noexcept {
	auto listener{std::make_shared<UndoListener>(std::move(callback))};
	undoChangeListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		undoChangeListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnExternalProjectChanged(Callback callback) noexcept {
	auto listener{std::make_shared<UndoListener>(std::move(callback))};
	externalProjectChangedListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		externalProjectChangedListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnExternalProjectMapChanged(Callback callback) noexcept {
	auto listener{std::make_shared<UndoListener>(std::move(callback))};
	externalProjectMapChangedListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() {
		externalProjectMapChangedListeners_.erase(listener);
	}};
}

Subscription DataChangeDispatcher::registerOnAfterDispatch(Callback callback) {
	auto listener{std::make_shared<UndoListener>(std::move(callback))};
	onAfterDispatchListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		onAfterDispatchListeners_.erase(listener); 
	}};
}

void DataChangeDispatcher::registerBulkChangeCallback(BulkChangeCallback callback) {
	bulkChangeCallback_ = callback;
}

void DataChangeDispatcher::resetBulkChangeCallback() {
	bulkChangeCallback_ = nullptr;
}

void DataChangeDispatcher::emitUpdateFor(const std::map<std::string, std::set<core::ValueHandle>>& valueHandles) {
	decltype(listeners_)::mapped_type dirtyListeners;

	for (const auto& [objectID, cont] : valueHandles) {
		for (const auto& valueHandle : cont) {
			LOG_TRACE_IF(log_system::DATA_CHANGE, valueHandle && !valueHandle.isObject(), "emit changedValueHandle Property {}:{}", valueHandle.rootObject()->objectName(), valueHandle.getPropName());
			LOG_TRACE_IF(log_system::DATA_CHANGE, valueHandle && valueHandle.isObject(), "emit changedValueHandle Object {}:{}", valueHandle.rootObject()->objectName(), valueHandle.rootObject()->objectID());
			LOG_TRACE_IF(log_system::DATA_CHANGE, !valueHandle, "emit changedValueHandle project-global");

			auto listenerIt = listeners_.find(objectID);
			if (listenerIt != listeners_.end()) {
				for (const auto& ptr : listenerIt->second) {
					if (!ptr.expired()) {
						auto listener{ptr.lock()};
						if (listener->valueHandle() == valueHandle) {
							dirtyListeners.insert(ptr);
						}
					}
				}
			}

			decltype(childrenListeners_)::mapped_type dirtyChildrenListeners;
			auto childListenerIt = childrenListeners_.find(objectID);
			if (childListenerIt != childrenListeners_.end()) {
				for (const auto& ptr : childListenerIt->second) {
					if (!ptr.expired()) {
						auto listener{ptr.lock()};
						if (listener->valueHandle() == valueHandle || listener->valueHandle().contains(valueHandle)) {
							dirtyChildrenListeners.insert(ptr);
						}
					}
				}
			}
			for (const auto& ptr : dirtyChildrenListeners) {
				if (!ptr.expired()) {
					ptr.lock()->call(valueHandle);
				}
			}

			if (valueHandle.depth() > 0) {
				auto propName = valueHandle.getPropName();

				decltype(propertyChangeListeners_)::mapped_type dirtyPropertyListeners;
				auto it = propertyChangeListeners_.find(propName);
				if (it != propertyChangeListeners_.end()) {
					for (const auto& ptr : it->second) {
						if (!ptr.expired()) {
							dirtyPropertyListeners.insert(ptr);
						}
					}
				}
				for (const auto& ptr : dirtyPropertyListeners) {
					if (!ptr.expired()) {
						ptr.lock()->call(valueHandle);
					}
				}
			}
		}
	}

	for (const auto& ptr : dirtyListeners) {
		if (!ptr.expired()) {
			ptr.lock()->call();
		}
	}
}

void DataChangeDispatcher::emitErrorChanged(const ValueHandle& valueHandle) {
	auto copy{errorChangedListeners_};
	for (const auto& ptr : copy) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			if (listener->valueHandle() == valueHandle) {
				listener->call();
			}
		}
	}
}

void DataChangeDispatcher::emitErrorChangedInScene() {
	auto copy{errorChangedInSceneListeners_};
	for (const auto& ptr : copy) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			listener->call();
		}
	}
}

void DataChangeDispatcher::emitCreated(SEditorObject obj) {
	auto copy{objectLifecycleListeners_};
	for (const auto& ptr : copy) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			listener->onCreation(obj);
		}
	}
}

void DataChangeDispatcher::assertEmpty() {
	assert(objectLifecycleListeners_.empty());
	assert(linkLifecycleListeners_.empty());
	assert(linkValidityChangeListeners_.empty());
	assert(listeners_.empty());
	assert(childrenListeners_.empty());
	assert(propertyChangeListeners_.empty());
	assert(previewDirtyListeners_.empty());
	assert(errorChangedListeners_.empty());
	assert(errorChangedInSceneListeners_.empty());
	assert(undoChangeListeners_.empty());
	assert(externalProjectChangedListeners_.empty());
	assert(externalProjectMapChangedListeners_.empty());
	assert(onAfterDispatchListeners_.empty());
}

void DataChangeDispatcher::emitDeleted(SEditorObject obj) {
	auto copy{objectLifecycleListeners_};
	for (const auto& ptr : copy) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			listener->onDeletion(obj);
		}
	}
}

void DataChangeDispatcher::emitPreviewDirty(SEditorObject obj) {
	auto copy{previewDirtyListeners_};
	for (const auto& ptr : copy) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			if (listener->editorObject() == obj) {
				listener->call();
			}
		}
	}
}

void DataChangeDispatcher::emitBulkChange(const SEditorObjectSet& changedObjects) {
	if (bulkChangeCallback_) {
		bulkChangeCallback_(changedObjects);
	}
}

}  // namespace raco::core
