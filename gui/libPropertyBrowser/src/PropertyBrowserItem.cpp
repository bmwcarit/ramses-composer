/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserItem.h"

#include "core/BasicAnnotations.h"
#include "core/Errors.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/PropertyDescriptor.h"
#include "core/Queries.h"
#include "core/Queries_Tags.h"
#include "property_browser/PropertyBrowserRef.h"
#include "property_browser/PropertyBrowserCache.h"

#include "user_types/EngineTypeAnnotation.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/RenderPass.h"

#include <cassert>
#include <spdlog/fmt/bundled/ranges.h>

using raco::data_storage::PrimitiveType;
using raco::log_system::PROPERTY_BROWSER;

using SDataChangeDispatcher = raco::components::SDataChangeDispatcher;

namespace raco::property_browser {

PropertyBrowserItem::PropertyBrowserItem(
	const std::set<core::ValueHandle>& valueHandles,
	SDataChangeDispatcher dispatcher,
	core::CommandInterface* commandInterface,
	PropertyBrowserModel* model,
	PropertyBrowserItem* parent)
	: QObject{parent},
	  parentItem_{parent},
	  valueHandles_(valueHandles.begin(), valueHandles.end()),

	  commandInterface_{commandInterface},
	  dispatcher_{dispatcher},
	  model_{model},
	  expanded_{getDefaultExpanded()} {

	assert(std::all_of(valueHandles_.begin(), valueHandles_.end(), [](const core::ValueHandle& handle) {
		return handle.isObject();
	}) || std::all_of(valueHandles_.begin(), valueHandles_.end(), [](const core::ValueHandle& handle) {
		return handle.isProperty();
	}));
	assert(isObject() || allHandlesHaveSamePropName());
	assert(isObject() || std::all_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) {
		return handle.type() == type();
	}));

	QObject::connect(&PropertyBrowserCache::instance(), &PropertyBrowserCache::newExpandedStateCached, this, &PropertyBrowserItem::onNewExpandedStateCached);

	const auto commonValueHandles = getCommonValueHandles(valueHandles_);
	for (auto& commonHandleForSeveralObjects : commonValueHandles) {
		children_.push_back(new PropertyBrowserItem(commonHandleForSeveralObjects, dispatcher_, commandInterface_, model_, this));
	}

	if (isProperty() && valueHandles_.begin()->type() == PrimitiveType::Ref) {
		refItem_ = new PropertyBrowserRef(this);
	}

	if (isProperty()) {
		editable_ = editable();
	}

	std::for_each(valueHandles_.begin(), valueHandles_.end(), [this, &dispatcher](const core::ValueHandle& handle) {
		if (handle.isObject()) {
			structureChangeSubs_.emplace_back(dispatcher->registerOnChildren(handle, [this, &handle](auto changedHandle) {
				if (changedHandle.type() == PrimitiveType::Table) {
					if (!findNamedChild(changedHandle.getPropertyNamesVector().front())) {
						syncChildrenWithValueHandle();
					}
				}
			}));
		} else if (hasTypeSubstructure(handle.type())) {
			structureChangeSubs_.emplace_back(dispatcher->registerOn(handle, [this, &handle]() {
				syncChildrenWithValueHandle();
			}));
		}

		handleChangeSubs_.emplace_back(components::Subscription(dispatcher->registerOn(handle, [this, &handle]() {
			Q_EMIT valueChanged();
			if (handle.isProperty()) {
				if (auto newEditable = editable(); newEditable != editable_) {
					editable_ = newEditable;
					Q_EMIT editableChanged(editable());
					Q_EMIT linkStateChanged();
				}
			}
		})));
		errorSubs_.emplace_back(components::Subscription(dispatcher->registerOnErrorChanged(handle, [this]() {
			Q_EMIT errorChanged();
		})));

		// links
		bool linkEnd = core::Queries::isValidLinkEnd(*project(), handle);
		bool linkStart = core::Queries::isValidLinkStart(handle);

		auto linkValidityCallback = [this, &handle](const core::LinkDescriptor& link) {
			if (isValid()) {
				auto endHandle = core::ValueHandle(link.end);
				if (endHandle && endHandle == handle) {
					updateLinkState();
				}
				auto startHandle = core::ValueHandle(link.start);
				if (startHandle && startHandle == handle) {
					updateLinkState();
				}
			}
		};

		if (linkStart || linkEnd) {
			if (auto link = core::Queries::getLink(*project(), handle.getDescriptor())) {
				linkValidityChangeSubs_[handle] = components::Subscription(dispatcher_->registerOnLinkValidityChange(linkValidityCallback));
			}
		}

		if (linkEnd) {
			linkLifecycleEndSubs_.emplace_back(components::Subscription(dispatcher_->registerOnLinksLifeCycleForEnd(
				handle.rootObject(),
				[this, linkValidityCallback, &handle](const core::LinkDescriptor& link) {
						if (isValid()) {
							auto endHandle = core::ValueHandle(link.end);
							if (endHandle && endHandle == handle) {
								linkValidityChangeSubs_[handle] = components::Subscription(dispatcher_->registerOnLinkValidityChange(linkValidityCallback));
								updateLinkState();
							}
						} },
				[this, &handle](const core::LinkDescriptor& link) {
					if (isValid()) {
						auto endHandle = core::ValueHandle(link.end);
						if (endHandle && endHandle == handle) {
							linkValidityChangeSubs_.erase(handle);
							updateLinkState();
						}
					}
				})));
		}
		if (linkStart) {
			linkLifecycleStartSubs_.emplace_back(components::Subscription(dispatcher_->registerOnLinksLifeCycleForStart(
				handle.rootObject(),
				[this, linkValidityCallback, &handle](const core::LinkDescriptor& link) {
						if (isValid()) {
							auto startHandle = core::ValueHandle(link.start);
							if (startHandle && startHandle == handle) {
								linkValidityChangeSubs_[handle] = components::Subscription(dispatcher_->registerOnLinkValidityChange(linkValidityCallback));
								updateLinkState();
							}
						} },
				[this, &handle](const core::LinkDescriptor& link) {
					if (isValid()) {
						auto startHandle = core::ValueHandle(link.start);
						if (startHandle && startHandle == handle) {
							linkValidityChangeSubs_.erase(handle);
							updateLinkState();
						}
					}
				})));
		}

		// This needs refactoring. There needs to be a way for a user_type to say that some properties are only visible (or enabled?)
		// when certain conditions depending on another property are fulfilled.
		static const std::map<data_storage::ReflectionInterface::TypeDescriptor const*, std::string> requiredChildSubscriptions = {
			{&user_types::RenderPass::typeDescription, "target"},
			{&user_types::RenderBuffer::typeDescription, "format"},
			{&user_types::RenderLayer::typeDescription, "sortOrder"}};
		if (const auto itChildSub = requiredChildSubscriptions.find(&handle.rootObject()->getTypeDescription()); handle.depth() == 0 && itChildSub != requiredChildSubscriptions.end()) {
			childrenChangeSubs_.emplace_back(dispatcher->registerOn(core::ValueHandle{handle.rootObject(), {itChildSub->second}}, [this, &handle] {
				if (isValid()) {
					syncChildrenWithValueHandle();
				}
			}));
		}
	});
}

