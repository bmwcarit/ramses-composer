/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/PreferencesView.h"

#include "utils/u8path.h"

#include "common_widgets/PropertyBrowserButton.h"
#include "core/PathManager.h"
#include "log_system/log.h"
#include "application/RaCoApplication.h"

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
			if (!dir.isEmpty()) {
				userProjectEdit_->setText(dir);
			}
		});
	}

	featureLevelEdit_ = new QSpinBox(this);
	featureLevelEdit_->setRange(raco::application::RaCoApplication::minFeatureLevel(), raco::application::RaCoApplication::maxFeatureLevel());
	featureLevelEdit_->setValue(RaCoPreferences::instance().featureLevel);
	featureLevelEdit_->setToolTip(QString::fromStdString(raco::application::RaCoApplication::featureLevelDescriptions()));
	formLayout->addRow("New Project Feature Level", featureLevelEdit_);

	QObject::connect(featureLevelEdit_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
		Q_EMIT dirtyChanged(dirty());
	});

	uriValidationCaseSensitiveCheckbox_ = new QCheckBox(this);
	uriValidationCaseSensitiveCheckbox_->setCheckState(RaCoPreferences::instance().isUriValidationCaseSensitive ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
	uriValidationCaseSensitiveCheckbox_->setToolTip("Enable to detect path case mismatch in URIs.");
	formLayout->addRow("Case-sensitive URI validation", uriValidationCaseSensitiveCheckbox_);

	QObject::connect(uriValidationCaseSensitiveCheckbox_, &QCheckBox::stateChanged, this, [this]() {
		Q_EMIT dirtyChanged(dirty());
	});
	
	preventAccidentalUpgradeCheckbox_ = new QCheckBox(this);
	preventAccidentalUpgradeCheckbox_->setCheckState(RaCoPreferences::instance().preventAccidentalUpgrade ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
	preventAccidentalUpgradeCheckbox_->setToolTip("Prevent accidental file version upgrade on save");
	formLayout->addRow("Prevent accidental upgrade", preventAccidentalUpgradeCheckbox_);

	QObject::connect(preventAccidentalUpgradeCheckbox_, &QCheckBox::stateChanged, this, [this]() {
		Q_EMIT dirtyChanged(dirty());
	});

	{
		auto* selectScreenshotDirectoryButton = new PropertyBrowserButton("  ...  ", this);

		auto container = new QWidget{this};
		auto containerLayout = new QGridLayout{container};
		screenshotDirectoryEdit_ = new QLineEdit{this};

		containerLayout->addWidget(screenshotDirectoryEdit_, 0, 0);
		containerLayout->addWidget(selectScreenshotDirectoryButton, 0, 1);
		containerLayout->setColumnStretch(0, 1);
		containerLayout->setMargin(0);

		formLayout->addRow("Screenshot Directory", container);
		screenshotDirectoryEdit_->setText(RaCoPreferences::instance().screenshotDirectory);
		QObject::connect(screenshotDirectoryEdit_, &QLineEdit::textChanged, this, [this](auto) {
			Q_EMIT dirtyChanged(dirty());
		});

		QObject::connect(selectScreenshotDirectoryButton, &QPushButton::clicked, [this]() {
			auto dir = screenshotDirectoryEdit_->text();
			dir = QFileDialog::getExistingDirectory(this, "Select Directory", dir);
			if (!dir.isEmpty()) {
				screenshotDirectoryEdit_->setText(dir);
			}
		});
	}

	auto buttonBox = new QDialogButtonBox{this};
	auto cancelButton{new QPushButton{"Close", buttonBox}};
	QObject::connect(cancelButton, &QPushButton::clicked, this, &PreferencesView::close);
	auto saveButton{new QPushButton{"Save", buttonBox}};
	saveButton->setDisabled(raco::utils::u8path(RaCoPreferences::instance().userProjectsDirectory.toStdString()).existsDirectory());
	QObject::connect(this, &PreferencesView::dirtyChanged, saveButton, &QPushButton::setEnabled);
	QObject::connect(saveButton, &QPushButton::clicked, this, &PreferencesView::save);
	buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
	buttonBox->addButton(saveButton, QDialogButtonBox::AcceptRole);
	layout->addWidget(buttonBox, Qt::AlignBottom);
}

void PreferencesView::save() {
	auto newUserProjectPath = raco::utils::u8path(userProjectEdit_->text().toStdString());
	auto newUserProjectPathString = QString::fromStdString(newUserProjectPath.string());

	if (!newUserProjectPath.existsDirectory()) {
		if(QMessageBox::question(
			this, "User Projects Directory does not exist",
				QString("The user projects directory '") + newUserProjectPathString + "' does not exist. Choose OK to create the directory or press cancel to not save the preferences.",
			QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {

			std::filesystem::create_directories(newUserProjectPath);
			if (!newUserProjectPath.existsDirectory()) {
				QMessageBox::critical(this, "Saving settings failed", "Cannot create directory: " + newUserProjectPathString);
				return;
			} 
		} else {
			return;
		}		
	} 

	RaCoPreferences& prefs{RaCoPreferences::instance()};
	
	prefs.userProjectsDirectory = newUserProjectPathString;
	prefs.featureLevel = featureLevelEdit_->value();
	prefs.isUriValidationCaseSensitive = uriValidationCaseSensitiveCheckbox_->checkState() == Qt::CheckState::Checked;
	prefs.preventAccidentalUpgrade = preventAccidentalUpgradeCheckbox_->checkState() == Qt::CheckState::Checked;
	prefs.screenshotDirectory = screenshotDirectoryEdit_->text();

	if (!prefs.save()) {
		LOG_ERROR(raco::log_system::COMMON, "Saving settings failed: {}", raco::core::PathManager::preferenceFilePath().string());
		QMessageBox::critical(this, "Saving settings failed", QString("Settings could not be saved. Check whether the application can write to its config directory.\nFile: ") + QString::fromStdString(raco::core::PathManager::preferenceFilePath().string()));
	}
	Q_EMIT dirtyChanged(false);
	
}

bool PreferencesView::dirty() {
	auto& prefs{RaCoPreferences::instance()};
	return prefs.userProjectsDirectory != userProjectEdit_->text() ||
		prefs.featureLevel != featureLevelEdit_->value() ||
		prefs.isUriValidationCaseSensitive != (uriValidationCaseSensitiveCheckbox_->checkState() == Qt::CheckState::Checked) ||
		prefs.preventAccidentalUpgrade != (preventAccidentalUpgradeCheckbox_->checkState() == Qt::CheckState::Checked) ||
		prefs.screenshotDirectory != screenshotDirectoryEdit_->text();
}

}  // namespace raco::common_widgets
