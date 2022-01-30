/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/EditorObject.h"

#include "core/Context.h"
#include "core/Errors.h"
#include "core/Handles.h"
#include "core/Iterators.h"
#include "core/PathQueries.h"
#include "core/Queries.h"
#include "core/MeshCacheInterface.h"

#include <functional>
#include <stdexcept>

#include <QUuid>

namespace raco::core {
using namespace raco::data_storage;

EditorObject::EditorObject(std::string name, std::string id) : 
	ClassWithReflectedMembers(),
															   objectName_{name, DisplayNameAnnotation("Object Name")},
	objectID_{normalizedObjectID(id), {}} 
{
	fillPropertyDescription();
}


std::string const& EditorObject::objectID() const 
{
	return *objectID_;
}

void EditorObject::setObjectID(std::string const& id) 
{
	objectID_ = normalizedObjectID(id);
}

std::string const& EditorObject::objectName() const 
{
	return *objectName_;
}

void EditorObject::setObjectName(std::string const& name) 
{
	objectName_ = name;
}

EditorObject::ChildIterator EditorObject::begin() {
	return ChildIterator(shared_from_this(), 0);
}

EditorObject::ChildIterator EditorObject::end() {
	return ChildIterator(shared_from_this(), children_->size());
}

void EditorObject::onBeforeDeleteObject(Errors& errors) const {
	errors.removeAll(shared_from_this());
	uriListeners_.clear();
}

std::string EditorObject::normalizedObjectID(std::string const& id) 
{
	if (id.empty()) {
		return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
	}
	return id;
}

std::string EditorObject::XorObjectIDs(std::string const& id1, std::string const& id2) {
	QUuid qid1(QString::fromStdString(id1));
	QUuid qid2(QString::fromStdString(id2));

	QUuid qid_result(qid1.data1 ^ qid2.data1,
		qid1.data2 ^ qid2.data2,
		qid1.data3 ^ qid2.data3,
		qid1.data4[0] ^ qid2.data4[0],
		qid1.data4[1] ^ qid2.data4[1],
		qid1.data4[2] ^ qid2.data4[2], 
		qid1.data4[3] ^ qid2.data4[3],
		qid1.data4[4] ^ qid2.data4[4], 
		qid1.data4[5] ^ qid2.data4[5],
		qid1.data4[6] ^ qid2.data4[6],
		qid1.data4[7] ^ qid2.data4[7]);

	return qid_result.toString(QUuid::WithoutBraces).toStdString();
}

const std::set<WEditorObject, std::owner_less<WEditorObject>>& EditorObject::referencesToThis() const {
	return referencesToThis_;
}

namespace {
bool hasReferenceTo(const ReflectionInterface& src, SCEditorObject target, const ValueBase* exceptSourceValueBase) {
	for (size_t index = 0; index < src.size(); index++) {
		auto v = src.get(index);
		if (v != exceptSourceValueBase) {
			if (v->type() == PrimitiveType::Ref) {
				auto vref = v->asRef();
				if (vref && vref == target) {
					return true;
				}
			} else if (hasTypeSubstructure(v->type())) {
				if (hasReferenceTo(v->getSubstructure(), target, exceptSourceValueBase)) {
					return true;
				}
			}
		}
	}
	return false;
}
}  // namespace

void EditorObject::onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const {
	auto srcRootObject = sourceReferenceProperty.rootObject();

	if (!hasReferenceTo(*srcRootObject, shared_from_this(), sourceReferenceProperty.constValueRef())) {
		referencesToThis_.erase(srcRootObject);
	}

	if (srcRootObject) {
		if (ValueHandle(srcRootObject, &EditorObject::children_).contains(sourceReferenceProperty)) {
			assert(srcRootObject == parent_.lock());
			parent_.reset();
		}
	}
}

void EditorObject::onAfterDeserialization() const {
	// Note: removing the const is necessary since we can't create ValueHandles using a
	// shared_ptr<const EditorObject>
	// To void the cast we would need to create a ValueHandle class holding a shared_ptr<const EditorObject>
	// instead and additionally create/adapt all the supporting code.
	std::shared_ptr<EditorObject> self = std::const_pointer_cast<EditorObject>(shared_from_this());
	for (const auto& valueHandle : Queries::findAllReferences(self)) {
		valueHandle.asTypedRef<EditorObject>()->onAfterAddReferenceToThis(valueHandle);
	}
}

void EditorObject::onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const {
	auto srcRootObject = sourceReferenceProperty.rootObject();
	referencesToThis_.insert(srcRootObject);

	if (srcRootObject) {
		if (ValueHandle(srcRootObject, &EditorObject::children_).contains(sourceReferenceProperty)) {
			parent_ = srcRootObject;
		}
	}
}

FileChangeMonitor::UniqueListener EditorObject::registerFileChangedHandler(BaseContext& context, const ValueHandle& value, FileChangeCallback::Callback callback) {
	auto resourceAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), value);
	return context.meshCache()->registerFileChangedHandler(resourceAbsPath,
		{&context, shared_from_this(), std::move(callback)});
}