bool PropertyBrowserItem::isHidden(core::ValueHandle handle) const {
	if (core::Queries::isHiddenInPropertyBrowser(*commandInterface_->project(), handle)) {
		return true;
	}

	if (const auto& renderPass = handle.rootObject()->as<user_types::RenderPass>(); renderPass != nullptr && renderPass->target_.asRef() == nullptr && renderPass->isClearTargetProperty(handle)) {
		return true;
	}

	if (const auto& renderBuffer = handle.rootObject()->as<user_types::RenderBuffer>(); renderBuffer != nullptr && !renderBuffer->areSamplingParametersSupported(engineInterface()) && renderBuffer->isSamplingProperty(handle)) {
		return true;
	}
	return false;
}

bool PropertyBrowserItem::match(const core::ValueHandle& left, const core::ValueHandle& right) const {
	using namespace core;

	if (left && right && left.type() == right.type()) {
		if (!matchAnnotation<DisplayNameAnnotation>(left, right)) {
			return false;
		}
		switch (left.type()) {
			case PrimitiveType::Int: {
				return matchAnnotation<EnumerationAnnotation>(left, right);
				break;
			}

			case PrimitiveType::String: {
				return matchAnnotation<URIAnnotation>(left, right);
				break;
			}

			case PrimitiveType::Ref:
				return left.constValueRef()->baseTypeName() == right.constValueRef()->baseTypeName();
				break;

			case PrimitiveType::Table: {
				auto leftIsTag = Queries::isTagContainerProperty(left);
				auto rightIsTag = Queries::isTagContainerProperty(right);

				if (leftIsTag != rightIsTag) {
					return false;
				}

				if (leftIsTag) {
					if (core::Queries::getHandleTagType(left) != core::Queries::getHandleTagType(right)) {
						return false;
					}
					if (!matchAnnotation<core::UserTagContainerAnnotation>(left, right) || !matchAnnotation<core::TagContainerAnnotation>(left, right) || !matchAnnotation<core::RenderableTagContainerAnnotation>(left, right)) {
						return false;
					}
				}

				// Do no descend into subtree for TagContainerAnnotation, RenderableTagContainerAnnotation and UserTagContainerAnnotation properties
				if (!leftIsTag) {

					if (left.size() != right.size()) {
						return false;
					}

					for (size_t index = 0; index < left.size(); index++) {
						auto propName = left[index].getPropName();
						if (propName.empty()) {
							return false;
						}
						if (!right.hasProperty(propName)) {
							return false;
						}
						if (!match(left[index], right.get(propName))) {
							return false;
						}
					}
				}
				return true;
				break;
			}

			case PrimitiveType::Struct:
				return left.constValueRef()->baseTypeName() == right.constValueRef()->baseTypeName();
				break;

			default:
				return true;
		}
	}
	return false;
}

