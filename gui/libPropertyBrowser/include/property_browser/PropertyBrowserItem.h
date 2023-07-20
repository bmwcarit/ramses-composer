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
#include "core/CommandInterface.h"
#include "core/CoreFormatter.h"
#include "components/EditorObjectFormatter.h"
#include "core/ErrorItem.h"
#include "core/Handles.h"
#include "core/Queries.h"
#include "components/DataChangeDispatcher.h"

#include <QList>
#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <sstream>
#include <string>

namespace raco::property_browser {
class PropertyBrowserModel;

class PropertyBrowserRef;

/**
 * Indirection wrapper around ValueHandle.
 */
class PropertyBrowserItem final : public QObject {
	Q_OBJECT

public:
	inline static const QString MultipleValueText = "Multiple values";

	PropertyBrowserItem(const std::set<core::ValueHandle>& valueHandles, components::SDataChangeDispatcher dispatcher, core::CommandInterface* commandInterface, PropertyBrowserModel* model, PropertyBrowserItem* parent = nullptr);
	core::PrimitiveType type() const noexcept;
	std::string luaTypeName() const noexcept;
	std::string displayName() const noexcept;
	size_t size() noexcept;
	std::optional<std::string> linkText(bool fullLinkPath = false) const noexcept;
	std::string linkText(const core::ValueHandle& valueHandle, bool fullLinkPath = false) const noexcept;
	void setLink(const core::ValueHandle& start, bool isWeak) noexcept;
	void removeLink() noexcept;
	bool editable() noexcept;
	bool expandable() const noexcept;
	void setLocked(bool locked) noexcept;
	bool isLocked() const noexcept;
	template <typename T>
	core::AnnotationHandle<T> query() const {
		return valueHandles_.begin()->query<T>();
	}

	template <typename T>
	void set(T v) {
		if constexpr (std::is_same<T, int>::value) {
			commandInterface_->set(valueHandles_, v);
		} else if constexpr (std::is_same<T, int64_t>::value) {
			commandInterface_->set(valueHandles_, v);
		} else if constexpr (std::is_same<T, double>::value) {
			commandInterface_->set(valueHandles_, v);
		} else {
			if (valueHandles_.size() > 1) {
				std::string desc = fmt::format("Set property '{}' to {}", getPropertyPath(), v);
				commandInterface_->executeCompositeCommand([this, &v]() {
					for (auto& h : valueHandles_) {
						commandInterface_->set(h, v);
					}
				},
					desc);
			} else {
				commandInterface_->set(*valueHandles_.begin(), v);
			}
		}
	}

	void setTags(std::vector<std::string> const& tags);
	void setTags(std::vector<std::pair<std::string, int>> const& prioritizedTags);

	core::Project* project() const;
	const core::CommandInterface* commandInterface() const;
	core::CommandInterface* commandInterface();
	components::SDataChangeDispatcher dispatcher() const;
	core::EngineInterface& engineInterface() const;

	template <typename T>
	std::optional<T> as() const {
		T value = valueHandles_.begin()->as<T>();
		if (std::all_of(++valueHandles_.begin(), valueHandles_.end(), [value](auto handle) {
				return handle.template as<T>() == value;
			})) {
			return value;
		}
		return std::nullopt;
	}
	std::optional<core::SEditorObject> asRef() const;

	const std::set<core::ValueHandle>& valueHandles() noexcept;
	const QList<PropertyBrowserItem*>& children();
	PropertyBrowserItem* findNamedChild(const std::string& propertyName);
	PropertyBrowserItem* parentItem() const noexcept;
	PropertyBrowserItem* rootItem() noexcept;
	PropertyBrowserItem* siblingItem(std::string_view propertyName) const noexcept;
	PropertyBrowserRef* refItem() noexcept;
	PropertyBrowserModel* model() const noexcept;
	bool isRoot() const noexcept;
	/** locates the firstitem in hierachy which has no collapsed parent (can be this) */
	PropertyBrowserItem* findItemWithNoCollapsedParentInHierarchy() noexcept;
	bool hasCollapsedParent() const noexcept;
	/** Local expanded flag, doesn't take into account if ancestors are collapsed or not. */
	bool expanded() const noexcept;
	void setExpanded(bool expanded, bool byUser = true) noexcept;
	void setExpandedRecursively(bool expanded) noexcept;
	void restoreDefaultExpandedRecursively() noexcept;
	bool showChildren() const;

