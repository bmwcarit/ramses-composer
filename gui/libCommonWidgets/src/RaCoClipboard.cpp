/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/RaCoClipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

void raco::RaCoClipboard::set(const std::string& content) {
	QMimeData* mimeData = new QMimeData();
	mimeData->setData("text/plain", content.c_str());
	mimeData->setData(MimeTypes::EDITOR_OBJECT_CLIPBOARD, content.c_str());
	QApplication::clipboard()->setMimeData(mimeData);
}

bool raco::RaCoClipboard::hasEditorObject() {
	return QApplication::clipboard()->mimeData()->formats().contains(MimeTypes::EDITOR_OBJECT_CLIPBOARD);
}

std::string raco::RaCoClipboard::get() {
	if (hasEditorObject()) {
		auto byteArray = QApplication::clipboard()->mimeData()->data(MimeTypes::EDITOR_OBJECT_CLIPBOARD);
		return byteArray.toStdString();
	} else {
		return QApplication::clipboard()->text().toStdString();
	}
}