void EditorObject::onAfterContextActivated(BaseContext& context) {
	for (size_t i = 0; i < size(); i++) {
		if (auto anno = get(i)->query<URIAnnotation>(); anno && !anno->isProjectSubdirectoryURI()) {
			auto propName = name(i);
			ValueHandle handle{shared_from_this(), {i}};
			uriListeners_[propName] = registerFileChangedHandler(context, handle, [this, handle, &context]() {
				this->updateFromExternalFile(context);
			});
		}
	}
	updateFromExternalFile(context);
}

void EditorObject::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (auto anno = value.query<URIAnnotation>(); anno && !anno->isProjectSubdirectoryURI()) {
		assert(value.depth() == 1);
		auto propName = value.getPropName();
		uriListeners_[propName] = registerFileChangedHandler(context, value, [this, value, &context]() {
			this->updateFromExternalFile(context);
		});
		updateFromExternalFile(context);
	}
}

int EditorObject::findChildIndex(const EditorObject* node) {
	for (int i = 0; i < children_->size(); i++) {
		SEditorObject child = children_->get(i)->asRef();
		if (child.get() == node) {
			return i;
		}
	}

	return -1;
}

SEditorObject EditorObject::getParent() {
	return parent_.lock();
}

EditorObject::ChildIterator::ChildIterator(SEditorObject const& object, size_t index) : object_(object), index_(index) {
}

SEditorObject EditorObject::ChildIterator::operator*() {
	return object_->children_->get(index_)->asRef();
}

bool EditorObject::ChildIterator::operator!=(ChildIterator const& other) const {
	return !(object_ == other.object_ && index_ == other.index_);
}

EditorObject::ChildIterator& EditorObject::ChildIterator::operator++() {
	if (index_ < object_->children_->size()) {
		++index_;
	}
	return *this;
}


TreeIterator::TreeIterator(SEditorObject object) : top_(object) {
}

SEditorObject TreeIterator::operator*() {
	if (stack_.empty()) {
		return top_;
	}
	return *stack_.top().current;
}

TreeIterator::operator bool() {
	return top_ != nullptr;
}

TreeIterator& TreeIterator::operator++() {
	if (*this) {
		SEditorObject current = operator*();
		if (current->children_->size() > 0) {
			stack_.push({current->begin(), current->end()});
		}
		else {
			// Traverse siblings and ascend at end
			while (!stack_.empty() && !static_cast<bool>(++stack_.top())) {
				stack_.pop();
			}
			if (stack_.empty()) {
				top_.reset();
			}
		}
	}
	return *this;
}

bool TreeIterator::operator!=(TreeIterator const& other) {
	return top_ != other.top_ || stack_ != other.stack_;
}

}

