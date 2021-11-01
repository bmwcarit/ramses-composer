/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mainwindow.h"

#include "DebugActions.h"
#include "EditMenu.h"
#include "OpenRecentMenu.h"
#include "RaCoDockManager.h"
#include "SavedLayoutsDialog.h"
#include "common_widgets/ErrorView.h"
#include "common_widgets/ExportDialog.h"
#include "common_widgets/MeshAssetImportDialog.h"
#include "common_widgets/PreferencesView.h"
#include "common_widgets/UndoView.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/Handles.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "data_storage/BasicTypes.h"
#include "data_storage/Value.h"
#include "log_system/log.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeView.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "object_tree_view_model/ObjectTreeViewTopLevelSortProxyModel.h"
#include "ramses_widgets/PreviewMainWindow.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertyBrowserWidget.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/LogicEngineFormatter.h"
#include "ramses_base/BaseEngineBackend.h"
#include "components/Naming.h"
#include "application/RaCoApplication.h"
#include "components/RaCoPreferences.h"
#include "components/RamsesProjectMigration.h"
#include "serialization/Serialization.h"
#include "ui_mainwindow.h"

#include "user_types/CubeMap.h"
#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"
#include "user_types/Texture.h"

#include "utils/FileUtils.h"
#include "versiondialog.h"
#include "utils/PathUtils.h"


#include <DockWidget.h>
#include <IconProvider.h>
#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QObject>
#include <QScreen>
#include <QShortcut>
#include <QShortcutEvent>
#include <QTimer>
#include <chrono>
#include <string>
#include <vector>

static const int timerInterval60Fps = 17;

using namespace raco::core;

namespace {

using SDataChangeDispatcher = raco::components::SDataChangeDispatcher;
using SceneBackend = raco::ramses_adaptor::SceneBackend;

RaCoDockManager* createDockManager(MainWindow* parent) {
	auto* dockManager{new RaCoDockManager(parent)};
	dockManager->setConfigFlag(RaCoDockManager::eConfigFlag::TabCloseButtonIsToolButton, true);
	dockManager->setStyleSheet("");
	dockManager->iconProvider().registerCustomIcon(ads::TabCloseIcon, parent->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaCloseIcon, parent->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaMenuIcon, parent->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarMenuButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaUndockIcon, parent->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarNormalButton));

	if (QFile{PathManager::layoutFilePath().c_str()}.exists()) {
		QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
		dockManager->loadAllLayouts(settings);
	}

	QObject::connect(dockManager, &RaCoDockManager::perspectiveListChanged, [parent, dockManager]() {
		parent->updateSavedLayoutMenu();
	});

	return dockManager;
}

ads::CDockWidget* createDockWidget(const QString& title, QWidget* parent) {
	auto* dock = new ads::CDockWidget(title, parent);
	dock->setAttribute(Qt::WA_DeleteOnClose);
	dock->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
	return dock;
}

ads::CDockAreaWidget* createAndAddPreview(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::ramses_widgets::RendererBackend& rendererBackend, raco::application::RaCoApplication* application) {
	const auto& viewport = application->activeRaCoProject().project()->settings()->viewport_;
	const auto& backgroundColor = application->activeRaCoProject().project()->settings()->backgroundColor_.asVec3f();
	auto* previewWidget = new raco::ramses_widgets::PreviewMainWindow{rendererBackend, application->sceneBackendImpl(), {*viewport->i1_, *viewport->i2_}, application->activeRaCoProject().project(), application->dataChangeDispatcher()};
	QObject::connect(mainWindow, &MainWindow::viewportChanged, previewWidget, &raco::ramses_widgets::PreviewMainWindow::setViewport);
	previewWidget->displayScene(application->sceneBackendImpl()->currentSceneId(), backgroundColor);
	previewWidget->setWindowFlags(Qt::Widget);

	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::RAMSES_PREVIEW, mainWindow);
	dock->setObjectName(dockObjName);
	dock->setWidget(previewWidget);
	QObject::connect(dock, &ads::CDockWidget::closed, [mainWindow]() {
		mainWindow->setNewPreviewMenuEntryEnabled(true);
	});
	mainWindow->setNewPreviewMenuEntryEnabled(false);
	return dockManager->addDockWidget(ads::CenterDockWidgetArea, dock);
}