	void requestNextSiblingFocus();

	bool hasError() const;
	std::optional<core::ErrorCategory> errorCategory() const;
	core::ErrorLevel maxErrorLevel() const;
	std::string errorMessage() const;

	void markForDeletion();

	bool isValid() const;
	bool canBeChosenByColorPicker() const;
	bool isLuaProperty() const;
	bool hasSingleValue() const;
	bool isObject() const;
	bool isProperty() const;
	void getTagsInfo(std::set<std::shared_ptr<user_types::RenderPass>>& renderedBy, bool& isMultipleRenderedBy, std::set<std::shared_ptr<user_types::RenderLayer>>& addedTo, bool& isMultipleAddedTo) const;
	bool isTagContainerProperty() const;

	std::string getRootObjectName() const;
	std::string getPropertyName() const;
	std::string getPropertyPath() const;
	std::string getPropertyPathWithoutObject() const;

Q_SIGNALS:
	void linkStateChanged();
	void childrenChangedOrCollapsedChildChanged();
	void showChildrenChanged(bool show);
	void valueChanged();
	void errorChanged();
	void expandedChanged(bool expanded);
	void childrenChanged(const QList<PropertyBrowserItem*>& children);
	void editableChanged(bool editable);
	void widgetRequestFocus();

protected:
	Q_SLOT void updateLinkState() noexcept;
	Q_SLOT void onNewExpandedStateCached(const std::string& namesVector, bool expanded);

private:
	std::vector<const core::ErrorItem*> errors() const;

	void syncChildrenWithValueHandle();
	std::vector<std::set<core::ValueHandle>> getCommonValueHandles(const std::set<core::ValueHandle>& selectedItemsHandles) const;
	bool match(const core::ValueHandle& handle_1, const core::ValueHandle& handle_2) const;
	bool matchCurrent() const;

	template <class Annotation>
	bool matchAnnotation(const core::ValueHandle& left, const core::ValueHandle& right) const {
		auto leftAnno = left.constValueRef()->query<Annotation>();
		auto rightAnno = right.constValueRef()->query<Annotation>();

		if ((leftAnno == nullptr) != (rightAnno == nullptr)) {
			return false;
		}

		if (leftAnno) {
			return *leftAnno == *rightAnno;
		}
		return true;
	}

	bool allHandlesHaveSamePropName() const;
	bool isHidden(core::ValueHandle handle) const;


	PropertyBrowserItem* parentItem_{nullptr};
	PropertyBrowserRef* refItem_{nullptr};

	std::set<core::ValueHandle> valueHandles_;

	std::vector<components::Subscription> handleChangeSubs_;

	std::vector<components::Subscription> structureChangeSubs_;

	std::vector<components::Subscription> errorSubs_;
	std::map<core::ValueHandle, components::Subscription> linkValidityChangeSubs_;
	std::vector<components::Subscription> linkLifecycleStartSubs_;
	std::vector<components::Subscription> linkLifecycleEndSubs_;
	std::vector<components::Subscription> childrenChangeSubs_;
	core::CommandInterface* commandInterface_;
	components::SDataChangeDispatcher dispatcher_;
	PropertyBrowserModel* model_;
	QList<PropertyBrowserItem*> children_;
	bool expanded_;
	bool editable_ = true;
	bool locked_ = false;

	bool getDefaultExpandedFromValueHandleType(const core::ValueHandle& handle) const;
	bool getDefaultExpanded() const;
};

}  // namespace raco::property_browser
