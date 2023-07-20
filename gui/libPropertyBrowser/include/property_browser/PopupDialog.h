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

#include <QDialog>

class QApplicationStateChangeEvent;

namespace raco::property_browser {

	class PopupDialog : public QDialog {
	public:
		explicit PopupDialog(QWidget* anchor, QString title);
		void showCenteredOnAnchor();

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		QWidget* anchor_{};
	};

}

