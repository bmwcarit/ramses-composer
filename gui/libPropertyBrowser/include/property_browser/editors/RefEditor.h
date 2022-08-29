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

#include "property_browser/PropertyBrowserRef.h"

#include "PropertyEditor.h"

class QPushButton;
class QLineEdit;

namespace raco::property_browser {
class PropertyBrowserItem;

class RefEditor final : public PropertyEditor {
	Q_OBJECT
	Q_PROPERTY(bool unexpectedEmptyReference READ unexpectedEmptyReference);

public:
	explicit RefEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);
	bool unexpectedEmptyReference() const noexcept;

protected:
	bool emptyReference_ = false;

	PropertyBrowserRef* ref_{nullptr};
	QLineEdit* currentRef_{nullptr};
	QPushButton* changeRefButton_{nullptr};
	QPushButton* goToRefObjectButton_{nullptr};
protected Q_SLOTS:
	void createCustomContextMenu(const QPoint& p);
};

}  // namespace raco::property_browser
