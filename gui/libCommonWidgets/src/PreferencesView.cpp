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

#include "utils/PathUtils.h"

#include "common_widgets/PropertyBrowserButton.h"
#include "core/PathManager.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
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

	imageEdit_ = new QLineEdit{this};
	formLayout->addRow("Image subdirectory", imageEdit_);
	imageEdit_->setText(RaCoPreferences::instance().imageSubdirectory);
	QObject::connect(imageEdit_, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });

	meshEdit_ = new QLineEdit{this};
	formLayout->addRow("Mesh subdirectory", meshEdit_);
	meshEdit_->setText(RaCoPreferences::instance().meshSubdirectory);
	QObject::connect(meshEdit_, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });

	scriptEdit_ = new QLineEdit{this};
	formLayout->addRow("Script subdirectory", scriptEdit_);
	scriptEdit_->setText(RaCoPreferences::instance().scriptSubdirectory);
	QObject::connect(scriptEdit_, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });

	shaderEdit_ = new QLineEdit{this};
	formLayout->addRow("Shader subdirectory", shaderEdit_);
	shaderEdit_->setText(RaCoPreferences::instance().shaderSubdirectory);
	QObject::connect(shaderEdit_, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });

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
	if (raco::utils::path::isExistingDirectory(userProjectEdit_->text().toStdString())) {
			RaCoPreferences::instance().userProjectsDirectory = userProjectEdit_->text();
	}
	RaCoPreferences::instance().imageSubdirectory = imageEdit_->text();
	RaCoPreferences::instance().meshSubdirectory = meshEdit_->text();
	RaCoPreferences::instance().scriptSubdirectory = scriptEdit_->text();
	RaCoPreferences::instance().shaderSubdirectory = shaderEdit_->text();
	RaCoPreferences::instance().save();
	Q_EMIT dirtyChanged(false);
}

bool PreferencesView::dirty() {
	return RaCoPreferences::instance().userProjectsDirectory != userProjectEdit_->text() || 
		RaCoPreferences::instance().imageSubdirectory != imageEdit_->text() || 
		RaCoPreferences::instance().meshSubdirectory != meshEdit_->text() ||
		RaCoPreferences::instance().scriptSubdirectory != scriptEdit_->text() ||
		RaCoPreferences::instance().shaderSubdirectory != shaderEdit_->text();
}

}  // namespace raco::common_widgets