bool PropertyBrowserItem::matchCurrent() const {
	return std::all_of(++valueHandles_.begin(), valueHandles_.end(), [this](auto handle) {
		return match(*valueHandles_.begin(), handle);
	});
}

std::vector<std::set<core::ValueHandle>> PropertyBrowserItem::getCommonValueHandles(const std::set<core::ValueHandle>& handles) const {
	std::vector<std::set<core::ValueHandle>> children;

	core::ValueHandle refHandle = *handles.begin();

	if (refHandle.isProperty() && (refHandle.query<core::UserTagContainerAnnotation>() || refHandle.query<core::TagContainerAnnotation>())) {
		return {};
	}

	for (size_t index = 0; index < refHandle.size(); index++) {
		auto refChildHandle = refHandle[index];
		auto propName = refChildHandle.getPropName();

		if (!isHidden(refChildHandle)) {
			if (std::all_of(++handles.begin(), handles.end(), [this, refChildHandle, propName](auto handle) {
					return handle.hasProperty(propName) && !isHidden(handle.get(propName)) && match(refChildHandle, handle.get(propName));
				})) {
				std::set<core::ValueHandle> childHandleSet;
				for (auto handle : handles) {
					childHandleSet.insert(handle.get(propName));
				}
				children.emplace_back(childHandleSet);
			}
		}
	}
	return children;
}

