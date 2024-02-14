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

#include "core/EditorObject.h"

#include "property_browser/PropertyBrowserItem.h"

#include <QWidget>

namespace raco {
namespace core {
class Project;
}
}

class QCheckBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace raco::property_browser {
class ColorChannelListWidget;
class ImageWidget;

class TexturePreviewWidget : public QWidget {
	Q_OBJECT

public:
	explicit TexturePreviewWidget(PropertyBrowserItem* item, QWidget* parent = nullptr);

	void setFiles(std::vector<std::pair<QString, QString>> files);
	void setImageWidget(ImageWidget* widget);
	static std::vector<std::pair<QString, QString>> collectValidTextureUris(const core::Project* project, const core::Errors& errors, const core::SEditorObject& texture);

private slots:
	void levelSelected(int);

private:
	QImage getProcessedImage() const;
	void refreshPreview() const;

	PropertyBrowserItem* item_;

	QImage currentImage_;
	ImageWidget* imageWidget_ = nullptr;
	ColorChannelListWidget* channelList_ = nullptr;
	QCheckBox* checkerCheck_ = nullptr;
	QComboBox* uriCombo_ = nullptr;

	components::Subscription previewSubscription_;
	components::Subscription mipMapLevelSubscription_;
};

}  // namespace raco::property_browser