void connectPropertyBrowserAndTreeDockManager(raco::property_browser::PropertyBrowserWidget* propertyBrowser, raco::object_tree::view::ObjectTreeDockManager& treeDockManager) {
	QObject::connect(&treeDockManager, &raco::object_tree::view::ObjectTreeDockManager::newObjectTreeItemsSelected, propertyBrowser, &raco::property_browser::PropertyBrowserWidget::setValueHandles);
	QObject::connect(&treeDockManager, &raco::object_tree::view::ObjectTreeDockManager::selectionCleared, propertyBrowser, &raco::property_browser::PropertyBrowserWidget::clear);
	QObject::connect(propertyBrowser->model(), &raco::property_browser::PropertyBrowserModel::objectSelectionRequested, &treeDockManager, &raco::object_tree::view::ObjectTreeDockManager::selectObjectAcrossAllTreeDocks);
}

ads::CDockAreaWidget* createAndAddPropertyBrowser(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* application) {
	auto propertyBrowser = new raco::property_browser::PropertyBrowserWidget(application->dataChangeDispatcher(), application->activeRaCoProject().commandInterface(), mainWindow);
	connectPropertyBrowserAndTreeDockManager(propertyBrowser, treeDockManager);
	auto* dockWidget = createDockWidget(MainWindow::DockWidgetTypes::PROPERTY_BROWSER, mainWindow);
	dockWidget->setWidget(propertyBrowser, ads::CDockWidget::ForceNoScrollArea);
	dockWidget->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::RightDockWidgetArea, dockWidget);
}

void createAndAddProjectSettings(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::application::RaCoProject* project, SDataChangeDispatcher dataChangeDispatcher, CommandInterface* commandInterface) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PROJECT_SETTINGS, mainWindow);
	dock->setObjectName(dockObjName);
	auto propertyBrowser = new raco::property_browser::PropertyBrowserWidget(dataChangeDispatcher, commandInterface, mainWindow);
	propertyBrowser->setValueHandle({project->project()->settings()});
	propertyBrowser->setLockable(false);
	dock->setWidget(propertyBrowser);
	dockManager->addDockWidget(ads::RightDockWidgetArea, dock);
}

ads::CDockAreaWidget* createAndAddObjectTree(const char* title, const char* dockObjName, raco::object_tree::model::ObjectTreeViewDefaultModel *dockModel, QSortFilterProxyModel* sortFilterModel, const raco::object_tree::model::ObjectFilterFunc& filter, ads::DockWidgetArea area, MainWindow* mainWindow, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, ads::CDockAreaWidget* dockArea) {
	auto* dockObjectView = new raco::object_tree::view::ObjectTreeDock(title, mainWindow);
	QObject::connect(dockModel, &raco::object_tree::model::ObjectTreeViewDefaultModel::meshImportFailed, mainWindow, &MainWindow::showMeshImportErrorMessage);
	QObject::connect(dockModel, &raco::object_tree::model::ObjectTreeViewDefaultModel::meshImportSucceeded, mainWindow, &MainWindow::showMeshImportWarningMessage);
	dockModel->setProjectObjectFilterFunction(filter);
	dockModel->buildObjectTree();
	auto newTreeView = new raco::object_tree::view::ObjectTreeView(title, dockModel, sortFilterModel);
	if (sortFilterModel) {
		newTreeView->setSortingEnabled(true);
		newTreeView->sortByColumn(raco::object_tree::model::ObjectTreeViewDefaultModel::COLUMNINDEX_TYPE, Qt::SortOrder::AscendingOrder);
	}
	dockObjectView->addTreeView(newTreeView);
	treeDockManager.addTreeDock(dockObjectView);
	dockModel->setParent(dockObjectView);

	dockObjectView->setObjectName(dockObjName);
	return dockManager->addDockWidget(area, dockObjectView, dockArea);
}

ads::CDockAreaWidget* createAndAddProjectBrowser(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication, ads::CDockAreaWidget* dockArea) {
	auto* model = new raco::object_tree::model::ObjectTreeViewExternalProjectModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->activeRaCoProject().fileChangeMonitor(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects());
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::PROJECT_BROWSER, dockObjName, model, new QSortFilterProxyModel, Queries::filterForVisibleObjects, ads::BottomDockWidgetArea, mainWindow, dockManager, treeDockManager, dockArea);
}

