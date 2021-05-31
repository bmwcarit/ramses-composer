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

#include "StringEditor.h"

#include <QPushButton>

namespace raco::property_browser {
	
class PropertyBrowserItem;

class URIEditor : public StringEditor {
	Q_OBJECT
	Q_PROPERTY(bool updatedInBackground READ updatedInBackground);
	Q_PROPERTY(int errorLevel READ errorLevel);

public:
	explicit URIEditor(PropertyBrowserItem* item, QWidget* parent = nullptr);

	bool pathIsAbsolute();
	void switchAbsoluteRelativePath();
	void updateURIValueHandle();

protected:
	std::string createAbsolutePath();
	std::string createRelativePath();
	bool fileExists();
	void showCustomLineEditContextMenu(const QPoint& p, PropertyBrowserItem* item);
	void updateFileEditButton();
	void updateURILineEditString();
	
	PropertyBrowserItem* currentItem_;
	QPushButton* editButton_;
};

}  // namespace raco::property_browser
