/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/URIEditor.h"

#include "common_widgets/PropertyBrowserButton.h"


#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFocusEvent>
#include <QMenu>
#include <QObject>
#include <QPalette>
#include <QPushButton>

#include "style/Icons.h"

#include "utils/u8path.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

using PathManager = core::PathManager;

URIEditor::URIEditor(PropertyBrowserItem* item, QWidget* parent) : StringEditor(item, parent) {
	auto* loadFileButton = new raco::common_widgets::PropertyBrowserButton("  ...  ", this);

	QObject::connect(loadFileButton, &QPushButton::clicked, [this]() {
		auto uriAnno = item_->query<core::URIAnnotation>();
		std::string cachedPath;
		auto projectAbsPath = item_->project()->currentFolder();
		auto cachedPathKey = raco::core::PathManager::getCachedPathKeyCorrespondingToUserType(item_->valueHandle().rootObject()->getTypeDescription());

		if (fileExists()) {
			auto fileInfo = QFileInfo(QString::fromStdString(raco::utils::u8path(lineEdit_->text().toStdString()).normalizedAbsolutePath(projectAbsPath).string()));
			QDir uriDir = fileInfo.absoluteDir();
			cachedPath = uriDir.absolutePath().toStdString();
		} else {
			cachedPath = raco::core::PathManager::getCachedPath(cachedPathKey).string();
		}

		auto fileFilter = QString();
		auto fileMode = QFileDialog::FileMode::Directory;
		if (!uriAnno->isProjectSubdirectoryURI()) {
			fileFilter = uriAnno->filter_.asString().c_str();
			fileMode = QFileDialog::FileMode::AnyFile;
		}

		QFileDialog* dialog = new QFileDialog(this, tr("Choose URI"), QString::fromStdString(raco::utils::u8path(cachedPath).existsDirectory() ? cachedPath : projectAbsPath), fileFilter);
		if (uriAnno->isProjectSubdirectoryURI()) {
			dialog->setOption(QFileDialog::ShowDirsOnly, true);
		}
		dialog->setFileMode(fileMode);
		connect(dialog, &QFileDialog::fileSelected, [this, cachedPathKey](const QString& file) {
			auto oldUriWasAbsolute = pathIsAbsolute();
			if (oldUriWasAbsolute) {
				item_->set(file.toStdString());
			} else {
				item_->set(raco::utils::u8path(file.toStdString()).normalizedRelativePath(item_->project()->currentFolder()).string());
			}

			QDir dir = QFileInfo(file).absoluteDir();
			raco::core::PathManager::setCachedPath(cachedPathKey, dir.absolutePath().toStdString());
		});
		dialog->exec();
	});
	layout()->addWidget(loadFileButton);

	editButton_ = new raco::common_widgets::PropertyBrowserButton(raco::style::Icons::instance().openInNew, "", this);
	editButton_->setMaximumWidth(raco::common_widgets::PropertyBrowserButton::MAXIMUM_WIDTH_PX + 5);
	editButton_->setEnabled(fileExists());
	connect(editButton_, &QPushButton::clicked, [this]() {
		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(createAbsolutePath())));
	});
	layout()->addWidget(editButton_);

	lineEdit_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(lineEdit_, &QLineEdit::customContextMenuRequested, [this, item](const QPoint& p) { showCustomLineEditContextMenu(p, item); });

	connect(item_, &PropertyBrowserItem::valueChanged, this, [this, item](core::ValueHandle& handle) {
		updateURILineEditString();
		updateFileEditButton();
	});

	updateFileEditButton();

	// Override the enabled behaviour of the parent class, so that the open with button can remain enabled even though the rest of the control gets disabled.
	setEnabled(true);
	loadFileButton->setEnabled(item->editable());
	lineEdit_->setEnabled(item->editable());
	QObject::disconnect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, [loadFileButton, this]() {
		loadFileButton->setEnabled(item_->editable());
		lineEdit_->setEnabled(item_->editable()); 
	});
}

void URIEditor::showCustomLineEditContextMenu(const QPoint& p, PropertyBrowserItem* item) {
	QMenu* contextMenu = lineEdit_->createStandardContextMenu();

	QAction* absoluteRelativeURISwitch = new QAction("Use Absolute Path", this);
	auto URIisOnDifferentPartition = pathIsAbsolute() && !raco::utils::u8path::areSharingSameRoot(lineEdit_->text().toStdString(), item->project()->currentPath());
	absoluteRelativeURISwitch->setDisabled(URIisOnDifferentPartition);
	absoluteRelativeURISwitch->setCheckable(true);
	absoluteRelativeURISwitch->setChecked(pathIsAbsolute() || URIisOnDifferentPartition);

	connect(absoluteRelativeURISwitch, &QAction::triggered, [this]() {
		switchAbsoluteRelativePath();
	});
	contextMenu->insertSeparator(contextMenu->actions().front());
	contextMenu->insertAction(contextMenu->actions().front(), absoluteRelativeURISwitch);

	contextMenu->exec(mapToGlobal(p));
}

void URIEditor::updateFileEditButton() {
	editButton_->setEnabled(fileExists());
}

void URIEditor::updateURILineEditString() {
	lineEdit_->setText(QString::fromStdString(item_->valueHandle().asString()));
}

void URIEditor::updateURIValueHandle() {
	std::string newPathString{""};
	if (!item_->valueHandle().asString().empty()) {
		newPathString = pathIsAbsolute() ? createAbsolutePath() : createRelativePath();
	}

	item_->set(newPathString);
}

bool URIEditor::pathIsAbsolute() {
	return raco::utils::u8path(item_->valueHandle().asString()).is_absolute();
}

void URIEditor::switchAbsoluteRelativePath() {
	if (item_->valueHandle().asString().empty()) {
		return;
	}
	item_->set(pathIsAbsolute() ? createRelativePath() : createAbsolutePath());
}

bool URIEditor::fileExists() {
	if (item_->valueHandle().asString().empty()) {
		return false;
	}

	auto itemAbsolutePath = pathIsAbsolute() ? item_->valueHandle().asString() : createAbsolutePath();
	return raco::utils::u8path(itemAbsolutePath).exists();
}

std::string URIEditor::createAbsolutePath() {
	return core::PathQueries::resolveUriPropertyToAbsolutePath(*item_->project(), item_->valueHandle());
}

std::string URIEditor::createRelativePath() {
	auto itemPath = item_->valueHandle().asString();
	auto projectAbsPath = item_->project()->currentFolder();
	return raco::utils::u8path(itemPath).normalizedRelativePath(projectAbsPath).string();
}

}  // namespace raco::property_browser