void PropertyBrowserItem::getTagsInfo(std::set<std::shared_ptr<user_types::RenderPass>>& renderedBy, bool& isMultipleRenderedBy, std::set<std::shared_ptr<user_types::RenderLayer>>& addedTo, bool& isMultipleAddedTo) const {
	const auto tagData = core::TagDataCache::createTagDataCache(project(), core::TagType::NodeTags_Referencing);
	const auto firstHandleAllTags = core::Queries::renderableTagsWithParentTags((*valueHandles_.begin()).rootObject());
	renderedBy = tagData->allRenderPassesForObjectWithTags((*valueHandles_.begin()).rootObject(), firstHandleAllTags);
	addedTo = tagData->allReferencingObjects<user_types::RenderLayer>(firstHandleAllTags);
	for (const auto& handle : valueHandles_) {
		const auto handleAllTags = core::Queries::renderableTagsWithParentTags(handle.rootObject());
		const auto handleRenderedBy = tagData->allRenderPassesForObjectWithTags(handle.rootObject(), handleAllTags);

		if (!isMultipleRenderedBy && renderedBy != handleRenderedBy) {
			isMultipleRenderedBy = true;
		}

		if (!isMultipleAddedTo) {
			const auto handleAddedTo = tagData->allReferencingObjects<user_types::RenderLayer>(handleAllTags);
			if (addedTo != handleAddedTo) {
				isMultipleAddedTo = true;
			}
		}
	}
}

std::string PropertyBrowserItem::getRootObjectName() const {
	if (valueHandles_.size() > 1) {
		return "Multiple objects";
	}
	return (*valueHandles_.begin()).rootObject()->objectName();
}

std::string PropertyBrowserItem::getPropertyName() const {
	return valueHandles_.begin()->getPropName();
}

std::string PropertyBrowserItem::getPropertyPath() const {
	return core::Queries::getPropertyPath(valueHandles_);
}

std::string PropertyBrowserItem::getPropertyPathWithoutObject() const {
	return fmt::format("{}", fmt::join(valueHandles_.begin()->getPropertyNamesVector(), "."));
}

void PropertyBrowserItem::updateLinkState() noexcept {
	Q_EMIT linkStateChanged();
	Q_EMIT editableChanged(editable());
	for (auto* child_ : children_) {
		child_->updateLinkState();
	}
}

void PropertyBrowserItem::onNewExpandedStateCached(const std::string& namesVector, bool expanded) {
	if (!isObject() && expandable() && (getPropertyPathWithoutObject() == namesVector) && !isLocked()) {
		setExpanded(expanded, false);
	}
}

PrimitiveType PropertyBrowserItem::type() const noexcept {
	return valueHandles_.begin()->type();
}

std::string PropertyBrowserItem::luaTypeName() const noexcept {
	std::unordered_set<std::string> luaTypeNames;
	for (const auto& handle : valueHandles_) {
		if (auto anno = handle.constValueRef()->query<user_types::EngineTypeAnnotation>()) {
			luaTypeNames.emplace(commandInterface_->engineInterface().luaNameForPrimitiveType(static_cast<core::EnginePrimitive>(*anno->engineType_)));
			if (luaTypeNames.size() > 1) {
				return {};
			}
		}
	}

	if (luaTypeNames.size() == 1) {
		return *luaTypeNames.begin();
	}

	return {};
}

PropertyBrowserRef* PropertyBrowserItem::refItem() noexcept {
	return refItem_;
}

PropertyBrowserModel* PropertyBrowserItem::model() const noexcept {
	return model_;
}

const QList<PropertyBrowserItem*>& PropertyBrowserItem::children() {
	return children_;
}

PropertyBrowserItem* PropertyBrowserItem::findNamedChild(const std::string& propertyName) {
	auto it = std::find_if(children_.begin(), children_.end(), [propertyName](auto child) {
		return child->getPropertyName() == propertyName;
	});
	if (it != children_.end()) {
		return *it;
	}
	return nullptr;
}

bool PropertyBrowserItem::hasError() const {
	return std::any_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) {
		return commandInterface_->errors().hasError(handle);
	});
}

std::optional<core::ErrorCategory> PropertyBrowserItem::errorCategory() const {
	std::set<core::ErrorCategory> categories;
	std::for_each(valueHandles_.begin(), valueHandles_.end(), [this, &categories](const core::ValueHandle& handle) {
		if (commandInterface_->errors().hasError(handle)) {
			categories.insert(commandInterface_->errors().getError(handle).category());
		}
	});
	if (categories.size() == 1) {
		return *categories.begin();
	}
	return std::nullopt;
}