ads::CDockAreaWidget* createAndAddResourceTree(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication, ads::CDockAreaWidget* dockArea) {
	using namespace raco::user_types;

	static const std::vector<std::string> allowedCreateableUserTypes{
		CubeMap::typeDescription.typeName,
		Material::typeDescription.typeName,
		Mesh::typeDescription.typeName,
		Texture::typeDescription.typeName,
		RenderBuffer::typeDescription.typeName,
		RenderTarget::typeDescription.typeName,
		RenderLayer::typeDescription.typeName,
		RenderPass::typeDescription.typeName
	};

	auto* model = new raco::object_tree::model::ObjectTreeViewResourceModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);
	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::RESOURCES, dockObjName, model, new QSortFilterProxyModel,
		[](const std::vector<SEditorObject>& objects) -> std::vector<SEditorObject> {
			return Queries::filterByTypeName(objects, allowedCreateableUserTypes);
		},
		ads::BottomDockWidgetArea, mainWindow, dockManager, treeDockManager, dockArea);
}

ads::CDockAreaWidget* createAndAddPrefabTree(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication, ads::CDockAreaWidget* dockArea) {
	using namespace raco::user_types;

	static const std::vector<std::string> allowedCreateableUserTypes{
		Node::typeDescription.typeName,
		MeshNode::typeDescription.typeName,
		Prefab::typeDescription.typeName,
		PrefabInstance::typeDescription.typeName,
		OrthographicCamera::typeDescription.typeName,
		PerspectiveCamera::typeDescription.typeName,
		LuaScript::typeDescription.typeName};

	auto* model = new raco::object_tree::model::ObjectTreeViewPrefabModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);

	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::PREFABS, dockObjName, model, new raco::object_tree::model::ObjectTreeViewTopLevelSortFilterProxyModel,
		[](const std::vector<SEditorObject>& objects) -> std::vector<SEditorObject> {
			std::vector<SEditorObject> result{};
			std::copy_if(objects.begin(), objects.end(), std::back_inserter(result),
				[](const SEditorObject& object) {
					for (auto parent = object; parent; parent = parent->getParent()) {
						if (parent->getTypeDescription().typeName == raco::user_types::Prefab::typeDescription.typeName) {
							return true;
						}
					}
					return false;
				});
			return result;
		},
		ads::BottomDockWidgetArea, mainWindow, dockManager, treeDockManager, dockArea);
}

ads::CDockAreaWidget* createAndAddSceneGraphTree(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication) {
	using namespace raco::user_types;

	static const std::vector<std::string> allowedCreateableUserTypes{
		Node::typeDescription.typeName,
		MeshNode::typeDescription.typeName,
		PrefabInstance::typeDescription.typeName,
		OrthographicCamera::typeDescription.typeName,
		PerspectiveCamera::typeDescription.typeName,
		LuaScript::typeDescription.typeName};

	auto* model = new raco::object_tree::model::ObjectTreeViewDefaultModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::SCENE_GRAPH, dockObjName, model, nullptr,
		[](const std::vector<SEditorObject>& objects) -> std::vector<SEditorObject> {
			return Queries::filterByTypeName(objects, allowedCreateableUserTypes);
		},
		ads::LeftDockWidgetArea, mainWindow, dockManager, treeDockManager, nullptr);
}

ads::CDockAreaWidget* createAndAddUndoView(raco::application::RaCoApplication* application, const char *dockObjName, raco::application::RaCoProject* project, MainWindow* mainWindow, RaCoDockManager* dockManager, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::UNDO_STACK, mainWindow);
	dock->setWidget(new raco::common_widgets::UndoView(project->undoStack(), application->dataChangeDispatcher(), mainWindow));
	dock->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* createAndAddErrorView(MainWindow* mainWindow, raco::application::RaCoApplication* application, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* errorView = new raco::common_widgets::ErrorView(application->activeRaCoProject().commandInterface(), application->dataChangeDispatcher());
	QObject::connect(errorView, &raco::common_widgets::ErrorView::objectSelectionRequested, &treeDockManager, &raco::object_tree::view::ObjectTreeDockManager::selectObjectAcrossAllTreeDocks);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::ERROR_VIEW, mainWindow);
	dock->setWidget(errorView);
	dock->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

