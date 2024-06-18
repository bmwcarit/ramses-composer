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

#include "style/Colors.h"

#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>

namespace raco::common_widgets {

class PythonOutputDialog : public QDialog {
public:
	PythonOutputDialog(const QString& title, const QString& output, const QString& error, QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle(title);

		auto icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);

		auto* textEdit = new QTextEdit();
		textEdit->setReadOnly(true);

		if (!output.isEmpty()) {
			textEdit->append("Script output:\n");
			textEdit->append(output + "\n");
		}

		if (!error.isEmpty()) {
			const auto errorColorHEX = style::Colors::color(style::Colormap::errorColorLight).name();
			textEdit->append("Script error:\n");
			textEdit->append(QString("<span style=\"color: %1;\">" + error + "</span>").arg(errorColorHEX));
			icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
		}

		setWindowIcon(icon);

		auto* okButton = new QPushButton("Ok");
		connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

		auto* layout = new QVBoxLayout(this);
		layout->addWidget(textEdit);
		layout->addWidget(okButton);

		resize(600, 400);
	}
};

}  // namespace raco::common_widgets