core::ErrorLevel PropertyBrowserItem::maxErrorLevel() const {
	core::ErrorLevel maxLevel = core::ErrorLevel::NONE;
	for (const auto error : errors()) {
		maxLevel = std::max(maxLevel, error->level());
	}
	return maxLevel;
}

std::vector<const core::ErrorItem*> PropertyBrowserItem::errors() const {
	std::vector<const core::ErrorItem*> errors;
	std::for_each(valueHandles_.begin(), valueHandles_.end(), [this, &errors](const core::ValueHandle& handle) {
		if (commandInterface_->errors().hasError(handle)) {
			errors.push_back(&commandInterface_->errors().getError(handle));
		}
	});
	return errors;
}

std::string PropertyBrowserItem::errorMessage() const {
	std::set<std::string> messages;
	std::for_each(valueHandles_.begin(), valueHandles_.end(), [this, &messages](const core::ValueHandle& handle) {
		if (commandInterface_->errors().hasError(handle)) {
			messages.insert(commandInterface_->errors().getError(handle).message());
		} else {
			messages.insert(std::string());
		}
	});
	if (messages.size() == 1) {
		return *messages.begin();
	} else if (messages.size() > 1) {
		std::map<core::ErrorLevel, std::string> levelDesc = {
			{core::ErrorLevel::NONE, ""},
			{core::ErrorLevel::INFORMATION, "Information Items"},
			{core::ErrorLevel::WARNING, "Warnings"},
			{core::ErrorLevel::ERROR, "Errors"}};
		return "Multiple " + levelDesc[maxErrorLevel()];
	}
	return {};
}

void PropertyBrowserItem::markForDeletion() {
	// prevent crashes caused by delayed subscription callbacks
	handleChangeSubs_.clear();
	structureChangeSubs_.clear();
	errorSubs_.clear();
	linkValidityChangeSubs_.clear();
	linkLifecycleStartSubs_.clear();
	linkLifecycleEndSubs_.clear();
	childrenChangeSubs_.clear();

	for (auto& child : children_) {
		child->markForDeletion();
	}

	this->deleteLater();
}

size_t PropertyBrowserItem::size() noexcept {
	return children_.size();
}

std::string PropertyBrowserItem::displayName() const noexcept {
	//  Note: the property names _must_ in multiedit case:
	if (auto displayNameAnno = query<core::DisplayNameAnnotation>()) {
		return *displayNameAnno->name_;
	}
	return getPropertyName();
}

const std::set<core::ValueHandle>& PropertyBrowserItem::valueHandles() noexcept {
	return valueHandles_;
}

std::optional<core::SEditorObject> PropertyBrowserItem::asRef() const {
	core::SEditorObject value = valueHandles_.begin()->asRef();
	if (std::all_of(++valueHandles_.begin(), valueHandles_.end(), [value](auto handle) {
			return handle.asRef() == value;
		})) {
		return value;
	}
	return std::nullopt;
}

core::Project* PropertyBrowserItem::project() const {
	return commandInterface_->project();
}

const core::CommandInterface* PropertyBrowserItem::commandInterface() const {
	return commandInterface_;
}

core::CommandInterface* PropertyBrowserItem::commandInterface() {
	return commandInterface_;
}

components::SDataChangeDispatcher PropertyBrowserItem::dispatcher() const {
	return dispatcher_;
}

core::EngineInterface& PropertyBrowserItem::engineInterface() const {
	return commandInterface_->engineInterface();
}

bool PropertyBrowserItem::editable() noexcept {
	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) {
		return !core::Queries::isReadOnly(*commandInterface_->project(), handle);
	});
}

bool PropertyBrowserItem::expandable() const noexcept {
	// Note: we already know that the expandable state must be the same for all properties in multiselection case:
	const auto& handle = *valueHandles_.begin();
	return handle.isObject() || handle.hasSubstructure() && !handle.query<core::TagContainerAnnotation>() && !handle.query<core::UserTagContainerAnnotation>();
}

