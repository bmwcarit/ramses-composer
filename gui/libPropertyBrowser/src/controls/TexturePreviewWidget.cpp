/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#include "property_browser/controls/TexturePreviewWidget.h"
#include "property_browser/controls/ColorChannelListWidget.h"
#include "property_browser/controls/ImageWidget.h"
#include "common_widgets/NoContentMarginsLayout.h"

#include "core/Errors.h"
#include "user_types/Texture.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGuiApplication>
#include <QListWidget>
#include <QVBoxLayout>

namespace raco::property_browser {

TexturePreviewWidget::TexturePreviewWidget(PropertyBrowserItem* item, QWidget* parent) : QWidget(parent), item_(item) {
	assert(item->valueHandles().size() == 1);
	
	const auto verticalLayout = new common_widgets::NoContentMarginsLayout<QVBoxLayout>(parent);

	uriCombo_ = new QComboBox(this);
	connect(uriCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TexturePreviewWidget::levelSelected);
	verticalLayout->addWidget(uriCombo_);

	channelList_ = new ColorChannelListWidget(this);
	connect(channelList_, &ColorChannelListWidget::selectionChanged, this, [this]() {
		refreshPreview();
	});
	verticalLayout->addWidget(channelList_);

	checkerCheck_ = new QCheckBox("Checker Background", this);
	checkerCheck_->setCheckState(Qt::CheckState::Checked);
	connect(checkerCheck_, &QCheckBox::stateChanged, this, [this]() {
		if (imageWidget_) {
			imageWidget_->setCheckerEnabled(checkerCheck_->isChecked());
		}
	});
	verticalLayout->addWidget(checkerCheck_);

	// Allow widget to stretch down
	verticalLayout->addStretch(1);

	setLayout(verticalLayout);
	refreshPreview();

	const auto textureObject{item->valueHandles().begin()->rootObject()};
	previewSubscription_ = item->dispatcher()->registerOnPreviewDirty(textureObject, [this, item, textureObject]() {
		setFiles(TexturePreviewWidget::collectValidTextureUris(item->project(), item->commandInterface()->errors(), textureObject));
	});
	mipMapLevelSubscription_ = item->dispatcher()->registerOn({textureObject, &user_types::Texture::mipmapLevel_}, [this, item, textureObject]() {
		setFiles(TexturePreviewWidget::collectValidTextureUris(item->project(), item->commandInterface()->errors(), textureObject));
	});
}

void TexturePreviewWidget::levelSelected(int) {
	const auto file = uriCombo_->currentData().toString();
	currentImage_.load(file);
	channelList_->refreshChannelList(currentImage_);
	refreshPreview();
}

QImage TexturePreviewWidget::getProcessedImage() const {
	auto processedImage = QImage(currentImage_.size(), QImage::Format_ARGB32);
	auto [isAlphaEnabled, isSomeColorEnabled, operations] = channelList_->getSelectedOperations();

	// Apply operations
	for (int y = 0; y < currentImage_.size().height(); ++y) {
		for (int x = 0; x < currentImage_.size().width(); ++x) {
			QRgb pixel = 0;
			for (const auto& op: operations) {
				pixel += op(currentImage_.pixel(x, y));
			}
			if (!isAlphaEnabled && isSomeColorEnabled) {
				// Set opaque alpha to make color components visible
				pixel = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 255);
			} else if (!isSomeColorEnabled && isAlphaEnabled) {
				// Make Alpha visible by setting color to white
				pixel = qRgba(255, 255, 255, qAlpha(pixel));
			}
			processedImage.setPixel(x, y, pixel);
		}
	}

	return processedImage;
}

void TexturePreviewWidget::refreshPreview() const {
	if (imageWidget_) {
		imageWidget_->setPixmap(QPixmap::fromImage(getProcessedImage()));
	}
}

void TexturePreviewWidget::setFiles(std::vector<std::pair<QString, QString>> files) {
	{
		const QSignalBlocker blocker{uriCombo_};
		const auto oldText = uriCombo_->currentText();
		uriCombo_->clear();
		for (auto [caption, path] : files) {
			uriCombo_->addItem(caption, {path});
		}
		const auto index = uriCombo_->findText(oldText);
		if (index != -1) {
			uriCombo_->setCurrentIndex(index);
		}
	}

	// Load uri selected in combo
	const auto currentIndex{uriCombo_->currentIndex()};
	if (!files.empty() && currentIndex >= 0 && currentIndex < static_cast<int>(files.size())) {
		currentImage_.load(files[currentIndex].second);
	} else {
		currentImage_ = QImage{};
	}

	channelList_->refreshChannelList(currentImage_);
	refreshPreview();
}

void TexturePreviewWidget::setImageWidget(ImageWidget* widget) {
	imageWidget_ = widget;

	const auto textureObject{item_->valueHandles().begin()->rootObject()};
	setFiles(TexturePreviewWidget::collectValidTextureUris(item_->project(), item_->commandInterface()->errors(), textureObject));
}

std::vector<std::pair<QString, QString>> TexturePreviewWidget::collectValidTextureUris(const core::Project* project, const core::Errors& errors, const core::SEditorObject& texture) {
	std::vector<std::pair<QString, QString>> uris;
	const auto mipmapLevelCount = core::ValueHandle{texture, &user_types::Texture::mipmapLevel_}.asInt();
	int currentLevel = 0;

	// Collect only valid URIs
	static std::list<std::string> uriProperties{"uri", "level2uri", "level3uri", "level4uri"};
	for (const auto& property : uriProperties) {
		if (currentLevel++ == mipmapLevelCount) {
			break;
		}
		const auto uriHandle = core::ValueHandle{texture, {property}};
		const auto absPath = core::PathQueries::resolveUriPropertyToAbsolutePath(*project, uriHandle);
		if (!absPath.empty() && (!errors.hasError(uriHandle) || errors.getError(uriHandle).level() < core::ErrorLevel::ERROR)) {
			std::string displayName = property;
			if (auto displayNameAnnotation = uriHandle.query<core::DisplayNameAnnotation>()) {
				displayName = displayNameAnnotation->name_.asString();
			}
			uris.emplace_back(QString::fromStdString(displayName), QString::fromStdString(absPath));
		}
	}

	return uris;
}

}  // namespace raco::common_widgets