void createInitialWidgets(MainWindow* mainWindow, raco::ramses_widgets::RendererBackend& rendererBackend, raco::application::RaCoApplication* application,  RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager) {
	createAndAddPreview(mainWindow, "defaultPreview", dockManager, rendererBackend, application);

	auto leftDockArea = createAndAddSceneGraphTree(mainWindow, "defaultSceneGraph", dockManager, treeDockManager, application);
	leftDockArea = createAndAddResourceTree(mainWindow, "defaultResourceTree", dockManager, treeDockManager, application, leftDockArea);
	createAndAddPrefabTree(mainWindow, "defaultPrefabTree", dockManager, treeDockManager, application, leftDockArea);

	createAndAddUndoView(application, "defaultUndoView", &application->activeRaCoProject(), mainWindow, dockManager, leftDockArea);

	createAndAddPropertyBrowser(mainWindow, "defaultPropertyBrowser", dockManager, treeDockManager, application);
}

}  // namespace

MainWindow::MainWindow(raco::application::RaCoApplication* racoApplication, raco::ramses_widgets::RendererBackend* rendererBackend, QWidget* parent)
	: QMainWindow(parent),
	  rendererBackend_{rendererBackend},
	  racoApplication_{racoApplication},
	  applicationName_("Ramses Composer") {
	// Setup the UI from the QtCreator file mainwindow.ui
	ui = new Ui::MainWindow();
	ui->setupUi(this);
	recentFileMenu_ = new OpenRecentMenu(this);
	QObject::connect(recentFileMenu_, &OpenRecentMenu::openProject, this, &MainWindow::openProject);
	ui->menuFile->insertMenu(ui->actionSave, recentFileMenu_);
	dockManager_ = createDockManager(this);
	setWindowIcon(QIcon(":applicationLogo"));
	resize(QGuiApplication::screenAt(this->pos())->size() * 0.85);

	// Shortcuts
	{
		
		auto undoShortcut = new QShortcut(QKeySequence::Undo, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(undoShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalUndoCallback(racoApplication_);
		});
		auto redoShortcut = new QShortcut(QKeySequence::Redo, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(redoShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalRedoCallback(racoApplication_);
		});

		auto copyShortcut = new QShortcut(QKeySequence::Copy, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(copyShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalCopyCallback(racoApplication_, &treeDockManager_);
		});

		auto pasteShortcut = new QShortcut(QKeySequence::Paste, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(pasteShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalPasteCallback(racoApplication_, &treeDockManager_);
		});
		
		ui->actionSave->setShortcut(QKeySequence::Save);
		ui->actionSave->setShortcutContext(Qt::ApplicationShortcut);
		QObject::connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveActiveProject);

		ui->actionSaveAs->setShortcut(QKeySequence::SaveAs);
		ui->actionSaveAs->setShortcutContext(Qt::ApplicationShortcut);
		QObject::connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveAsActiveProject);
	}

	QObject::connect(ui->actionOpen, &QAction::triggered, [this]() {
		auto file = QFileDialog::getOpenFileName(this, "Open", QString::fromStdString(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project)), "Ramses Composer Assembly (*.rca)");
		if (file.size() > 0) {
			openProject(file);
		}
	});
	QObject::connect(ui->actionNew, &QAction::triggered, [this]() {
		openProject();
	});
	QObject::connect(ui->actionExport, &QAction::triggered, this, [this]() {
		auto dialog = new raco::common_widgets::ExportDialog(racoApplication_, this);
		dialog->exec();
	});
	QObject::connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);

	new EditMenu(racoApplication_, &treeDockManager_, ui->menuEdit);

	QObject::connect(ui->actionPreferences, &QAction::triggered, [this]() {
		auto dialog = new raco::common_widgets::PreferencesView(this);
		dialog->resize(500, 500);
		dialog->exec();
		if (racoApplication_) {
			const auto& prefs = raco::components::RaCoPreferences::instance();
			raco::core::PathManager::setAllCachedPathRoots(racoApplication_->activeProjectFolder(),
				prefs.imageSubdirectory.toStdString(),
				prefs.meshSubdirectory.toStdString(),
				prefs.scriptSubdirectory.toStdString(),
				prefs.shaderSubdirectory.toStdString());
		}
	});

	// View actions
	QObject::connect(ui->actionNewPreview, &QAction::triggered, [this]() { createAndAddPreview(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, *rendererBackend_, racoApplication_); });
	QObject::connect(ui->actionNewPropertyBrowser, &QAction::triggered, [this]() { createAndAddPropertyBrowser(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_); });
	QObject::connect(ui->actionNewProjectBrowser, &QAction::triggered, [this]() { createAndAddProjectBrowser(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewSceneGraphTree, &QAction::triggered, [this]() { createAndAddSceneGraphTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_); });
	QObject::connect(ui->actionNewResourcesTree, &QAction::triggered, [this]() { createAndAddResourceTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewPrefabTree, &QAction::triggered, [this]() { createAndAddPrefabTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewUndoView, &QAction::triggered, [this]() { createAndAddUndoView(racoApplication_, EditorObject::normalizedObjectID("").c_str(), &racoApplication_->activeRaCoProject(), this, dockManager_); });
	QObject::connect(ui->actionNewErrorView, &QAction::triggered, [this]() { createAndAddErrorView(this, racoApplication_, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_); });
	QObject::connect(ui->actionRestoreDefaultLayout, &QAction::triggered, [this](){
		resetDockManager();
		createInitialWidgets(this, *rendererBackend_, racoApplication_, dockManager_, treeDockManager_);
	});

	QObject::connect(ui->actionSaveCurrentLayout, &QAction::triggered, [this]() {
		bool ok;
		auto layoutName = QInputDialog::getText(this, "Save Current Layout",
			"Layout Name:", QLineEdit::Normal,
			"", &ok);

		if (ok && !layoutName.isEmpty()) {
			if (dockManager_->perspectiveNames().contains(layoutName)) {
				auto overwriteConfirmed = QMessageBox::warning(this, "Overwriting Layout", fmt::format("Layout '{layout_name}' already exists.\n\nOverwrite?", fmt::arg("layout_name", layoutName.toStdString())).c_str(), QMessageBox::Yes | QMessageBox::No);
				if (overwriteConfirmed == QMessageBox::No) {
					return;
				}
			}

			dockManager_->addCustomLayout(layoutName);
			QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
			dockManager_->saveCustomLayouts(settings);
		}
	});

	QObject::connect(ui->actionManageLayouts, &QAction::triggered, [this]() {
		SavedLayoutsDialog(dockManager_, this).exec();
		QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
		dockManager_->saveCustomLayouts(settings);
	});

	QObject::connect(ui->actionProjectSettings, &QAction::triggered, [this]() { createAndAddProjectSettings(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, &racoApplication_->activeRaCoProject(), racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface()); });
	QObject::connect(ui->actionEnableRuntimeScriptPreview, &QAction::toggled, [this](const bool toggled) {
		racoApplication_->activeRaCoProject().project()->settings()->runTimer_ = toggled;
	});

	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());
	// Help actions
	QObject::connect(ui->actionAbout, &QAction::triggered, [this] {
		VersionDialog about(this);
		about.exec();
	});

	if (restoreSettings() == false) {
		createInitialWidgets(this, *rendererBackend_, racoApplication_, dockManager_, treeDockManager_);
	}

	// Setup
	updateApplicationTitle();
	updateSavedLayoutMenu();

	// Will we support Mac?
	setUnifiedTitleAndToolBarOnMac(true);

	renderTimerId_ = startTimer(timerInterval60Fps);
}

void MainWindow::timerEvent(QTimerEvent* event) {
	auto startLoop = std::chrono::high_resolution_clock::now();
	racoApplication_->doOneLoop();

	const auto& viewport = racoApplication_->activeRaCoProject().project()->settings()->viewport_;
	const auto& backgroundColor = racoApplication_->activeRaCoProject().project()->settings()->backgroundColor_.asVec3f();

	Q_EMIT viewportChanged({*viewport->i1_, *viewport->i2_});

	for (auto preview : findChildren<raco::ramses_widgets::PreviewMainWindow*>()) {
		preview->commit();
		preview->displayScene(racoApplication_->sceneBackendImpl()->currentSceneId(), backgroundColor);
	}
	auto logicEngineExecutionEnd = std::chrono::high_resolution_clock::now();
	timingsModel_.addLogicEngineTotalExecutionDuration(std::chrono::duration_cast<std::chrono::microseconds>(logicEngineExecutionEnd - startLoop).count());
	rendererBackend_->doOneLoop();

	racoApplication_->sceneBackendImpl()->flush();
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (resolveDirtiness()) {
		killTimer(renderTimerId_);
		QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
		settings.setValue("geometry", saveGeometry());
		settings.setValue("windowState", saveState());
		dockManager_->saveCurrentLayoutInCache(settings);
		QMainWindow::closeEvent(event);
		event->accept();
	} else {
		event->ignore();
	}
}

bool MainWindow::restoreSettings() {
	if (QFile{PathManager::layoutFilePath().c_str()}.exists()) {
		QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState").toByteArray());

		if (settings.childGroups().contains("cachedLayout")) {
			restoreCachedLayout();
		} else {
			createInitialWidgets(this, *rendererBackend_, racoApplication_, dockManager_, treeDockManager_);
		}

		return true;
	} else {
		return false;
	}
}

void MainWindow::openProject(const QString& file) {
	auto fileString = file.toStdString();
	if (!fileString.empty() && (!raco::utils::path::exists(fileString) || !raco::utils::path::userHasReadAccess(fileString))) {
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file {} is not available for loading.\n\nCheck whether the file at the specified path still exists and that you have read access to that file.", fileString).c_str(), QMessageBox::Close);
		return;
	}

	if (!resolveDirtiness()) {
		return;
	}

	if (file.size() > 0) {
		recentFileMenu_->addRecentFile(file);
	}

	{
		QSettings settings(PathManager::layoutFilePath().c_str(), QSettings::IniFormat);
		dockManager_->saveCurrentLayoutInCache(settings);
		// destroying QSettings actually creates and saves settings file
	}

	// Delete all ui widgets (and their listeners) before changing the project
	// Don't create a new DockManager right away - making QMessageBoxes pop up messes up state restoring
	// (see https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/315)
	delete dockManager_;

	try {
		racoApplication_->switchActiveRaCoProject(file);
	} catch (const raco::application::FutureFileVersion& error) {
		racoApplication_->switchActiveRaCoProject({});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file was created with newer version of {app_name}. Please upgrade.\n\nExpected File Version: {expected_file_version}\nFound File Version: {file_version}", fmt::arg("app_name", "Ramses Composer"), fmt::arg("expected_file_version", raco::components::RAMSES_PROJECT_FILE_VERSION), fmt::arg("file_version", error.fileVersion_)).c_str(), QMessageBox::Close);
	} catch (const ExtrefError& error) {
		racoApplication_->switchActiveRaCoProject({});
		QMessageBox::warning(this, "File Load Error", fmt::format("External reference update failed.\n\n{}", error.what()).c_str(), QMessageBox::Close);
	} catch (const std::exception& e) {
		racoApplication_->switchActiveRaCoProject({});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file {} could not be loaded.\n\nReported error: {}\n\nCheck whether the file has been broken or corrupted.", fileString, e.what()).c_str(), QMessageBox::Close);
	}

	// Recreate our layout with new context
	dockManager_ = createDockManager(this);
	restoreCachedLayout();
	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());

	updateApplicationTitle();

	if (racoApplication_->activeRaCoProject().project()->settings()->enableTimerFlag_.asBool() == true) {
		ui->menuDebug->addAction(ui->actionEnableRuntimeScriptPreview);
		ui->actionEnableRuntimeScriptPreview->setChecked(racoApplication_->activeRaCoProject().project()->settings()->runTimer_.asBool());
	} else {
		ui->menuDebug->removeAction(ui->actionEnableRuntimeScriptPreview);
		racoApplication_->activeRaCoProject().project()->settings()->runTimer_ = false;
	}
}

