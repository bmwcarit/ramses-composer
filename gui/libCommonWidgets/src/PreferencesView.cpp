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
		auto edit{new QLineEdit{this}};

		containerLayout->addWidget(edit, 0, 0);
		containerLayout->addWidget(selectDirectoryButton, 0, 1);
		containerLayout->setColumnStretch(0, 1);
		containerLayout->setMargin(0);

		formLayout->addRow("User Projects Directory", container);
		edit->setText(RaCoPreferences::instance().userProjectsDirectory);
		QObject::connect(edit, &QLineEdit::textChanged, this, [this](auto) { Q_EMIT dirtyChanged(dirty()); });
		stringEdits_.push_back(std::make_pair(&RaCoPreferences::userProjectsDirectory, edit));

		QObject::connect(selectDirectoryButton, &QPushButton::clicked, [this, edit]() {
			auto dir = edit->text();
			dir = QFileDialog::getExistingDirectory(this, "Select Directory", dir);
			if (dir.size() > 0) {
				edit->setText(dir);
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
	for (const auto& edit : stringEdits_) {
		if (raco::utils::path::isExistingDirectory(edit.second->text().toStdString())) {
			RaCoPreferences::instance().*(edit.first) = edit.second->text();
		}
	}
	RaCoPreferences::instance().save();
	Q_EMIT dirtyChanged(false);
}

bool PreferencesView::dirty() {
	for (const auto& edit : stringEdits_) {
		if (RaCoPreferences::instance().*(edit.first) != edit.second->text())
			return true;
	}
	return false;
}

}  // namespace raco::common_widgets
