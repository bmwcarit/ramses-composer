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

#include "common_widgets/NoContentMarginsLayout.h"
#include "ColorChannelInfo.h"

#include <QHBoxLayout>
#include <QWidget>

class QCheckBox;

namespace raco::property_browser {

class ColorChannelListWidget : public QWidget {
	Q_OBJECT

public:
	explicit ColorChannelListWidget(QWidget* parent = nullptr);
	void refreshChannelList(const QImage& image);
	std::tuple<bool, bool, ColorChannels::ChannelOperations> getSelectedOperations() const;

Q_SIGNALS:
	void selectionChanged();

private:
	QCheckBox* checkbox(size_t index) const;
	bool isChannelPresent(size_t index) const;

	// Checkbox and its color channel presence flag
	std::vector<std::pair<QCheckBox*, bool>> checkBoxes_;
	common_widgets::NoContentMarginsLayout<QVBoxLayout> layout_;
};

}  // namespace raco::property_browser