MainWindow::~MainWindow() {
	// sceneBackend needs to be reset first to unregister all adaptors (and their file listeners)
	// before the file change monitors and mesh caches get destroyed
	racoApplication_->resetScene();
	killTimer(renderTimerId_);
	delete ui;
}

void MainWindow::updateApplicationTitle() {
	raco::application::RaCoProject& project = racoApplication_->activeRaCoProject();
	if (racoApplication_->activeProjectPath().empty()) {
		setWindowTitle(applicationName_ + " - <New project>");
	} else {
		setWindowTitle(applicationName_ + " - " + project.name() + " (" + QString::fromStdString(racoApplication_->activeProjectPath()) + ")");
	}
}

void MainWindow::saveActiveProject() {
	if (racoApplication_->canSaveActiveProject()) {
		if (racoApplication_->activeProjectPath().empty()) {
			saveAsActiveProject();
		} else {
			racoApplication_->activeRaCoProject().save();
			recentFileMenu_->addRecentFile(racoApplication_->activeProjectPath().c_str());
		}
	} else {
		QMessageBox::warning(this, "Save Error", fmt::format("Can't save project: externally referenced projects not clean.").c_str(), QMessageBox::Close);
	}
}

void MainWindow::saveAsActiveProject() {
	if (racoApplication_->canSaveActiveProject()) {
		bool setProjectName = racoApplication_->activeProjectPath().empty();
		auto newPath = QFileDialog::getSaveFileName(this, "Save As...", QString::fromStdString(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project)), "Ramses Composer Assembly (*.rca)");
		if (newPath.isEmpty()) {
			return;
		}
		racoApplication_->activeRaCoProject().saveAs(newPath, setProjectName);
		recentFileMenu_->addRecentFile(racoApplication_->activeProjectPath().c_str());

		updateApplicationTitle();
	} else {
		QMessageBox::warning(this, "Save Error", fmt::format("Can't save project: externally referenced projects not clean.").c_str(), QMessageBox::Close);
	}
}

