/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser_extend//PropertyBrowserNodeWidget.h"
#include "property_browser/controls/ExpandButton.h"
#include "property_browser_extend/controls/ExpandControlNoItemButton.h"

namespace raco::property_browser {
PropertyBrowserNodeWidget::PropertyBrowserNodeWidget(QString labelName, QWidget *parent, bool visible)
    : labelName_{labelName}, QWidget{parent} {
    title_ = new QWidget{this};
    auto* hLayout = new PropertyBrowserHBoxLayout{title_};
    button_ = new ExpandControlNoItemButton{title_, this};
    button_->setFixedWidth(28);
    label_ = new QLabel(labelName_, title_);
    hLayout->addWidget(button_, 0);
    hLayout->addWidget(label_, 0);

    if (visible) {
        addButton_ = new QPushButton{QString("+"), title_};
        delButton_ = new QPushButton{QString("-"), title_};
        addButton_->setFlat(true);
        delButton_->setFlat(true);
        addButton_->setFixedWidth(28);
        delButton_->setFixedWidth(28);
        hLayout->addWidget(delButton_, Qt::AlignRight);
        hLayout->addWidget(addButton_, Qt::AlignRight);
        QObject::connect(addButton_, &QPushButton::clicked, this, &PropertyBrowserNodeWidget::addButtonClicked);
        QObject::connect(delButton_, &QPushButton::clicked, this, &PropertyBrowserNodeWidget::delButtonClicked);
    }

    title_->setLayout(hLayout);
    title_->setStyleSheet("QWidget{background-color:#5d5d5d;}");
}

void PropertyBrowserNodeWidget::setShowWidget(QWidget *view) {
    if (view != nullptr) {
        view_ = view;
        view_->hide();
        auto* vLayout = new PropertyBrowserVBoxLayout{this};
        vLayout->addWidget(title_, 0);
        vLayout->addWidget(view_, 1);

        setLayout(vLayout);
    }
}

void PropertyBrowserNodeWidget::expandedWidget() noexcept{
    if (view_ == nullptr) {
        return;
    }
    bExpanded = !bExpanded;
    if(bExpanded) {
        view_->show();
    }
    else {
        view_->hide();
    }
    Q_EMIT updateIcon(bExpanded);
    update();
}

void PropertyBrowserNodeWidget::addButtonClicked() {
    Q_EMIT insertData();
}

void PropertyBrowserNodeWidget::delButtonClicked() {
    Q_EMIT removeData();
}
}
