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

#include "core/Handles.h"
#include "core/Project.h"
#include <QWidget>
#include <iostream>

namespace raco::property_browser {

class PropertyBrowserItem;

class PropertyEditor : public QWidget {
public:
	explicit PropertyEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);

	bool canCopyValue();
	bool canPasteValue();

	void pasteValue();
	virtual void copyValue();

protected:
	PropertyBrowserItem* item_;

	// Made protected to enable unit testing:
	static void pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value);

private:
	static void pasteInt(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteInt64(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteDouble(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteBool(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteString(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteRef(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteStruct(PropertyBrowserItem* item, core::ValueBase* value);
	static void pasteTable(PropertyBrowserItem* item, core::ValueBase* value);

	static bool isVector(PropertyBrowserItem* item);
	static bool isVector(core::ValueBase* value);
	template <typename T>
	static void pasteVector(PropertyBrowserItem* item, core::ValueBase* value);
	static void pastePropertyOfSameType(PropertyBrowserItem* item, data_storage::ValueBase* value);
};

}  // namespace raco::property_browser