void MainWindow::importScene() {
	auto sceneFolder = raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, racoApplication_->activeProjectFolder());
	auto filePath = QFileDialog::getOpenFileName(this, "Load Asset File", QString::fromStdString(sceneFolder), "glTF files (*.gltf *.glb)");
	if (!filePath.isEmpty()) {
		MeshDescriptor meshDesc;
		meshDesc.absPath = filePath.toStdString();
		meshDesc.bakeAllSubmeshes = false;
		auto importSuccess = true;

		if (racoApplication_->activeRaCoProject().meshCache()->loadMesh(meshDesc)) {
			auto sceneGraph = racoApplication_->activeRaCoProject().meshCache()->getMeshScenegraph(meshDesc.absPath);

			auto importStatus = raco::common_widgets::MeshAssetImportDialog(sceneGraph, this).exec();
			if (importStatus == QDialog::Accepted) {
				importSuccess = racoApplication_->activeRaCoProject().commandInterface()->insertAssetScenegraph(sceneGraph, meshDesc.absPath, nullptr);
			}
		} else {
			importSuccess = false;
		}

		if (!importSuccess) {
			showMeshImportErrorMessage(meshDesc.absPath);
		}
		showMeshImportWarningMessage(meshDesc.absPath);
	}
}

void MainWindow::updateSavedLayoutMenu() {
	ui->menuSavedLayoutList->clear();
	for (const auto& layoutName : dockManager_->perspectiveNames()) {
		auto action = ui->menuSavedLayoutList->addAction(layoutName);
		QObject::connect(action, &QAction::triggered, this, [this, layoutName]() {
			restoreCustomLayout(layoutName);
		});
	}
	ui->menuSavedLayoutList->setDisabled(dockManager_->perspectiveNames().isEmpty());
}

