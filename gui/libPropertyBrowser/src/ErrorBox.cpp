/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ErrorBox.h"

#include "core/ErrorItem.h"
#include "style/Colors.h"

namespace raco::property_browser {

using namespace raco::style;

ErrorBox::ErrorBox(const QString& content, core::ErrorLevel errorLevel, QWidget* parent) : QTextEdit{parent} {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QPalette pal(palette());
	if (errorLevel == core::ErrorLevel::INFORMATION) {
		pal.setColor(QPalette::Base, Colors::color(Colormap::grayEditDisabled));
	} else if(errorLevel == core::ErrorLevel::WARNING) {
		pal.setColor(QPalette::Base, Colors::color(Colormap::warningColor).darker());
	} else {
		pal.setColor(QPalette::Base, Colors::color(Colormap::errorColorDark).darker());
	}
	pal.setColor(QPalette::Text, Colors::color(Colormap::textDisabled));
	setPalette(pal);
	setReadOnly(true);

	QObject::connect(this, &QTextEdit::textChanged, this, [this]() { update(); });
	setPlainText(content);
}

QSize ErrorBox::sizeHint() const {
	return document()->size().toSize();
}

void ErrorBox::updateContent(const QString& content) {
	setPlainText(content);
	update();
}

void ErrorBox::paintEvent(QPaintEvent* event) {
	if (size().height() != sizeHint().height()) {
		setMinimumHeight(sizeHint().height());
		setMaximumHeight(sizeHint().height());
		updateGeometry();
	}
	QTextEdit::paintEvent(event);
}

}  // namespace raco::property_browser
