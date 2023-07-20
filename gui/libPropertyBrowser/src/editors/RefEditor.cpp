/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/RefEditor.h"

#include "property_browser/ObjectSearchView.h"
#include "common_widgets/PropertyBrowserButton.h"
#include "common_widgets/RaCoClipboard.h"
#include "core/Context.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserEditorPopup.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "style/Icons.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

namespace raco::property_browser {

using namespace style;

class RefEditorPopup : public PropertyBrowserEditorPopup {
	QMetaObject::Connection indexChangedConnection_;

public:
	class RefSearchView : public ObjectSearchView {
		core::Project* project_;
		QMetaObject::Connection itemsChangedConnection_;

	public:
		RefSearchView(const PropertyBrowserRef* ref, components::SDataChangeDispatcher dispatcher, core::Project* project, const std::set<core::ValueHandle>& objects, QWidget* parent)
			: ObjectSearchView(dispatcher, project, objects, parent), 
			project_(project),
			ref_(ref) {
			rebuild();
			updateSelection();

			itemsChangedConnection_ = QObject::connect(ref_, &PropertyBrowserRef::itemsChanged, [this](auto) {
				rebuild();
			});
		}

		~RefSearchView() {
			QObject::disconnect(itemsChangedConnection_);
		}

	protected:
		void rebuild() noexcept override {
			model_.clear();

			for (auto i = 1; i < ref_->items().size(); ++i) {
				auto ref = ref_->items()[i];
				auto obj = project_->getInstanceByID(ref.objId.toStdString());
				auto* item = new ObjectSearchViewItem{ref.objName, obj};
				model_.appendRow(item);
			}
		}

		const PropertyBrowserRef* ref_;
	};

	RefEditorPopup(PropertyBrowserItem* item, QWidget* anchor) : PropertyBrowserEditorPopup{item, anchor, new RefSearchView(item->refItem(), item->dispatcher(), item->project(), item->valueHandles(), anchor)} {
		currentRelation_.setReadOnly(true);
		deleteButton_.setFlat(true);
		deleteButton_.setIcon(Icons::instance().remove);
		update();

		indexChangedConnection_ = QObject::connect(item->refItem(), &PropertyBrowserRef::indexChanged, [this](auto) {
			update();
		});
	}

	~RefEditorPopup() {
		QObject::disconnect(indexChangedConnection_);
	}

protected:
	void establishObjectRelation() override {
		item_->refItem()->setIndex((list_->selection().row() + 1));
	}

	void removeObjectRelation() override {
		item_->refItem()->setIndex(PropertyBrowserRef::EMPTY_REF_INDEX);
	}

	void update() {
		currentRelation_.setVisible(true);
		deleteButton_.setVisible(true);

		if (item_->refItem()->hasMultipleValues()) {
			currentRelation_.setText(PropertyBrowserItem::MultipleValueText);
		} else {
			const bool hasRef = (item_->refItem()->currentIndex() != PropertyBrowserRef::EMPTY_REF_INDEX);
			if (hasRef) {
				currentRelation_.setText(item_->refItem()->items().at(item_->refItem()->currentIndex()).objName);
			} else {
				currentRelation_.setVisible(false);
				deleteButton_.setVisible(false);
			}
		}
	}
};


RefEditor::RefEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: PropertyEditor(item, parent),
	  ref_{item->refItem()} {
	auto* layout{new common_widgets::NoContentMarginsLayout<QHBoxLayout>{this}};

	changeRefButton_ = new common_widgets::PropertyBrowserButton(Icons::instance().openInNew, "", this);
	QObject::connect(changeRefButton_, &QPushButton::clicked, [this]() {
		new RefEditorPopup(item_, changeRefButton_);
	});
	layout->addWidget(changeRefButton_);

	currentRef_ = new QLineEdit{this};
	currentRef_->setReadOnly(true);
	currentRef_->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	layout->addWidget(currentRef_);

	goToRefObjectButton_ = new common_widgets::PropertyBrowserButton(Icons::instance().goTo, "", this);
	layout->addWidget(goToRefObjectButton_);

	updateRef();

	QObject::connect(currentRef_, &QLineEdit::customContextMenuRequested, this, &RefEditor::createCustomContextMenu);

	QObject::connect(goToRefObjectButton_, &QPushButton::clicked, [this, item]() {
		item->model()->Q_EMIT objectSelectionRequested(ref_->items().at(ref_->currentIndex()).objId);
	});

	QObject::connect(ref_, &PropertyBrowserRef::indexChanged, [this](auto index) {
		updateRef();
	});

	// Override the enabled behaviour of the parent class, so that the goto button can remain enabled even though the rest of the control gets disabled.
	setEnabled(true);
	currentRef_->setEnabled(item->editable());
	changeRefButton_->setEnabled(item->editable());
	QObject::disconnect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, [this]() {
		currentRef_->setEnabled(item_->editable());
		changeRefButton_->setEnabled(item_->editable());
		auto emptyReference = (ref_->currentIndex() == PropertyBrowserRef::EMPTY_REF_INDEX);
		goToRefObjectButton_->setDisabled(emptyReference || ref_->hasMultipleValues());
	});
}

