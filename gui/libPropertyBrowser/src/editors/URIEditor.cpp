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
#include <QDesktopServices>
#include <QFileDialog>
#include <QFocusEvent>
#include <QMenu>
#include <QObject>
#include <QPushButton>

#include "style/Icons.h"

#include "core/PathQueries.h"
#include "core/Project.h"
#include "property_browser/PropertyBrowserItem.h"
#include "utils/u8path.h"

namespace raco::property_browser {

using PathManager = core::PathManager;

URIEditor::URIEditor(PropertyBrowserItem* item, QWidget* parent) : StringEditor(item, parent) {
	auto* loadFileButton = new common_widgets::PropertyBrowserButton("  ...  ", this);

	QObject::connect(loadFileButton, &QPushButton::clicked, [this]() {
		auto uriAnno = item_->query<core::URIAnnotation>();
		std::string cachedPath;
		auto projectAbsPath = item_->project()->currentFolder();

		if (fileUniqueAndExists()) {
			auto fileInfo = QFileInfo(QString::fromStdString(uniqueAbsolutePath().value()));
			QDir uriDir = fileInfo.absoluteDir();
			cachedPath = uriDir.absolutePath().toStdString();
		} else {
			auto cachedPathKey = uriAnno->getFolderTypeKey();
			cachedPath = core::PathManager::getCachedPath(cachedPathKey).string();
		}

		auto fileFilter = QString();
		auto fileMode = QFileDialog::FileMode::Directory;
		if (!uriAnno->isProjectSubdirectoryURI()) {
			fileFilter = uriAnno->filter_.asString().c_str();
			fileMode = QFileDialog::FileMode::AnyFile;
		}

		QFileDialog* dialog = new QFileDialog(this, tr("Choose URI"), QString::fromStdString(utils::u8path(cachedPath).existsDirectory() ? cachedPath : projectAbsPath), fileFilter);
		if (uriAnno->isProjectSubdirectoryURI()) {
			dialog->setOption(QFileDialog::ShowDirsOnly, true);
		}
		dialog->setFileMode(fileMode);
		connect(dialog, &QFileDialog::fileSelected, this, &URIEditor::setFromFileDialog);
		dialog->exec();
	});
	layout()->addWidget(loadFileButton);

	editButton_ = new common_widgets::PropertyBrowserButton(style::Icons::instance().openInNew, "", this);
	editButton_->setMaximumWidth(common_widgets::PropertyBrowserButton::MAXIMUM_WIDTH_PX + 5);
	editButton_->setEnabled(fileUniqueAndExists());
	connect(editButton_, &QPushButton::clicked, [this]() {
		// We know the absolute path has a unique value since the button is disabled otherwise.
		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(uniqueAbsolutePath().value())));
	});
	layout()->addWidget(editButton_);

	const QString dragAndDropFilter = QString::fromStdString(item_->query<core::URIAnnotation>()->filter_.asString());
	lineEdit_->setDragAndDropFilter(dragAndDropFilter);
	lineEdit_->setAcceptDrops(true);
	connect(lineEdit_, &StringEditorLineEdit::fileDropped, this, [this](const QString& file) {
		setFromFileDialog(file);
	});
	lineEdit_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(lineEdit_, &QLineEdit::customContextMenuRequested, [this, item](const QPoint& p) { showCustomLineEditContextMenu(p, item); });

	connect(item_, &PropertyBrowserItem::valueChanged, this, [this, item]() {
		updateLineEdit();
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

	QAction* switchToAbsolutePathAction = new QAction("Make path absolute", this);
	switchToAbsolutePathAction->setEnabled(switchToAbsolutePathEnabled());
	connect(switchToAbsolutePathAction, &QAction::triggered, this, &URIEditor::switchToAbsolutePath);

	QAction* switchToRelativePathAction = new QAction("Make path relative", this);
	switchToRelativePathAction->setEnabled(switchToRelativePathEnabled());
	connect(switchToRelativePathAction, &QAction::triggered, this, &URIEditor::switchToRelativePath);

	contextMenu->insertSeparator(contextMenu->actions().front());
	contextMenu->insertAction(contextMenu->actions().front(), switchToRelativePathAction);
	contextMenu->insertAction(contextMenu->actions().front(), switchToAbsolutePathAction);

	contextMenu->exec(mapToGlobal(p));
}

void URIEditor::updateFileEditButton() {
	editButton_->setEnabled(fileUniqueAndExists());
}

void URIEditor::switchToAbsolutePath() {
	std::string desc = fmt::format("Make path absolute for property '{}'", item_->getPropertyPath());
	item_->commandInterface()->executeCompositeCommand(
		[this]() {
			for (const auto& handle : item_->valueHandles()) {
				if (canSwitchToAbsolutePath(handle)) {
					std::string absPath = core::PathQueries::resolveUriPropertyToAbsolutePath(*item_->project(), handle);
					item_->commandInterface()->set(handle, absPath);
				}
			}
		},
		desc);
}

void URIEditor::switchToRelativePath() {
	std::string desc = fmt::format("Make path relative for property '{}'", item_->getPropertyPath());
	item_->commandInterface()->executeCompositeCommand(
		[this]() {
			auto projectAbsPath = item_->project()->currentFolder();
			for (const auto& handle : item_->valueHandles()) {
				if (canSwitchToRelativePath(handle)) {
					std::string relPath = utils::u8path(handle.asString()).normalizedRelativePath(projectAbsPath).string();
					item_->commandInterface()->set(handle, relPath);
				}
			}
		},
		desc);
}

bool URIEditor::canSwitchToAbsolutePath(const core::ValueHandle& handle) {
	return !handle.asString().empty() && utils::u8path(handle.asString()).is_relative();
}

bool URIEditor::canSwitchToRelativePath(const core::ValueHandle& handle) {
	auto value = handle.asString();
	return !value.empty() && utils::u8path(value).is_absolute() && utils::u8path::areSharingSameRoot(value, item_->project()->currentPath());
}

bool URIEditor::switchToAbsolutePathEnabled() {
	return std::any_of(item_->valueHandles().begin(), item_->valueHandles().end(), [this](auto handle) {
		return canSwitchToAbsolutePath(handle);
	});
}

bool URIEditor::switchToRelativePathEnabled() {
	return std::any_of(item_->valueHandles().begin(), item_->valueHandles().end(), [this](auto handle) {
		return canSwitchToRelativePath(handle);
	});
}

bool URIEditor::fileUniqueAndExists() {
	auto absPath = uniqueAbsolutePath();
	if (absPath.has_value()) {
		return utils::u8path(absPath.value()).exists();
	}
	return false;
}

std::optional<std::string> URIEditor::uniqueAbsolutePath() {
	auto absPath = core::PathQueries::resolveUriPropertyToAbsolutePath(*item_->project(), *item_->valueHandles().begin());
	if (std::all_of(item_->valueHandles().begin(), item_->valueHandles().end(), [this, absPath](auto handle) {
			return !handle.asString().empty() &&
				   core::PathQueries::resolveUriPropertyToAbsolutePath(*item_->project(), handle) == absPath;
		})) {
		return absPath;
	}
	return std::nullopt;
}

void URIEditor::setFromFileDialog(const QString& absPath) {
	std::string desc = fmt::format("Set property '{}' to {}", item_->getPropertyPath(), absPath.toStdString());
	item_->commandInterface()->executeCompositeCommand(
		[this, &absPath]() {
			auto projectAbsPath = item_->project()->currentFolder();
			for (const auto& handle : item_->valueHandles()) {
				if (utils::u8path(handle.asString()).is_absolute()) {
					item_->commandInterface()->set(handle, absPath.toStdString());
				} else {
					item_->commandInterface()->set(handle, utils::u8path(absPath.toStdString()).normalizedRelativePath(projectAbsPath).string());
				}
			}
		},
		desc);

	QDir dir = QFileInfo(absPath).absoluteDir();
	auto uriAnno = item_->query<core::URIAnnotation>();
	auto cachedPathKey = uriAnno->getFolderTypeKey();
	core::PathManager::setCachedPath(cachedPathKey, dir.absolutePath().toStdString());
}

}  // namespace raco::property_browser
