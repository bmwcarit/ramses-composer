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
	: QWidget{parent},
      item_{item},
	  ref_{item->refItem()} {
	auto* layout{new raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>{this}};
	comboBox_ = new QComboBox{this};
	comboBox_->setFocusPolicy(Qt::StrongFocus);
	comboBox_->installEventFilter(new MouseWheelGuard());
	layout->addWidget(comboBox_);

	goToRefObjectButton_ = new raco::common_widgets::PropertyBrowserButton(raco::style::Icons::icon(raco::style::Pixmap::go_to, this), "", this);

	QObject::connect(goToRefObjectButton_, &QPushButton::clicked, [this, item]() {
		item->model()->Q_EMIT objectSelectionRequested(ref_->items().at(ref_->currentIndex()).second);
	});

	layout->addWidget(goToRefObjectButton_);

	QObject::connect(ref_, &PropertyBrowserRef::indexChanged, comboBox_, &QComboBox::setCurrentIndex);
	QObject::connect(ref_, &PropertyBrowserRef::itemsChanged, this, &RefEditor::updateItems);
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::currentIndexChanged), [this](auto index) {
		emptyReference_ = (index == PropertyBrowserRef::EMPTY_REF_INDEX);
		goToRefObjectButton_->setDisabled(index == PropertyBrowserRef::EMPTY_REF_INDEX);
	});
	updateItems(ref_->items());
}

void RefEditor::updateItems(const PropertyBrowserRef::ComboBoxItems& items) {
	QObject::disconnect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
	comboBox_->clear();
	for (const auto& comboItem : items) {
		comboBox_->addItem(comboItem.first, comboItem.second);
	}
	comboBox_->setCurrentIndex(ref_->currentIndex());
	QObject::connect(comboBox_, qOverload<int>(&QComboBox::activated), ref_, &PropertyBrowserRef::setIndex);
}

void RefEditor::changeEvent(QEvent* event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::EnabledChange) {
		// retroactively set the RefEditor enabled at all times and only disable/enable the Ref combo box
		// because QWidgets have to be enabled for their child widgets to be enabled and we want to enable the goto button at all times
		auto enabled = this->isEnabled();
		this->setEnabled(true);

		comboBox_->setEnabled(enabled);
		goToRefObjectButton_->setEnabled(!emptyReference_);
	}
}

bool RefEditor::unexpectedEmptyReference() const noexcept {
	return emptyReference_ && !item_->valueHandle().query<core::ExpectEmptyReference>();
}

}  // namespace raco::property_browser