void PropertyBrowserItem::setLocked(bool locked) noexcept {
	locked_ = locked;
}

bool PropertyBrowserItem::isLocked() const noexcept {
	if (parentItem_) {
		return parentItem_->isLocked();
	}
	return locked_;
}

bool PropertyBrowserItem::showChildren() const {
	return expandable() && children_.size() > 0 && expanded_;
}

void PropertyBrowserItem::requestNextSiblingFocus() {
	auto children = &parentItem()->children();
	int index = children->indexOf(this);
	if (index >= 0 && index < children->size() - 1) {
		Q_EMIT children->at(index + 1)->widgetRequestFocus();
	}
}

void PropertyBrowserItem::setLink(const core::ValueHandle& start, bool isWeak) noexcept {
	std::string desc = fmt::format("Create {} link from '{}' to '{}'", isWeak ? "weak" : "strong", start.getPropertyPath(), getPropertyPath());
	commandInterface_->executeCompositeCommand(
		[this, start, isWeak]() {
			for (const auto& handle : valueHandles_) {
				commandInterface_->addLink(start, handle, isWeak);
			}
		},
		desc);
}

void PropertyBrowserItem::removeLink() noexcept {
	std::string desc = fmt::format("Remove link ending on '{}'", getPropertyPath());
	commandInterface_->executeCompositeCommand(
		[this]() {
			for (const auto& handle : valueHandles_) {
				commandInterface_->removeLink(handle.getDescriptor());
			}
		},
		desc);
}

std::optional<std::string> PropertyBrowserItem::linkText(bool fullLinkPath) const noexcept {
	if (!valueHandles_.empty()) {
		auto firstHandleLinkText = linkText(*valueHandles_.begin(), fullLinkPath);
		if (std::all_of(++valueHandles_.begin(), valueHandles_.end(), [&firstHandleLinkText, fullLinkPath, this](const core::ValueHandle& handle) {
				return linkText(handle, fullLinkPath) == firstHandleLinkText;
			})) {
			return firstHandleLinkText;
		}
	}

	return std::nullopt;
}

std::string PropertyBrowserItem::linkText(const core::ValueHandle& valueHandle, bool fullLinkPath) const noexcept {
	if (auto link{core::Queries::getLink(*commandInterface_->project(), valueHandle.getDescriptor())}) {
		auto propertyDesc = link->startProp();
		auto propertyPath = (fullLinkPath) ? propertyDesc.getFullPropertyPath() : propertyDesc.getPropertyPath();
		if (*link->isWeak_) {
			propertyPath += " (weak)";
		}
		if (!link->isValid()) {
			propertyPath += " (broken)";
		}

		return propertyPath;
	}

	return "";
}

PropertyBrowserItem* PropertyBrowserItem::parentItem() const noexcept {
	return parentItem_;
}

PropertyBrowserItem* PropertyBrowserItem::rootItem() noexcept {
	auto current = this;
	while (current->parentItem()) {
		current = current->parentItem();
	}
	return current;
}

PropertyBrowserItem* PropertyBrowserItem::siblingItem(std::string_view propertyName) const noexcept {
	if (!parentItem()) {
		return nullptr;
	}
	for (auto* sibling : parentItem()->children()) {
		if (sibling->getPropertyName() == propertyName) {
			return sibling;
		}
	}
	return nullptr;
}

bool PropertyBrowserItem::isRoot() const noexcept {
	return parentItem() == nullptr;
}

bool PropertyBrowserItem::hasCollapsedParent() const noexcept {
	if (isRoot()) {
		return false;
	} else {
		return !parentItem()->expanded() || parentItem()->hasCollapsedParent();
	}
}

PropertyBrowserItem* PropertyBrowserItem::findItemWithNoCollapsedParentInHierarchy() noexcept {
	PropertyBrowserItem* current = this;
	while (current->hasCollapsedParent()) {
		current = current->parentItem();
	}
	return current;
}

bool PropertyBrowserItem::expanded() const noexcept {
	return expanded_;
}

