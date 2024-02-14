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

#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

class PropertyCopyPaste {
public:
	static bool canCopyValue(PropertyBrowserItem* item);
	static bool canPasteValue(PropertyBrowserItem* item);

	static void pasteValue(PropertyBrowserItem* item);
	static void copyValue(PropertyBrowserItem* item);

	static void copyValuePlainText(PropertyBrowserItem* item);
	static void copyChildValuePlainText(PropertyBrowserItem* item, int index);

	// Used only in unit tests
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
	static void copyValuePlainText(const core::ValueBase* valueBase);
};

}  // namespace raco::property_browser