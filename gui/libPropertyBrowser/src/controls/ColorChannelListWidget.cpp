/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/controls/ColorChannelListWidget.h"
#include "property_browser/controls/ColorChannelInfo.h"

#include <QCheckBox>
#include <QGuiApplication>

namespace raco::property_browser {

ColorChannelListWidget::ColorChannelListWidget(QWidget* parent) : QWidget(parent), layout_(this) {
	for (size_t i = 0; i < ColorChannels::channels_.size(); ++i) {
		auto checkBox = new QCheckBox(ColorChannels::channels_[i].displayName_.c_str());
		checkBox->setCheckState(Qt::Checked);

		connect(checkBox, &QCheckBox::stateChanged, this, [this]() {
			Q_EMIT selectionChanged();
		});

		layout_.addWidget(checkBox);
		checkBoxes_.emplace_back(checkBox, false);
	}

	setLayout(&layout_);
}

QCheckBox* ColorChannelListWidget::checkbox(size_t index) const {
	return checkBoxes_[index].first;
}

bool ColorChannelListWidget::isChannelPresent(size_t index) const {
	return checkBoxes_[index].second;
}

void ColorChannelListWidget::refreshChannelList(const QImage& image) {
	for (size_t i = 0; i < checkBoxes_.size(); ++i) {
		const auto imageHasChannel = ColorChannels::channels_[i].channelExistsPredicate_(image);
		checkbox(i)->setCheckState(Qt::Checked);
		checkbox(i)->setVisible(imageHasChannel);
		checkBoxes_[i].second = imageHasChannel;
	}
}

std::tuple<bool, bool, ColorChannels::ChannelOperations> ColorChannelListWidget::getSelectedOperations() const {
	bool isAlphaEnabled{false}, isSomeColorEnabled{false};

	// Collect enabled channels
	std::list<std::function<int(QRgb)>> operations;
	for (size_t i = 0; i < checkBoxes_.size(); ++i) {
		// Take only visible checkboxes state into account
		if (checkbox(i)->checkState() == Qt::Checked && isChannelPresent(i)) {
			auto title = checkbox(i)->text();
			if (title == "Alpha") {
				isAlphaEnabled = true;
			} else {
				isSomeColorEnabled = true;
			}

			// Collect enabled channel operations
			operations.emplace_back(ColorChannels::channels_[i].extractFunction_);
		}
	}

	return {isAlphaEnabled, isSomeColorEnabled, operations};
}

}  // namespace raco::property_browser