void PropertyBrowserItem::setExpanded(bool expanded, bool byUser) noexcept {
	assert(expandable());

	if (expanded_ != expanded) {
		expanded_ = expanded;
		
		if (byUser && !isLocked()) {
			const auto namesVector = getPropertyPathWithoutObject();
			if (!namesVector.empty()) {
				PropertyBrowserCache::instance().cacheExpandedState(namesVector, expanded_);
			}
		}
		Q_EMIT expandedChanged(expanded_);
		Q_EMIT showChildrenChanged(showChildren());
	}
}

void PropertyBrowserItem::setExpandedRecursively(bool expanded) noexcept {
	if (expandable()) {
		setExpanded(expanded);

		for (const auto& child : children_) {
			child->setExpandedRecursively(expanded);
		}
	}
}

void PropertyBrowserItem::restoreDefaultExpandedRecursively() noexcept {
	const auto defaultExpended = getDefaultExpanded();
	if (defaultExpended != expanded_) {
		expanded_ = defaultExpended;
		Q_EMIT expandedChanged(expanded_);
		Q_EMIT showChildrenChanged(showChildren());
	}

	for (const auto& child : children_) {
		child->restoreDefaultExpandedRecursively();
	}
}

void PropertyBrowserItem::setTags(std::vector<std::string> const& tags) {
	std::string desc = fmt::format("Set tag set property '{}' to {}", getPropertyPath(), tags);
	commandInterface_->executeCompositeCommand(
		[this, &tags]() {
			for (const auto& handle : valueHandles_) {
				commandInterface_->setTags(handle, tags);
			}
		},
		desc);
}

void PropertyBrowserItem::setTags(std::vector<std::pair<std::string, int>> const& prioritizedTags) {
	std::string desc = fmt::format("Set renderable tag property '{}'", getPropertyPath());
	commandInterface_->executeCompositeCommand(
		[this, &prioritizedTags]() {
			for (const auto& handle : valueHandles_) {
				commandInterface_->setRenderableTags(handle, prioritizedTags);
			}
		},
		desc);
}

void PropertyBrowserItem::syncChildrenWithValueHandle() {
	if (isProperty() && !matchCurrent()) {
		// matching has failed
		// -> the current subtree will need to be removed at the root item
		// -> rebuild the root item
		rootItem()->syncChildrenWithValueHandle();
	} else {
		// matching still successful
		// -> sufficient to rebuild the current item

		// clear children
		for (auto& child : children_) {
			child->markForDeletion();
		}
		children_.clear();

		// create new children
		const auto commonValueHandles = getCommonValueHandles(valueHandles_);
		if (!commonValueHandles.empty()) {
			children_.reserve(static_cast<int>(commonValueHandles.size()));
			for (const auto& commonHandleForSeveralObjects : commonValueHandles) {
				children_.push_back(new PropertyBrowserItem(commonHandleForSeveralObjects, dispatcher_, commandInterface_, model_, this));
			}
		}

		Q_EMIT childrenChanged(children_);

		// notify first not collapsed item about the structural changed
		{
			auto* visibleItem = findItemWithNoCollapsedParentInHierarchy();
			assert(visibleItem != nullptr);
			Q_EMIT visibleItem->childrenChangedOrCollapsedChildChanged();
		}

		// notify the view state accordingly
		if (children_.size() <= 1) {
			Q_EMIT expandedChanged(expanded());
			Q_EMIT showChildrenChanged(showChildren());
		}
	}
}

bool PropertyBrowserItem::isValid() const {
	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) -> bool {
		return static_cast<bool>(handle);
	});
}