bool MainWindow::resolveDirtiness() {
	bool continueWithAction{true};
	if (racoApplication_->activeRaCoProject().dirty()) {
		QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Ramses Composer",
			tr("Save unsaved changes?\n"),
			QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
			QMessageBox::Yes);
		continueWithAction = resBtn != QMessageBox::Cancel;
		if (resBtn == QMessageBox::Yes) {
			saveActiveProject();
		}
	}
	return continueWithAction;
}

QString MainWindow::getActiveProjectFolder() {
	return QString::fromStdString(racoApplication_->activeProjectFolder());
}

void MainWindow::restoreCachedLayout() {
	auto cachedLayoutInfo = dockManager_->getCachedLayoutInfo();
	regenerateLayoutDocks(cachedLayoutInfo);

	dockManager_->restoreCachedLayoutState();
}

void MainWindow::restoreCustomLayout(const QString& layoutName) {
	// reset needed to delete all dock widgets of the previous layout
	resetDockManager();

	auto extraLayoutInfo = dockManager_->getCustomLayoutInfo(layoutName);
	regenerateLayoutDocks(extraLayoutInfo);

	dockManager_->openPerspective(layoutName);
}

void MainWindow::regenerateLayoutDocks(const RaCoDockManager::LayoutDocks& docks) {
	setNewPreviewMenuEntryEnabled(true);
	auto hasPreview = false;
	for (const auto& [savedDockType, savedDockName] : docks) {
		auto dockNameString = savedDockName.toStdString();
		auto dockNameCString = dockNameString.c_str();
		if (savedDockType == DockWidgetTypes::PREFABS) {
			createAndAddPrefabTree(this, dockNameCString, dockManager_, treeDockManager_, racoApplication_, nullptr);
		} else if (savedDockType == DockWidgetTypes::PROJECT_BROWSER) {
			createAndAddProjectBrowser(this, dockNameCString, dockManager_, treeDockManager_, racoApplication_, nullptr);
		} else if (savedDockType == DockWidgetTypes::PROJECT_SETTINGS) {
			createAndAddProjectSettings(this, dockNameCString, dockManager_, &racoApplication_->activeRaCoProject(), racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface());
		} else if (savedDockType == DockWidgetTypes::PROPERTY_BROWSER) {
			createAndAddPropertyBrowser(this, dockNameCString, dockManager_, treeDockManager_, racoApplication_);
		} else if (savedDockType == DockWidgetTypes::RAMSES_PREVIEW) {
			if (!hasPreview) {
				createAndAddPreview(this, dockNameCString, dockManager_, *rendererBackend_, racoApplication_);
				// prevent loading of multiple preview windows
				hasPreview = true;
			}
		} else if (savedDockType == DockWidgetTypes::RESOURCES) {
			createAndAddResourceTree(this, dockNameCString, dockManager_, treeDockManager_, racoApplication_, nullptr);
		} else if (savedDockType == DockWidgetTypes::SCENE_GRAPH) {
			createAndAddSceneGraphTree(this, dockNameCString, dockManager_, treeDockManager_, racoApplication_);
		} else if (savedDockType == DockWidgetTypes::UNDO_STACK) {
			createAndAddUndoView(racoApplication_, dockNameCString, &racoApplication_->activeRaCoProject(), this, dockManager_);
		} else if (savedDockType == DockWidgetTypes::ERROR_VIEW) {
			createAndAddErrorView(this, racoApplication_, dockNameCString, dockManager_, treeDockManager_);
		} else {
			assert(false && "Unknown Dock Type detected");
		}
	}
}

