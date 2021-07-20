/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

#include "utils/PathUtils.h"
#include "core/PathManager.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "property_browser/PropertyBrowserItem.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/Texture.h"

namespace raco::property_browser {

using PathManager = core::PathManager;

URIEditor::URIEditor(PropertyBrowserItem* item, QWidget* parent) : StringEditor(item, parent), currentItem_(item) {
	auto* loadFileButton = new raco::common_widgets::PropertyBrowserButton("  ...  ", this);

	auto uriAnno = item->query<core::URIAnnotation>();
	auto filter = *uriAnno->filter_;
	QObject::connect(loadFileButton, &QPushButton::clicked, [this, filter]() {
		std::string cachedPath;
		auto projectAbsPath = currentItem_->project()->currentFolder();
		auto cachedPathKey = getCachedPathKeyCorrespondingToUserType();

		if (fileExists()) {
			auto fileInfo = QFileInfo(QString::fromStdString(PathManager::constructAbsolutePath(projectAbsPath, lineEdit_->text().toStdString())));
			QDir uriDir = fileInfo.absoluteDir();
			cachedPath = uriDir.absolutePath().toStdString();
		} else {
			cachedPath = raco::core::PathManager::getCachedPath(cachedPathKey);
		}

		QFileDialog* dialog = new QFileDialog(this, tr("Choose URI"), QString::fromStdString(raco::utils::path::isExistingDirectory(cachedPath) ? cachedPath : projectAbsPath), tr(filter.c_str()));
		connect(dialog, &QFileDialog::fileSelected, [this, cachedPathKey](const QString& file) {
			auto oldUriWasAbsolute = pathIsAbsolute();
			if (oldUriWasAbsolute) {
				currentItem_->set(file.toStdString());
			} else {
				currentItem_->set(PathManager::constructRelativePath(file.toStdString(), currentItem_->project()->currentFolder()));
			}

			QDir dir = QFileInfo(file).absoluteDir();
			raco::core::PathManager::setCachedPath(cachedPathKey, dir.absolutePath().toStdString());
		});
		dialog->exec();
	});
	layout()->addWidget(loadFileButton);

	editButton_ = new raco::common_widgets::PropertyBrowserButton(raco::style::Icons::icon(raco::style::Pixmap::open_in_new, this), "", this);
	editButton_->setMaximumWidth(raco::common_widgets::PropertyBrowserButton::MAXIMUM_WIDTH_PX + 5);
	editButton_->setEnabled(fileExists());
	connect(editButton_, &QPushButton::clicked, [this]() {
		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(createAbsolutePath())));
	});
	layout()->addWidget(editButton_);

	lineEdit_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(lineEdit_, &QLineEdit::customContextMenuRequested, [this, item](const QPoint& p) { showCustomLineEditContextMenu(p, item); });

	connect(currentItem_, &PropertyBrowserItem::valueChanged, this, [this, item](core::ValueHandle& handle) {
		updateURILineEditString();
		updateFileEditButton();
	});

	updateFileEditButton();
}

void URIEditor::showCustomLineEditContextMenu(const QPoint& p, PropertyBrowserItem* item) {
	QMenu* contextMenu = lineEdit_->createStandardContextMenu();

	QAction* absoluteRelativeURISwitch = new QAction("Use Absolute Path", this);
	auto currentPath = std::filesystem::path(lineEdit_->text().toStdString());
	auto URIisOnDifferentPartition = pathIsAbsolute() && !PathManager::pathsShareSameRoot(lineEdit_->text().toStdString(), item->project()->currentPath());
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
	lineEdit_->setText(QString::fromStdString(currentItem_->valueHandle().asString()));
}

void URIEditor::updateURIValueHandle() {
	std::string newPathString{""};
	if (!currentItem_->valueHandle().asString().empty()) {
		newPathString = pathIsAbsolute() ? createAbsolutePath() : createRelativePath();
	}

	currentItem_->set(newPathString);
}

bool URIEditor::pathIsAbsolute() {
	return std::filesystem::path(currentItem_->valueHandle().asString()).is_absolute();
}

void URIEditor::switchAbsoluteRelativePath() {
	if (currentItem_->valueHandle().asString().empty()) {
		return;
	}
	currentItem_->set(pathIsAbsolute() ? createRelativePath() : createAbsolutePath());
}

bool URIEditor::fileExists() {
	if (currentItem_->valueHandle().asString().empty()) {
		return false;
	}

	auto itemAbsolutePath = pathIsAbsolute() ? currentItem_->valueHandle().asString() : createAbsolutePath();
	return raco::utils::path::exists(itemAbsolutePath);
}

std::string URIEditor::createAbsolutePath() {
	return core::PathQueries::resolveUriPropertyToAbsolutePath(*currentItem_->project(), currentItem_->valueHandle());
}

std::string URIEditor::createRelativePath() {
	auto itemPath = currentItem_->valueHandle().asString();
	auto projectAbsPath = currentItem_->project()->currentFolder();
	return PathManager::constructRelativePath(itemPath, projectAbsPath);
}

std::string URIEditor::getCachedPathKeyCorrespondingToUserType() {
	auto rootObj = currentItem_->valueHandle().rootObject();

	if (rootObj->as<raco::user_types::CubeMap>() || rootObj->as<raco::user_types::Texture>()) {
		return raco::core::PathManager::IMAGE_SUB_DIRECTORY;
	}

	if (rootObj->as<raco::user_types::Mesh>()) {
		return raco::core::PathManager::MESH_SUB_DIRECTORY;
	}

	if (rootObj->as<raco::user_types::LuaScript>()) {
		return raco::core::PathManager::SCRIPT_SUB_DIRECTORY;
	}

	if (rootObj->as<raco::user_types::Material>()) {
		return raco::core::PathManager::SHADER_SUB_DIRECTORY;
	}

	assert(false && "unknown user type found in URIEditor::getCachedPathKeyCorrespondingToUserType()");
	return "";
}

}  // namespace raco::property_browser