void RefEditor::updateRef() {
	if (ref_->hasMultipleValues()) {
		currentRef_->setText(PropertyBrowserItem::MultipleValueText);
		currentRef_->setToolTip(PropertyBrowserItem::MultipleValueText);
	} else {
		currentRef_->setText(ref_->items().at(ref_->currentIndex()).objName);
		currentRef_->setToolTip(ref_->items().at(ref_->currentIndex()).tooltipText);
	}
	auto emptyReference = (ref_->currentIndex() == PropertyBrowserRef::EMPTY_REF_INDEX);
	goToRefObjectButton_->setDisabled(emptyReference || ref_->hasMultipleValues());
	currentRef_->update();
}


bool RefEditor::unexpectedEmptyReference() const noexcept {
	return std::any_of(item_->valueHandles().begin(), item_->valueHandles().end(), [this](const core::ValueHandle& handle) {
		return handle.asRef() == nullptr && !handle.query<core::ExpectEmptyReference>();
	});
}

void RefEditor::createCustomContextMenu(const QPoint& p) {
	if (!item_->editable()) {
		return;
	}

	auto deserialization{serialization::deserializeObjects(RaCoClipboard::get())};
	if (!deserialization) {
		return;
	}

	auto copiedObjects = core::BaseContext::getTopLevelObjectsFromDeserializedObjects(*deserialization, item_->project());

	if (copiedObjects.empty()) {
		return;
	}

	const auto& refItems = ref_->items();
	std::vector<std::pair<PropertyBrowserRef::RefItem, int>> validRefTargets;
	for (auto refIndex = 0; refIndex < refItems.size(); ++refIndex) {
		const auto& item = refItems[refIndex];
		for (const auto& copiedObj : copiedObjects) {
			if (copiedObj->objectID() == item.objId.toStdString()) {
				validRefTargets.emplace_back(item, refIndex);
				break;
			}
		}
	}

	if (validRefTargets.empty()) {
		return;
	}

	auto* lineEditMenu = new QMenu(this);

	if (validRefTargets.size() == 1) {
		lineEditMenu->setToolTipsVisible(true);
		auto* setRefAction = lineEditMenu->addAction(QString("Set Reference to Copied Object %1").arg(validRefTargets.front().first.objName), [this, &validRefTargets]() {
			ref_->setIndex(validRefTargets.front().second);
		});
		setRefAction->setToolTip(validRefTargets.front().first.tooltipText);
	} else if (validRefTargets.size() > 1) {
		auto setReferenceMenu = lineEditMenu->addMenu("Set Reference to Copied Objects...");
		setReferenceMenu->setToolTipsVisible(true);

		for (const auto& validRefTarget : validRefTargets) {
			auto* setRefAction = setReferenceMenu->addAction(validRefTarget.first.objName, [this, &validRefTarget]() {
				ref_->setIndex(validRefTarget.second);
			});
			setRefAction->setToolTip(validRefTarget.first.tooltipText);
		}
	}

	lineEditMenu->exec(currentRef_->mapToGlobal(p));
}

}  // namespace raco::property_browser
