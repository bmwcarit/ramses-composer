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
	void call(ValueHandle handle) const noexcept {
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

	for (auto& link : dataChanges.getValidityChangedLinks()) {
		auto copy{linkValidityChangeListeners_};
		for (const auto& ptr : copy) {
			if (!ptr.expired()) {
				auto listener{ptr.lock()};
				listener->onLinkChange(link);
			}
		}
	}

	for (auto& link : dataChanges.getRemovedLinks()) {
		auto copy{linkLifecycleListeners_};
		for (const auto& ptr : copy) {
			if (!ptr.expired()) {
				auto listener{ptr.lock()};
				listener->onDeletion_(link);
			}
		}
	}

	// generic dispatcher code does
	// - register links
	// - set dirty flag


	for (auto& createdObject : dataChanges.getCreatedObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitCreated {}", createdObject.rootObject()->objectName());
		emitCreated(createdObject);
	}

	for (auto& changedValueHandle : dataChanges.getChangedValues()) {
		LOG_TRACE_IF(log_system::DATA_CHANGE, !changedValueHandle.isObject(), "emit changedValueHandle Property {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.getPropName());
		LOG_TRACE_IF(log_system::DATA_CHANGE, changedValueHandle.isObject(), "emit changedValueHandle Object {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.rootObject()->objectID());
		emitUpdateFor(changedValueHandle);
	}

	for (auto& changedValueHandle : dataChanges.getChangedErrors()) {
		LOG_TRACE_IF(log_system::DATA_CHANGE, !changedValueHandle.isObject(), "emit errorChanged for Property {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.getPropName());
		LOG_TRACE_IF(log_system::DATA_CHANGE, changedValueHandle.isObject(), "emit errorChanged for Object {}:{}", changedValueHandle.rootObject()->objectName(), changedValueHandle.rootObject()->objectID());
		emitErrorChanged(changedValueHandle);
	}

	for (auto& object : dataChanges.getPreviewDirtyObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitPreviewDirty {}", object->objectName());
		emitPreviewDirty(object);
	}

	// Bulk update notification will be used by the SceneAdaptor to perform the actual engine update.
	emitBulkChange(dataChanges.getAllChangedObjects(true));

	for (auto& link : dataChanges.getAddedLinks()) {
		auto copy{linkLifecycleListeners_};
		for (const auto& ptr : copy) {
			if (!ptr.expired()) {
				auto listener{ptr.lock()};
				listener->onCreation_(link);
			}
		}
	}

	for (auto& deletedObject : dataChanges.getDeletedObjects()) {
		LOG_TRACE(log_system::DATA_CHANGE, "emit emitDeleted {}", deletedObject.rootObject()->objectName());
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
	listeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		listeners_.erase(listener); 
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
	propertyChangeListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() {
		propertyChangeListeners_.erase(listener);
	}};
}

Subscription DataChangeDispatcher::registerOnChildren(ValueHandle valueHandle, ValueHandleCallback callback) noexcept {
	auto listener{std::make_shared<ChildrenListener>(std::move(valueHandle), std::move(callback))};
	childrenListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() {
		childrenListeners_.erase(listener);
	}};
}


Subscription DataChangeDispatcher::registerOnObjectsLifeCycle(EditorObjectCallback onCreation, EditorObjectCallback onDeletion) noexcept {
	auto listener{std::make_shared<ObjectLifecycleListener>(onCreation, onDeletion)};
	objectLifecycleListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		objectLifecycleListeners_.erase(listener); 
	}};
}

Subscription DataChangeDispatcher::registerOnLinksLifeCycle(LinkCallback onCreation, LinkCallback onDeletion) noexcept {
	auto listener{std::make_shared<LinkLifecycleListener>(onCreation, onDeletion)};
	linkLifecycleListeners_.insert(listener);
	return Subscription{this, listener, [this, listener]() { 
		linkLifecycleListeners_.erase(listener); 
	}};
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

void DataChangeDispatcher::emitUpdateFor(const ValueHandle& valueHandle) {
	auto copyListeners{listeners_};
	for (const auto& ptr : copyListeners) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			if (listener->valueHandle() == valueHandle) {
				listener->call();
			}
		}
	}

	auto copyChildListeners{childrenListeners_};
	for (const auto& ptr : copyChildListeners) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			if (listener->valueHandle() == valueHandle || listener->valueHandle().contains(valueHandle)) {
				listener->call(valueHandle);
			}
		}
	}

	auto copyObjectChangeListeners{objectChangeListeners_};
	for (const auto& ptr : copyObjectChangeListeners) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			if (listener->editorObject() == valueHandle.rootObject()) {
				listener->call();
			}
		}
	}
	auto copyPropertyChangeListeners{propertyChangeListeners_};
	for (const auto& ptr : copyPropertyChangeListeners) {
		if (!ptr.expired()) {
			auto listener{ptr.lock()};
			// only check last element of property names vector to handle dynamic properties in Tables
			if (valueHandle.depth() > 0 && listener->property() == valueHandle.getPropName()) {
				listener->call(valueHandle);
			}
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
	assert(objectChangeListeners_.empty());
	assert(propertyChangeListeners_.empty());
	assert(previewDirtyListeners_.empty());
	assert(errorChangedListeners_.empty());
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

void DataChangeDispatcher::emitBulkChange(const std::set<SEditorObject>& changedObjects) {
	if (bulkChangeCallback_) {
		bulkChangeCallback_(changedObjects);
	}
}

}  // namespace raco::core
