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

#include "PropertyEditor.h"

namespace raco::property_browser {

class ImageWidget;
class PropertyBrowserItem;

class TexturePreviewEditor : public PropertyEditor {

public:
	explicit TexturePreviewEditor(PropertyBrowserItem* item, QWidget* label, QWidget* parent = nullptr);

	ImageWidget* getImageWidget() const;

private:
	ImageWidget* imageWidget_;
};

}  // namespace raco::property_browser
