/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/RaCoClipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

void raco::RaCoClipboard::set(const std::string& content, const char* mimeType) {
	QMimeData* mimeData = new QMimeData();
	mimeData->setData("text/plain", content.c_str());
	mimeData->setData(mimeType, content.c_str());
	QApplication::clipboard()->setMimeData(mimeData);
}

bool raco::RaCoClipboard::hasEditorObject() {
	return QApplication::clipboard()->mimeData()->formats().contains(MimeTypes::OBJECTS);
}

std::string raco::RaCoClipboard::get(const char* mimeType) {
	if (QApplication::clipboard()->mimeData()->formats().contains(mimeType)) {
		auto byteArray = QApplication::clipboard()->mimeData()->data(mimeType);
		return byteArray.toStdString();
	}

	return {};
}
