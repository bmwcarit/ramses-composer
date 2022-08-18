/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertySubtreeChildrenContainer.h"

#include <QLabel>
#include <QPixmap>
#include <QPoint>

#include "property_browser/PropertySubtreeView.h"

namespace raco::property_browser {

PropertySubtreeChildrenContainer::PropertySubtreeChildrenContainer(PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent}, layout_{new PropertyBrowserVBoxLayout{this}} {
	layout_->setAlignment(Qt::AlignTop);
}

void PropertySubtreeChildrenContainer::setOffset(int offset) {
	layout_->setContentsMargins(offset, 0, 0, 0);
}

void PropertySubtreeChildrenContainer::addWidget(QWidget* child) {
	layout_->addWidget(child, 0, Qt::AlignTop);
}

void PropertySubtreeChildrenContainer::removeWidget(QWidget* child) {
	layout_->removeWidget(child);
}

void PropertySubtreeChildrenContainer::insertWidget(size_t index, QWidget* child) {
	layout_->insertWidget(static_cast<int>(index), child, 0, Qt::AlignTop);
}

}  // namespace raco::property_browser
