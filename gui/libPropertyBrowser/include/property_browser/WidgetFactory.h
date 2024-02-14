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

#include <QLabel>

namespace raco::property_browser {
class PropertyBrowserItem;
class LinkEditor;
class PropertyEditor;

/**
 * Helper factory for create property related widgets (e.g. Label Widget, Control Widget, Link Widget).
 * If necessary this should be made into a true factory, or singleton.
 */
class WidgetFactory {
public:
	static std::tuple<QWidget*, LinkEditor*> createPropertyWidgets(PropertyBrowserItem* item, QWidget* parent);

private:
	static PropertyEditor* createPropertyEditor(PropertyBrowserItem* item, QWidget* label, QWidget* parent);
	static QWidget* createPropertyLabel(PropertyBrowserItem* item, QWidget* parent);
	static LinkEditor* createLinkControl(PropertyBrowserItem* item, QWidget* parent, QWidget* propertyEditor);
	static PropertyEditor* createPropertyControl(PropertyBrowserItem* item, QWidget* label, QWidget* parent);
};
}  // namespace raco::property_browser