bool PropertyBrowserItem::canBeChosenByColorPicker() const {
	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) {
		if (handle.isObject() || !(handle.isVec3f() || handle.isVec4f())) {
			return false;
		}

		const auto rootTypeRef = &handle.rootObject()->getTypeDescription();

		if (!(rootTypeRef == &raco::user_types::ProjectSettings::typeDescription ||
				rootTypeRef == &raco::user_types::LuaScript::typeDescription ||
				rootTypeRef == &raco::user_types::LuaInterface::typeDescription ||
				rootTypeRef == &raco::user_types::MeshNode::typeDescription ||
				rootTypeRef == &raco::user_types::Material::typeDescription)) {
			return false;
		}

		if (handle.isRefToProp(&raco::user_types::Node::translation_) || handle.isRefToProp(&raco::user_types::Node::rotation_) || handle.isRefToProp(&raco::user_types::Node::scaling_)) {
			return false;
		}

		return true;
	});
}

bool PropertyBrowserItem::hasSingleValue() const {
	const auto firstValue = valueHandles_.begin()->constValueRef();
	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [firstValue](const core::ValueHandle& handle) {
		return *firstValue == *handle.constValueRef();
	});
}

bool PropertyBrowserItem::isLuaProperty() const {
	if (isProperty()) {
		return std::all_of(valueHandles_.begin(), valueHandles_.end(), [](const core::ValueHandle& handle) {
			return handle.rootObject()->isType<user_types::LuaScript>() || handle.rootObject()->isType<user_types::LuaInterface>();
		});
	}
	return false;
}

bool PropertyBrowserItem::isObject() const {
	return valueHandles_.begin()->isObject();
}

bool PropertyBrowserItem::isProperty() const {
	return valueHandles_.begin()->isProperty();
}

bool PropertyBrowserItem::isTagContainerProperty() const {
	return core::Queries::isTagContainerProperty(*valueHandles_.begin());
}

bool PropertyBrowserItem::allHandlesHaveSamePropName() const {
	auto propName{valueHandles_.begin()->getPropName()};
	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [&propName](const core::ValueHandle& handle) {
		return handle.getPropName() == propName;
	});
}

bool PropertyBrowserItem::getDefaultExpandedFromValueHandleType(const core::ValueHandle& handle) const {
	if (handle.isObject()) {
		return true;
	}

	const auto rootTypeRef = &handle.rootObject()->getTypeDescription();

	if (rootTypeRef == &user_types::MeshNode::typeDescription || rootTypeRef == &user_types::Material::typeDescription) {
		auto parent = handle.parent();

		if (parent.isProperty()) {
			auto parentPropName = parent.getPropName();
			bool isOptionsOrUniformGroup = parentPropName == "options" || parentPropName == "uniforms";
			return !isOptionsOrUniformGroup;
		}
	} else if (rootTypeRef == &user_types::LuaScript::typeDescription || rootTypeRef == &user_types::LuaInterface::typeDescription) {
		auto parent = handle.parent();

		bool isTopLevelLuaValueGroup = parent.isObject() &&
									   (handle.isRefToProp(&user_types::LuaScript::inputs_) || handle.isRefToProp(&user_types::LuaInterface::inputs_) || handle.isRefToProp(&user_types::LuaScript::outputs_) || handle.isRefToProp(&user_types::LuaScript::luaModules_));

		bool isFirstLevelChildOfInputOutputGroup = parent.isProperty() &&
												   parent.parent().isObject() &&
												   (parent.isRefToProp(&user_types::LuaScript::inputs_) || parent.isRefToProp(&user_types::LuaInterface::inputs_) 
												  || parent.isRefToProp(&user_types::LuaScript::outputs_));


		bool isTableOrStruct = handle.type() == PrimitiveType::Table || handle.type() == PrimitiveType::Struct;

		return isTopLevelLuaValueGroup || (isFirstLevelChildOfInputOutputGroup && isTableOrStruct);
	}

	return true;
}

bool PropertyBrowserItem::getDefaultExpanded() const {
	if (!isObject() && expandable()) {
		const auto expanded = PropertyBrowserCache::instance().getCachedExpandedState(getPropertyPathWithoutObject());
		if (expanded.has_value()) {
			return expanded.value();
		}
	}

	return std::all_of(valueHandles_.begin(), valueHandles_.end(), [this](const core::ValueHandle& handle) {
		return getDefaultExpandedFromValueHandleType(handle);
	});
}

}  // namespace raco::property_browser
