/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/RefEditor.h"

#include "common_widgets/PropertyBrowserButton.h"
#include "core/Context.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/controls/MouseWheelGuard.h"
#include "style/Colors.h"
#include "style/Icons.h"

#include <QComboBox>
#include <QEvent>
#include <QPushButton>

namespace raco::property_browser {

using namespace raco::style;

RefEditor::RefEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: PropertyEditor(item, parent),
	  ref_{item->refItem()} {
	auto* layout{new raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>{this}};
	comboBox_ = new QComboBox{this};
	comboBox_->setFocusPolicy(Qt::StrongFocus);
	comboBox_->installEventFilter(new MouseWheelGuard());
	layout->addWidget(comboBox_);

	goToRefObjectButton_ = new raco::common_widgets::PropertyBrowserButton(raco::style::Icons::instance().goTo, "", this);

	QObject::connect(goToRefObjectButton_, &QPushButton::clicked, [this, item]() {
		item->model()->Q_EMIT objectSelectionRequested(ref_->items().at(ref_->currentIndex()).objId);
	});

	layout->addWidget(goToRefObjectButton_);

	QObject::connect(ref_, &PropertyBrowserRef::indexChanged, comboBox_, &QComboBox::setCurrentIndex);
	QObject::connect(ref_, &PropertyBrowserRef::itemsChanged, this, &RefEditor::updateItems);
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::currentIndexChanged), [this](auto index) {
		emptyReference_ = (index == PropertyBrowserRef::EMPTY_REF_INDEX);
		goToRefObjectButton_->setDisabled(emptyReference_);
		comboBox_->setToolTip(comboBox_->itemData(index, Qt::ToolTipRole).toString());
	});
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		comboBox_->setFocus();
	});
	updateItems(ref_->items());

	// Override the enabled behaviour of the parent class, so that the goto button can remain enabled even though the rest of the control gets disabled.
	setEnabled(true);
	comboBox_->setEnabled(item->editable());
	goToRefObjectButton_->setEnabled(!emptyReference_);
	QObject::disconnect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, [this]() {
		comboBox_->setEnabled(item_->editable());
		goToRefObjectButton_->setEnabled(!emptyReference_);
	});
}

void RefEditor::updateItems(const PropertyBrowserRef::ComboBoxItems& items) {
	QObject::disconnect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
	comboBox_->clear();
	for (const auto& comboItem : items) {
		comboBox_->addItem(comboItem.objName, comboItem.objId);
		comboBox_->setItemData(comboBox_->count() - 1, comboItem.tooltipText, Qt::ToolTipRole);
	}
	comboBox_->setCurrentIndex(ref_->currentIndex());
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
}

bool RefEditor::unexpectedEmptyReference() const noexcept {
	return emptyReference_ && !item_->valueHandle().query<core::ExpectEmptyReference>();
}

}  // namespace raco::property_browser
