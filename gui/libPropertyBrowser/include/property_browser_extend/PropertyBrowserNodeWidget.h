/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QObject>

#include "property_browser/PropertyBrowserLayouts.h"


namespace raco::property_browser {
class PropertyBrowserNodeWidget : public QWidget {
    Q_OBJECT
public:
    PropertyBrowserNodeWidget(QString labelName, QWidget *parent = nullptr, bool visible = false);

    void setShowWidget(QWidget* view);

Q_SIGNALS:
    void updateIcon(bool bExpanded);
    void insertData();
    void removeData();

public Q_SLOTS:
    void expandedWidget() noexcept;
    void addButtonClicked();
    void delButtonClicked();
private:
    QWidget* title_{nullptr};
    QWidget* view_{nullptr};
    QWidget* button_{nullptr};
    QLabel* label_{nullptr};
    QPushButton* addButton_{nullptr};
    QPushButton* delButton_{nullptr};
    QString labelName_;
    bool bExpanded{false};
};

}  // namespace raco::property_browser