void MainWindow::resetDockManager() {
	delete dockManager_;
	dockManager_ = createDockManager(this);
}

void MainWindow::showMeshImportErrorMessage(const std::string& filePath) {
	auto importErrorMsg = QString::fromStdString(racoApplication_->activeRaCoProject().meshCache()->getMeshError(filePath));
	auto filePathQString = QString::fromStdString(filePath);
	auto dialogText = importErrorMsg.isEmpty() ? QString{"Ramses Composer encountered an unknown error while importing assets from %1.\nConsult with the logs or file contents to find the error."}.arg(filePathQString)
											   : QString{"Ramses Composer encountered the following error while importing assets from %1:\n\n%2"}.arg(filePathQString).arg(importErrorMsg);

	QMessageBox importErrorBox(QMessageBox::Critical, "Mesh Import Error", dialogText, QMessageBox::Ok, this);
	importErrorBox.setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	importErrorBox.exec();
}

void MainWindow::showMeshImportWarningMessage(const std::string& filePath) {
	auto importWarningMsg = QString::fromStdString(racoApplication_->activeRaCoProject().meshCache()->getMeshWarning(filePath));
	auto filePathQString = QString::fromStdString(filePath);
	if (importWarningMsg.isEmpty()) {
		return;
	}
	auto dialogText = QString{"Ramses Composer encountered the following warning while importing assets from %1:\n\n%2"}.arg(filePathQString).arg(importWarningMsg);

	QMessageBox importErrorBox(QMessageBox::Warning, "Mesh Import Warning", dialogText, QMessageBox::Ok, this);
	importErrorBox.setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	importErrorBox.exec();
}

// As long as we can't cleverly create multiple previews on the same scene, the "New Preview" menu item should only be enabled to create one Ramses preview.
void MainWindow::setNewPreviewMenuEntryEnabled(bool enabled) {
	ui->actionNewPreview->setEnabled(enabled);
}
