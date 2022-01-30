/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/PreferencesView.h"

#include "utils/u8path.h"

#include "common_widgets/PropertyBrowserButton.h"
#include "core/PathManager.h"
#include "log_system/log.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace raco::common_widgets {

using RaCoPreferences = raco::components::RaCoPreferences;

PreferencesView::PreferencesView(QWidget* parent) : QDialog{parent} {
	auto layout = new QVBoxLayout{this};
	auto form = new QWidget{this};
	layout->addWidget(form, Qt::AlignTop);
	auto formLayout = new QFormLayout{form};
	setAttribute(Qt::WA_DeleteOnClose);
	{
		auto* selectDirectoryButton = new PropertyBrowserButton("  ...  ", this);

		auto container = new QWidget{this};
		auto containerLayout = new QGridLayout{container};
		userProjectEdit_ = new QLineEdit{this};

		containerLayout->addWidget(userProjectEdit_, 0, 0);
		containerLayout->addWidget(selectDirectoryButton, 0, 1);
		containerLayout->setColumnStretch(0, 1);
		containerLayout->setMargin(0);

		formLayout->addRow("User Projects Directory", container);
		userProjectEdit_->setText(RaCoPreferences::instance().userProjectsDirectory);
		QObject::connect(userProjectEdit_, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });

		QObject::connect(selectDirectoryButton, &QPushButton::clicked, [this]() {
			auto dir = userProjectEdit_->text();
			dir = QFileDialog::getExistingDirectory(this, "Select Directory", dir);
			if (dir.size() > 0) {
				userProjectEdit_->setText(dir);
			}
		});
	}

	auto buttonBox = new QDialogButtonBox{this};
	auto cancelButton{new QPushButton{"Close", buttonBox}};
	QObject::connect(cancelButton, &QPushButton::clicked, this, &PreferencesView::close);
	auto saveButton{new QPushButton{"Save", buttonBox}};
	saveButton->setDisabled(true);
	QObject::connect(this, &PreferencesView::dirtyChanged, saveButton, &QPushButton::setEnabled);
	QObject::connect(saveButton, &QPushButton::clicked, this, &PreferencesView::save);
	buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
	buttonBox->addButton(saveButton, QDialogButtonBox::AcceptRole);
	layout->addWidget(buttonBox, Qt::AlignBottom);
}

void PreferencesView::save() {
	if (raco::utils::u8path(userProjectEdit_->text().toStdString()).existsDirectory()) {
			RaCoPreferences::instance().userProjectsDirectory = userProjectEdit_->text();
	}
	if (!RaCoPreferences::instance().save()) {
		LOG_ERROR(raco::log_system::COMMON, "Saving settings failed: {}", raco::core::PathManager::preferenceFilePath().string());
		QMessageBox::critical(this, "Saving settings failed", QString("Settings could not be saved. Check whether the application can write to its config directory.\nFile: ") 
			+ QString::fromStdString(raco::core::PathManager::preferenceFilePath().string()));
	}
	Q_EMIT dirtyChanged(false);
}

bool PreferencesView::dirty() {
	return RaCoPreferences::instance().userProjectsDirectory != userProjectEdit_->text();
}

}  // namespace raco::common_widgets
