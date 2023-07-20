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

#include "StringEditor.h"
#include "core/PathManager.h"
#include <QPushButton>

namespace raco::property_browser {
	
class PropertyBrowserItem;

class URIEditor : public StringEditor {
	Q_OBJECT
	Q_PROPERTY(bool updatedInBackground READ updatedInBackground);
	Q_PROPERTY(int errorLevel READ errorLevel);

public:
	explicit URIEditor(PropertyBrowserItem* item, QWidget* parent = nullptr);

protected:
	// Returns 
	// - the unique absolute path of all the handles if the absolute paths of the individual handles are 
	//   all non-empty and equal.
	// - nullopt otherwise.
	std::optional<std::string> uniqueAbsolutePath();

	// Returns true if the uniquAbsolutePath is not nullopt and the corresponding file exists
	bool fileUniqueAndExists();

	void showCustomLineEditContextMenu(const QPoint& p, PropertyBrowserItem* item);
	void updateFileEditButton();
	void setFromFileDialog(const QString& file);
	void switchToAbsolutePath();
	void switchToRelativePath();

	bool canSwitchToAbsolutePath(const core::ValueHandle& handle);
	bool canSwitchToRelativePath(const core::ValueHandle& handle);

	bool switchToAbsolutePathEnabled();
	bool switchToRelativePathEnabled();

	QPushButton* editButton_;

private:
	void setItemValue(const QString& file);
};

}  // namespace raco::property_browser
