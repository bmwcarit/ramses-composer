/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "application/RaCoApplication.h"
#include "common_widgets/ErrorView.h"
#include "common_widgets/ExportDialog.h"
#include "common_widgets/LogView.h"
#include "common_widgets/PythonOutputDialog.h"
#include "common_widgets/MeshAssetImportDialog.h"
#include "common_widgets/PerformanceTableView.h"
#include "common_widgets/PreferencesView.h"
#include "common_widgets/RunScriptDialog.h"
#include "common_widgets/TracePlayerWidget.h"
#include "common_widgets/UndoView.h"
#include "components/RaCoPreferences.h"
#include "core/BasicTypes.h"
#include "core/EditorObject.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/ProjectMigration.h"
#include "data_storage/Value.h"
#include "python_api/PythonAPI.h"
#include "gui_python_api/GUIPythonAPI.h"
#include "log_system/log.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeView.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewRenderViewModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "object_tree_view_model/ObjectTreeViewSceneGraphModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "property_browser/PropertyBrowserModel.h"

#include "ramses_adaptor/AbstractSceneAdaptor.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"
#include "ramses_widgets/AbstractViewMainWindow.h"
#include "ramses_widgets/PreviewMainWindow.h"
#include "ui_mainwindow.h"

#include "user_types/AnchorPoint.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/AnimationChannelRaco.h"
#include "user_types/BlitPass.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"
#include "user_types/RenderTarget.h"
#include "user_types/Skin.h"
#include "user_types/Texture.h"
#include "user_types/TextureExternal.h"
#include "user_types/Timer.h"

#include "utils/u8path.h"
#include "versiondialog.h"

#include "DockAreaWidget.h"
#include "ads_globals.h"

#include <DockWidget.h>
#include <IconProvider.h>
#include <QDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QObject>
#include <QScreen>
#include <QShortcut>
#include <QShortcutEvent>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

static const int timerInterval60Fps = 17;

using namespace raco;
using namespace raco::core;

using SDataChangeDispatcher = components::SDataChangeDispatcher;
using SceneBackend = ramses_adaptor::SceneBackend;

RaCoDockManager* MainWindow::createDockManager() {
	auto* dockManager{new RaCoDockManager(this)};
	dockManager->setConfigFlag(RaCoDockManager::eConfigFlag::TabCloseButtonIsToolButton, true);
	dockManager->setStyleSheet("");
	dockManager->iconProvider().registerCustomIcon(ads::TabCloseIcon, this->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaCloseIcon, this->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarCloseButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaMenuIcon, this->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarMenuButton));
	dockManager->iconProvider().registerCustomIcon(ads::DockAreaUndockIcon, this->style()->standardIcon(QStyle::StandardPixmap::SP_TitleBarNormalButton));

	if (PathManager::layoutFilePath().existsFile()) {
		auto settings = core::PathManager::layoutSettings();
		dockManager->loadAllLayouts(settings);
	}

	QObject::connect(dockManager, &RaCoDockManager::perspectiveListChanged, [this, dockManager]() {
		this->updateSavedLayoutMenu();
	});

	return dockManager;
}

ads::CDockWidget* MainWindow::createDockWidget(const QString& title, QWidget* parent) {
	auto* dock = new ads::CDockWidget(title, parent);
	dock->setAttribute(Qt::WA_DeleteOnClose);
	dock->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
	return dock;
}

ads::CDockAreaWidget* MainWindow::createAndAddPreview(const char* dockObjName) {
	const auto& viewport = racoApplication_->activeRaCoProject().project()->settings()->viewport_;
	const auto& backgroundColor = *racoApplication_->activeRaCoProject().project()->settings()->backgroundColor_;
	auto* previewWidget = new ramses_widgets::PreviewMainWindow{*rendererBackend_, racoApplication_->sceneBackendImpl(), {*viewport->i1_, *viewport->i2_}, racoApplication_->activeRaCoProject().project(), racoApplication_->dataChangeDispatcher()};
	QObject::connect(this, &MainWindow::viewportChanged, previewWidget, &ramses_widgets::PreviewMainWindow::setViewport);
	previewWidget->displayScene(racoApplication_->sceneBackendImpl()->currentSceneId(), backgroundColor);
	previewWidget->setWindowFlags(Qt::Widget);

	const auto shortcut = new QShortcut(QKeySequence(Qt::Key_F12), previewWidget, nullptr, nullptr, Qt::ApplicationShortcut);
	QObject::connect(shortcut, &QShortcut::activated, [previewWidget]() {
		previewWidget->saveScreenshot();
	});

	gui_python_api::setupPreviewWindow(previewWidget);

	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::RAMSES_PREVIEW, this);
	dock->setObjectName(dockObjName);
	dock->setWidget(previewWidget);
	QObject::connect(dock, &ads::CDockWidget::closed, [this]() {
		ui->actionNewPreview->setEnabled(true);
		gui_python_api::setupPreviewWindow(nullptr);
	});
	ui->actionNewPreview->setEnabled(false);
	return dockManager_->addDockWidget(ads::CenterDockWidgetArea, dock);
}

ads::CDockAreaWidget* MainWindow::createAndAddAbstractSceneView(const char* dockObjName) {
	auto* widget = new ramses_widgets::AbstractViewMainWindow{*rendererBackend_, racoApplication_->abstractScene(), &treeDockManager_, racoApplication_->activeRaCoProject().commandInterface()};
	widget->setWindowFlags(Qt::Widget);

	QObject::connect(widget, &ramses_widgets::AbstractViewMainWindow::selectionRequested, this, &MainWindow::focusToSelection);

	QObject::connect(&treeDockManager_, &object_tree::view::ObjectTreeDockManager::newObjectTreeItemsSelected, widget, &ramses_widgets::AbstractViewMainWindow::onSelectionChanged);

	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::ABSTRACT_SCENE_VIEW, this);
	dock->setObjectName(dockObjName);
	dock->setWidget(widget);
	QObject::connect(dock, &ads::CDockWidget::closed, [this]() {
		ui->actionNewAbstractSceneView->setEnabled(true);
	});
	ui->actionNewAbstractSceneView->setEnabled(false);
	return dockManager_->addDockWidget(ads::CenterDockWidgetArea, dock);
}

ads::CDockAreaWidget* MainWindow::createAndAddPropertyBrowser(const char* dockObjName) {
	auto propertyBrowser = new property_browser::PropertyBrowserWidget(racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->sceneBackendImpl(), &treeDockManager_, this);
	QObject::connect(this, &MainWindow::focusRequestedForPropertyBrowser, propertyBrowser, &property_browser::PropertyBrowserWidget::setObjectFromObjectId);
	QObject::connect(propertyBrowser->model(), &property_browser::PropertyBrowserModel::selectionRequested, this, &MainWindow::focusToSelection);

	QObject::connect(&treeDockManager_, &object_tree::view::ObjectTreeDockManager::newObjectTreeItemsSelected, propertyBrowser, &property_browser::PropertyBrowserWidget::setObjects);
	
	auto* dockWidget = createDockWidget(MainWindow::DockWidgetTypes::PROPERTY_BROWSER, this);
	dockWidget->setWidget(propertyBrowser);
	dockWidget->setObjectName(dockObjName);
	return dockManager_->addDockWidget(ads::RightDockWidgetArea, dockWidget);
}

void MainWindow::createAndAddProjectSettings(const char* dockObjName) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PROJECT_SETTINGS, this);
	dock->setObjectName(dockObjName);
	auto propertyBrowser = new property_browser::PropertyBrowserWidget(racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->sceneBackendImpl(), nullptr, this);
	propertyBrowser->setObjects({racoApplication_->activeRaCoProject().project()->settings()}, {});
	propertyBrowser->setLockable(false);
	dock->setWidget(propertyBrowser);
	dockManager_->addDockWidget(ads::RightDockWidgetArea, dock);
}

ads::CDockAreaWidget* MainWindow::createAndAddObjectTree(const char* title, const char* dockObjName, object_tree::model::ObjectTreeViewDefaultModel* dockModel, object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel* sortFilterModel, ads::DockWidgetArea area, ads::CDockAreaWidget* dockArea) {
	auto* dockObjectView = new object_tree::view::ObjectTreeDock(title, this);
	QObject::connect(dockModel, &object_tree::model::ObjectTreeViewDefaultModel::meshImportFailed, this, &MainWindow::showMeshImportErrorMessage);
	dockModel->buildObjectTree();
	auto newTreeView = new object_tree::view::ObjectTreeView(title, dockModel, sortFilterModel);

	dockObjectView->setTreeView(newTreeView);
	treeDockManager_.addTreeDock(dockObjectView);
	dockModel->setParent(dockObjectView);

	dockObjectView->setObjectName(dockObjName);
	return dockManager_->addDockWidget(area, dockObjectView, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddProjectBrowser(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* model = new object_tree::model::ObjectTreeViewExternalProjectModel(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), racoApplication_->externalProjects());
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::PROJECT_BROWSER, dockObjName, model, new object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel(this), ads::BottomDockWidgetArea, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddResourceTree(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* model = new object_tree::model::ObjectTreeViewResourceModel(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), racoApplication_->externalProjects());
	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::RESOURCES, dockObjName, model, new object_tree::model::ObjectTreeViewResourceSortFilterProxyModel(this),
		ads::BottomDockWidgetArea, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddPrefabTree(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* model = new object_tree::model::ObjectTreeViewPrefabModel(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), racoApplication_->externalProjects());
	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::PREFABS, dockObjName, model, new object_tree::model::ObjectTreeViewTopLevelSortFilterProxyModel(this),
		ads::BottomDockWidgetArea, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddSceneGraphTree(const char* dockObjName) {
	auto* model = new object_tree::model::ObjectTreeViewSceneGraphModel(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), racoApplication_->externalProjects());
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::SCENE_GRAPH, dockObjName, model, new object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel(this, false),
		ads::LeftDockWidgetArea, nullptr);
}

ads::CDockAreaWidget* MainWindow::createAndAddRenderView(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* model = new object_tree::model::ObjectTreeViewRenderViewModel(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), racoApplication_->externalProjects());

	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::RENDER_VIEW, dockObjName, model, new object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel(this, true),
		ads::BottomDockWidgetArea, dockArea);
}


ads::CDockAreaWidget* MainWindow::createAndAddUndoView(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::UNDO_STACK, this);
	dock->setWidget(new common_widgets::UndoView(racoApplication_->activeRaCoProject().undoStack(), racoApplication_->dataChangeDispatcher(), this));
	dock->setObjectName(dockObjName);
	return dockManager_->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddErrorView(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* errorView = new raco::common_widgets::ErrorView(racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->dataChangeDispatcher(), false, logViewModel_);
	QObject::connect(errorView, &raco::common_widgets::ErrorView::objectSelectionRequested, &treeDockManager_, [this](auto objectID) {
		if (auto object = racoApplication_->activeRaCoProject().project()->getInstanceByID(objectID.toStdString())) {
			treeDockManager_.selectObjectAndPropertyAcrossAllTreeDocks(object, {});
		}
	});
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::ERROR_VIEW, this);
	dock->setWidget(errorView);
	dock->setObjectName(dockObjName);
	return dockManager_->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddPerformanceTable(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto performanceView = new common_widgets::PerformanceTableView(racoApplication_, racoApplication_->dataChangeDispatcher(), this);
	QObject::connect(performanceView, &raco::common_widgets::PerformanceTableView::objectSelectionRequested, &treeDockManager_, [this](auto objectID) {
		if (auto object = racoApplication_->activeRaCoProject().project()->getInstanceByID(objectID.toStdString())) {
			treeDockManager_.selectObjectAndPropertyAcrossAllTreeDocks(object, {});
		}
	});
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PERFORMANCE_TABLE, this);
	dock->setWidget(performanceView);
	dock->setObjectName(dockObjName);
	return dockManager_->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddLogView(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	auto* logView = new common_widgets::LogView(logViewModel_);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::LOG_VIEW, this);
	dock->setWidget(logView);
	dock->setObjectName(dockObjName);
	return dockManager_->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddPythonRunner(const char* dockObjName, ads::CDockAreaWidget* dockArea) {
	

	auto* pythonRunner = new common_widgets::RunScriptDialog(pythonScriptCache_, pythonScriptArgumentCache_, this);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PYTHON_RUNNER, this);
	dock->setWidget(pythonRunner);
	dock->setObjectName(dockObjName);

	return dockManager_->addDockWidget(ads::RightDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* MainWindow::createAndAddTracePlayer() {
	if (auto existingTraceplayerDock{dockManager_->findDockWidget(MainWindow::DockWidgetTypes::TRACE_PLAYER)}) {
		return existingTraceplayerDock->dockAreaWidget();
	}

	auto* newTraceplayerDock{createDockWidget(MainWindow::DockWidgetTypes::TRACE_PLAYER, this)};
	newTraceplayerDock->setMinimumSizeHintMode(ads::CDockWidget::eMinimumSizeHintMode::MinimumSizeHintFromContent);

	auto* traceplayerWidget{new common_widgets::TracePlayerWidget(newTraceplayerDock->objectName(), &racoApplication_->activeRaCoProject().tracePlayer())};
	newTraceplayerDock->setWidget(traceplayerWidget, ads::CDockWidget::ForceNoScrollArea);

	ads::CDockWidget* existingPreviewDock{nullptr};
	constexpr auto isRamsesPreviewWidget{
		[](const ads::CDockWidget* dockWidget) { return dockWidget->windowTitle() == MainWindow::DockWidgetTypes::RAMSES_PREVIEW; }};
	const auto& dockWidgetsMap{dockManager_->dockWidgetsMap()};
	if (const auto itr = std::find_if(dockWidgetsMap.begin(), dockWidgetsMap.end(), isRamsesPreviewWidget); itr != dockWidgetsMap.end()) {
		existingPreviewDock = *itr;
	}

	// this prevents the whole dock area from auto resizing by ADS when TracePlayer is added to the top of Preview
	struct LockDockAreaWidth {
		QWidget* target_;
		LockDockAreaWidth(QWidget* target, ads::CDockWidget* host) : target_{target} {
			if (host) {
				target->setFixedWidth(host->width());
			}
		}
		~LockDockAreaWidth() {
			target_->setMinimumWidth(0);
			target_->setMaximumWidth(QWIDGETSIZE_MAX);
		}
	} lockDockAreaWidth{traceplayerWidget, existingPreviewDock};

	ads::CDockAreaWidget* previewDockArea{nullptr};
	if (existingPreviewDock) {
		previewDockArea = existingPreviewDock->dockAreaWidget();
	}

	return dockManager_->addDockWidget(ads::TopDockWidgetArea, newTraceplayerDock, previewDockArea);
}

void MainWindow::createInitialWidgets() {
	auto centerDockArea = createAndAddPreview("defaultPreview");
	createAndAddErrorView("defaultErrorView", centerDockArea);

	auto leftDockArea = createAndAddSceneGraphTree( "defaultSceneGraph");
	leftDockArea = createAndAddResourceTree("defaultResourceTree", leftDockArea);
	createAndAddPrefabTree("defaultPrefabTree", leftDockArea);

	createAndAddUndoView("defaultUndoView", leftDockArea);

	createAndAddPropertyBrowser("defaultPropertyBrowser");
}

void MainWindow::runPythonOnSaveScript(const std::string& scriptPath, const std::string& title) {
	utils::u8path utilPath = utils::u8path(scriptPath);
	if (utilPath.is_relative()) {
		utilPath = utilPath.normalizedAbsolutePath(racoApplication_->activeProjectFolder());
	}

	if (!scriptPath.empty()) {
		const auto runStatus = python_api::runPythonScriptFromFile(utilPath.string());

		if (!runStatus.stdOutBuffer.empty() || !runStatus.stdErrBuffer.empty()) {
			const auto msgBox = new common_widgets::PythonOutputDialog(
				QString::fromStdString(title),
				QString::fromStdString(runStatus.stdOutBuffer),
				QString::fromStdString(runStatus.stdErrBuffer));
			msgBox->exec();
		}
	}
}

void MainWindow::runPythonOnSaveScripts() {
	// Global
	const auto globalPythonOnSaveScript = components::RaCoPreferences::instance().globalPythonOnSaveScript;
	runPythonOnSaveScript(globalPythonOnSaveScript.toStdString(), "Global OnSave Script");

	// Project
	if (components::RaCoPreferences::instance().enableProjectPythonScript) {
		const auto projectPythonOnSaveScript = racoApplication_->activeRaCoProject().pythonOnSaveScriptPath();
		runPythonOnSaveScript(projectPythonOnSaveScript, "Project OnSave Script");
	}
}

MainWindow::MainWindow(raco::application::RaCoApplication* racoApplication, ramses_widgets::RendererBackend* rendererBackend, QWidget* parent)
	: QMainWindow(parent),
	  rendererBackend_(rendererBackend),
	  racoApplication_(racoApplication) {
	// Setup the UI from the QtCreator file mainwindow.ui
	ui = new Ui::MainWindow();
	ui->setupUi(this);
	recentFileMenu_ = new OpenRecentMenu(this);
	QObject::connect(recentFileMenu_, &OpenRecentMenu::openProject, this, [this](const QString& file) {
		this->openProject(file);
	});
	ui->menuFile->insertMenu(ui->actionSave, recentFileMenu_);

	updateUpgradeMenu();

	dockManager_ = createDockManager();
	setWindowIcon(QIcon(":applicationLogo"));

	logViewModel_ = new common_widgets::LogViewModel(this);

	// Shortcuts
	{
		const auto undoShortcut = new QShortcut(QKeySequence::Undo, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(undoShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalUndoCallback(racoApplication_);
		});

		const auto redoShortcut = new QShortcut(QKeySequence::Redo, this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(redoShortcut, &QShortcut::activated, this, [this]() {
			EditMenu::globalRedoCallback(racoApplication_);
		});

		ui->actionExport->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
		ui->actionExport->setShortcutContext(Qt::ApplicationShortcut);

		ui->actionNew->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
		ui->actionNew->setShortcutContext(Qt::ApplicationShortcut);

		ui->actionOpen->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
		ui->actionOpen->setShortcutContext(Qt::ApplicationShortcut);

		const auto openRecentShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O), this, nullptr, nullptr, Qt::ApplicationShortcut);
		QObject::connect(openRecentShortcut, &QShortcut::activated, this, [this]() {
			// Open menu File
			const int menuFileX = ui->menuBar->pos().x();
			const int menuFileY = ui->menuBar->pos().y() + ui->menuBar->height();
			ui->menuFile->popup(mapToGlobal(QPoint(menuFileX, menuFileY)));
			// Trigger Open Recent menu
			const auto recentFileAction = recentFileMenu_->menuAction();
			ui->menuFile->setActiveAction(recentFileAction);
			recentFileAction->trigger();
		});

		ui->actionQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
		ui->actionQuit->setShortcutContext(Qt::ApplicationShortcut);

		ui->actionSave->setShortcut(QKeySequence::Save);
		ui->actionSave->setShortcutContext(Qt::ApplicationShortcut);
		QObject::connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveActiveProject);

		ui->actionSaveAs->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
		ui->actionSaveAs->setShortcutContext(Qt::ApplicationShortcut);
		QObject::connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveAsActiveProject);

		ui->actionSaveAsWithNewID->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));
		ui->actionSaveAsWithNewID->setShortcutContext(Qt::ApplicationShortcut);
		QObject::connect(ui->actionSaveAsWithNewID, &QAction::triggered, this, &MainWindow::saveAsActiveProjectWithNewID);
	}

	QObject::connect(ui->actionOpen, &QAction::triggered, [this]() {
		auto file = QFileDialog::getOpenFileName(this, "Open", QString::fromStdString(core::PathManager::getCachedPath(core::PathManager::FolderTypeKeys::Project).string()), "Ramses Composer Assembly (*.rca);; All files (*.*)");
		if (file.size() > 0) {
			openProject(file);
		}
	});
	QObject::connect(ui->actionNew, &QAction::triggered, [this]() {
		openProject();
	});
	QObject::connect(ui->actionExport, &QAction::triggered, this, [this]() {
		auto dialog = new common_widgets::ExportDialog(racoApplication_, logViewModel_, this);
		dialog->exec();
	});
	QObject::connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);

	new EditMenu(racoApplication_, &treeDockManager_, ui->menuEdit);

	QObject::connect(ui->actionPreferences, &QAction::triggered, [this]() {
		auto dialog = new common_widgets::PreferencesView(racoApplication_, this);
		dialog->resize(500, 500);
		dialog->exec();
		racoApplication_->setNewFileFeatureLevel(components::RaCoPreferences::instance().featureLevel);
		racoApplication_->activeRaCoProject().applyPreferences();
	});

	// View actions
	QObject::connect(ui->actionNewPreview, &QAction::triggered, [this]() { createAndAddPreview(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewAbstractSceneView, &QAction::triggered, [this]() { createAndAddAbstractSceneView(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewPropertyBrowser, &QAction::triggered, [this]() { createAndAddPropertyBrowser(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewProjectBrowser, &QAction::triggered, [this]() { createAndAddProjectBrowser(EditorObject::normalizedObjectID("").c_str(), nullptr); });
	QObject::connect(ui->actionNewSceneGraphTree, &QAction::triggered, [this]() { createAndAddSceneGraphTree(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewResourcesTree, &QAction::triggered, [this]() { createAndAddResourceTree(EditorObject::normalizedObjectID("").c_str(), nullptr); });
	QObject::connect(ui->actionNewPrefabTree, &QAction::triggered, [this]() { createAndAddPrefabTree(EditorObject::normalizedObjectID("").c_str(), nullptr); });
	QObject::connect(ui->actionNewRenderView, &QAction::triggered, [this]() { createAndAddRenderView(EditorObject::normalizedObjectID("").c_str(), nullptr); });
	QObject::connect(ui->actionNewUndoView, &QAction::triggered, [this]() { createAndAddUndoView(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewErrorView, &QAction::triggered, [this]() { createAndAddErrorView( EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewLogView, &QAction::triggered, [this]() { createAndAddLogView(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewPythonRunner, &QAction::triggered, [this]() { createAndAddPythonRunner(EditorObject::normalizedObjectID("").c_str()); });
	QObject::connect(ui->actionNewPerformanceTable, &QAction::triggered, [this]() { createAndAddPerformanceTable(EditorObject::normalizedObjectID("").c_str(), nullptr); });
	QObject::connect(ui->actionRestoreDefaultLayout, &QAction::triggered, [this]() {
		resetDockManager();
		createInitialWidgets();
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
			saveDockManagerCustomLayouts();
		}
	});

	QObject::connect(ui->actionManageLayouts, &QAction::triggered, [this]() {
		SavedLayoutsDialog(dockManager_, this).exec();
		saveDockManagerCustomLayouts();
	});

	QObject::connect(ui->actionTracePlayer, &QAction::triggered, [this]() { createAndAddTracePlayer(); });
	QObject::connect(ui->actionProjectSettings, &QAction::triggered, [this]() { createAndAddProjectSettings(EditorObject::normalizedObjectID("").c_str()); });

	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());
	// Help actions
	QObject::connect(ui->actionAbout, &QAction::triggered, [this] {
		VersionDialog about(this);
		about.exec();
	});

	QObject::connect(this, &MainWindow::focusRequestedForTreeDock, &treeDockManager_, [this](auto objectID, auto propertyPath) {
		if (auto object = racoApplication_->activeRaCoProject().project()->getInstanceByID(objectID.toStdString())) {
			treeDockManager_.selectObjectAndPropertyAcrossAllTreeDocks(object, propertyPath);
		}
	});

	setAcceptDrops(true);

	updateProjectSavedConnection();

	restoreSettings();
	restoreCachedLayout();

	// Setup
	updateApplicationTitle();
	updateSavedLayoutMenu();
	
	gui_python_api::setupObjectTree(&treeDockManager_);

	// Will we support Mac?
	setUnifiedTitleAndToolBarOnMac(true);

	renderTimerId_ = startTimer(timerInterval60Fps);
}

void MainWindow::saveDockManagerCustomLayouts() {
	auto settings = PathManager::layoutSettings();
	dockManager_->saveCustomLayouts(settings);
	settings.sync();
	if (settings.status() != QSettings::NoError) {
		LOG_ERROR(log_system::COMMON, "Saving custom layout failed: {}", core::PathManager::recentFilesStorePath().string());
		QMessageBox::critical(this, "Saving custom layout failed", QString("Custom layout data could not be saved to disk and will be lost after closing Ramses Composer. Check whether the application can write to its config directory.\nFile: ") 
			+ QString::fromStdString(PathManager::layoutFilePath().string()));
	}
}

void MainWindow::timerEvent(QTimerEvent* event) {
	auto startLoop = std::chrono::high_resolution_clock::now();
	racoApplication_->doOneLoop();

	auto& viewport = racoApplication_->activeRaCoProject().project()->settings()->viewport_;
	const auto& backgroundColor = *racoApplication_->activeRaCoProject().project()->settings()->backgroundColor_;

	int range_max = *viewport->i1_.staticQuery<core::RangeAnnotation<int>>().max_;
	Q_EMIT viewportChanged(QSize(*viewport->i1_, *viewport->i2_).boundedTo({range_max, range_max}).expandedTo({1, 1}));

	for (auto preview : findChildren<ramses_widgets::PreviewMainWindow*>()) {
		preview->commit(racoApplication_->rendererDirty_);
		preview->displayScene(racoApplication_->sceneBackendImpl()->currentSceneId(), backgroundColor);
	}
	racoApplication_->rendererDirty_ = false;
	auto logicEngineExecutionEnd = std::chrono::high_resolution_clock::now();
	racoApplication_->sceneBackendImpl()->flush();
	if (racoApplication_->abstractScene()) {
		racoApplication_->abstractScene()->scene()->flush();
	}

	rendererBackend_->doOneLoop();
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (resolveDirtiness()) {
		killTimer(renderTimerId_);
		auto settings = core::PathManager::layoutSettings();
		settings.setValue("geometry", saveGeometry());
		settings.setValue("windowState", saveState());
		dockManager_->saveCurrentLayoutInCache(settings);
		
		settings.sync();		
		if (settings.status() != QSettings::NoError) {
			LOG_WARNING(log_system::COMMON, "Saving layout failed: {}", core::PathManager::recentFilesStorePath().string());
		}
		QMainWindow::closeEvent(event);
		event->accept();
	} else {
		event->ignore();
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
	const QFileInfo fileInfo = getDragAndDropFileInfo(event);
	if (fileInfo.suffix().toLower() == "rca") {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* event) {
	const QFileInfo fileInfo = getDragAndDropFileInfo(event);
	if (fileInfo.suffix().toLower() == "rca") {
		openProject(fileInfo.absoluteFilePath());
	}
}

QFileInfo MainWindow::getDragAndDropFileInfo(const QDropEvent* event) {
	const QList<QUrl> urls = event->mimeData()->urls();
	if (urls.empty()) {
		return {};
	}

	const QString filePath = urls.first().toLocalFile();
	if (filePath.isEmpty()) {
		return {};
	}

	const QFile file(filePath);
	return QFileInfo{file};
}

void MainWindow::restoreSettings() {
	if (PathManager::layoutFilePath().existsFile()) {
		auto settings = core::PathManager::layoutSettings();
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState").toByteArray());
	}
}

void MainWindow::openProject(const QString& file, int featureLevel, bool generateNewObjectIDs, bool ignoreDirtiness) {
	auto fileString = file.toStdString();
	if (!fileString.empty() && (!utils::u8path(fileString).exists() || !utils::u8path(fileString).userHasReadAccess())) {
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file {} is not available for loading.\n\nCheck whether the file at the specified path still exists and that you have read access to that file.", fileString).c_str(), QMessageBox::Close);
		return;
	}

	if (!ignoreDirtiness && !resolveDirtiness()) {
		return;
	}

	if (file.size() > 0) {
		recentFileMenu_->addRecentFile(file);
	}

	{
		auto settings = core::PathManager::layoutSettings();
		dockManager_->saveCurrentLayoutInCache(settings);
		settings.sync();
		if (settings.status() != QSettings::NoError) {
			LOG_WARNING(log_system::COMMON, "Saving layout failed: {}", core::PathManager::recentFilesStorePath().string());
		}
	}

	// Delete all ui widgets (and their listeners) before changing the project
	// Don't create a new DockManager right away - making QMessageBoxes pop up messes up state restoring
	// (see https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/315)
	delete dockManager_;
	logViewModel_->clear();

	killTimer(renderTimerId_);

	try {
		auto relinkCallback = [this](const std::string& projectPath) -> std::string {
			auto answer = QMessageBox::warning(this, "External Project Not Found: Relink?",
				fmt::format("External project '{}' was not found!\n\nSpecify replacement project and relink?", projectPath).c_str(),
				QMessageBox::Yes | QMessageBox::No);
			if (answer == QMessageBox::Yes) {
				auto projectDirectory = utils::u8path(projectPath).normalized().parent_path().string();
				auto file = QFileDialog::getOpenFileName(this,
					"Replace: " + QString::fromStdString(projectPath),
					QString::fromStdString(projectDirectory),
					"Ramses Composer Assembly (*.rca)");
				return file.toStdString();
			}
			return std::string();
		};

		int loadFeatureLevel = featureLevel;
		while (true) {
			try {
				racoApplication_->switchActiveRaCoProject(file, relinkCallback, true, loadFeatureLevel, generateNewObjectIDs);
				break;
			} catch (const ExtrefError& error) {
				if (auto flError = racoApplication_->getFlError()) {
					auto answer = QMessageBox::warning(this,
						"Feature Level Error: Upgrade Feature Level?",
						fmt::format("Project '{}' feature level {} smaller than external project '{}' feature level {}.\n\nUpgrade project '{}' feature level to {}?", file.toStdString(), flError->currentFeatureLevel_, flError->projectPath_, flError->projectFeatureLevel_, file.toStdString(), flError->projectFeatureLevel_).c_str(),
						QMessageBox::Yes | QMessageBox::No);
					if (answer == QMessageBox::Yes) {
						loadFeatureLevel = flError->projectFeatureLevel_;
						continue;
					}
				}
				throw error;
			}
		}

	} catch (const application::FutureFileVersion& error) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file was created with newer version of {app_name}. Please upgrade.\n\nExpected File Version: {expected_file_version}\nFound File Version: {file_version}", fmt::arg("app_name", "Ramses Composer"), fmt::arg("expected_file_version", serialization::RAMSES_PROJECT_FILE_VERSION), fmt::arg("file_version", error.fileVersion_)).c_str(), QMessageBox::Close);
	} catch (const ExtrefError& error) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("External reference update failed.\n\n{}", error.what()).c_str(), QMessageBox::Close);
	} catch (const std::exception& e) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file {} could not be loaded.\n\nReported error: {}\n\nCheck whether the file has been broken or corrupted.", fileString, e.what()).c_str(), QMessageBox::Close);
	}

	renderTimerId_ = startTimer(timerInterval60Fps);

	// Recreate our layout with new context
	dockManager_ = createDockManager();
	restoreCachedLayout();
	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());

	updateApplicationTitle();
	updateActiveProjectConnection();
	updateProjectSavedConnection();
	updateUpgradeMenu();

	for (auto abstractView : findChildren<ramses_widgets::AbstractViewMainWindow*>()) {
		abstractView->focusCamera(racoApplication_->activeRaCoProject().project()->instances());
	}
}

MainWindow::~MainWindow() {
	resetDockManager();
	// sceneBackend needs to be reset first to unregister all adaptors (and their file listeners)
	// before the file change monitors and mesh caches get destroyed
	racoApplication_->resetSceneBackend();
	killTimer(renderTimerId_);
	delete ui;
}

void MainWindow::updateApplicationTitle() {
	setWindowTitle(racoApplication_->generateApplicationTitle());
}

bool MainWindow::upgradeActiveProject(int newFeatureLevel) {
	if (racoApplication_->activeProjectPath().empty()) {
		QMessageBox::warning(this, "Upgrade Error", "Can't upgrade projects with empty path.");
		return false;
	}

	const int currentFeatureLevel = racoApplication_->activeRaCoProject().project()->featureLevel();
	const auto upgradeConfirmed = QMessageBox::question(this, "Upgrade feature level",
		"The scene will be upgraded from feature level " + QString::number(currentFeatureLevel) +
			" to feature level " + QString::number(newFeatureLevel) + ". This can't be reverted after the scene is saved! Are you sure?");

	if (upgradeConfirmed == QMessageBox::Yes) {
		if (racoApplication_->canSaveActiveProject()) {
			std::string errorMsg;
			if (racoApplication_->activeRaCoProject().save(errorMsg)) {
				openProject(QString::fromStdString(racoApplication_->activeProjectPath()), newFeatureLevel);
				return true;
			} else {
				updateApplicationTitle();
				QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
			}
		} else {
			QMessageBox::warning(this, "Save Error", fmt::format("Can not save project: externally referenced projects not clean.").c_str(), QMessageBox::Ok);
		}
	}
	
	return false;
}

bool MainWindow::promptToReloadActiveProject() {
	if (racoApplication_->activeProjectPath().empty()) {
		return false;
	}

	auto reloadConfirmed = QMessageBox::NoButton;

	if (racoApplication_->activeRaCoProject().dirty()) {
		reloadConfirmed = QMessageBox::warning(this, "Project File Modification Detected",
			"Active project file has unsaved changes and has been changed externally! Do you want to reload it and lose all unsaved changes?",
			QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
	} else {
		reloadConfirmed = QMessageBox::question(this, "Project File Modification Detected",
			"Active project file has been changed externally. Do you want to reload it?"	);
	}

	if (reloadConfirmed == QMessageBox::Yes) {
		openProject(QString::fromStdString(racoApplication_->activeProjectPath()), racoApplication_->activeRaCoProject().project()->featureLevel(), false, true);
		return true;
	}

	return false;
}

bool MainWindow::saveActiveProject() {
	if (racoApplication_->canSaveActiveProject()) {
		if (racoApplication_->activeProjectPath().empty()) {
			return saveAsActiveProject();
		} else {
			if (isUpgradePrevented()) {
				return false;
			}
			
			runPythonOnSaveScripts();

			std::string errorMsg;
			if (racoApplication_->activeRaCoProject().save(errorMsg)) {
				updateUpgradeMenu();
				return true;
			} else {
				updateApplicationTitle();	
				QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
			}
			
		}
	} else {
		QMessageBox::warning(this, "Save Error", fmt::format("Can not save project: externally referenced projects not clean.").c_str(), QMessageBox::Ok);
	}
	return false;
}

bool MainWindow::isUpgradePrevented() {
	if (components::RaCoPreferences::instance().preventAccidentalUpgrade) {
		const auto filename = QString::fromStdString(racoApplication_->activeProjectPath());
		constexpr auto currentFileVersion = serialization::RAMSES_PROJECT_FILE_VERSION;
		
		try {
			auto previousFileVersion = serialization::deserializeFileVersion(application::RaCoProject::loadJsonDocument(filename));
			if (currentFileVersion > previousFileVersion) {
				const auto answer = QMessageBox::warning(this, "Save File Warning", fmt::format("The project with the file version {} will be overwritten with the file version {}. Are you sure you want to save it with the new file version", previousFileVersion, currentFileVersion).c_str(), QMessageBox::Save, QMessageBox::Cancel);
				if (answer == QMessageBox::Cancel) {
					return true;
				}
			}
		} catch (const std::exception& e) {
			QMessageBox::warning(this, "Document Load Error", fmt::format("Document could not be loaded.\n\nReported error: {}\n\nCheck whether the file has been broken or corrupted.", e.what()).c_str(), QMessageBox::Close);
		}
	}
	return false;
}

bool MainWindow::saveAsActiveProject(bool newID) {
	if (racoApplication_->canSaveActiveProject()) {
		const bool setProjectName = racoApplication_->activeProjectPath().empty();
		const auto dialogCaption = newID ? "Save As with new ID..." : "Save As...";
		auto newPath = QFileDialog::getSaveFileName(this, dialogCaption, QString::fromStdString(core::PathManager::getCachedPath(core::PathManager::FolderTypeKeys::Project).string()), "Ramses Composer Assembly (*.rca)");
		if (newPath.isEmpty()) {
			return false;
		}
		if (!newPath.endsWith(".rca")) newPath += ".rca";
		std::string errorMsg;

		runPythonOnSaveScripts();

		if (newID) {
			if (racoApplication_->activeRaCoProject().saveAs(newPath, errorMsg, setProjectName)) {
				openProject(QString::fromStdString(racoApplication_->activeProjectPath()), -1, true);
				if (racoApplication_->activeRaCoProject().save(errorMsg)) {
					updateActiveProjectConnection();
					updateProjectSavedConnection();
					updateUpgradeMenu();
					return true;
				} else {
					updateApplicationTitle();
					QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
				}
			} else {
				updateApplicationTitle();
				QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
			}
		} else {
			if (racoApplication_->activeRaCoProject().saveAs(newPath, errorMsg, setProjectName)) {
				updateActiveProjectConnection();
				updateProjectSavedConnection();
				updateUpgradeMenu();
				return true;
			} else {
				updateApplicationTitle();
				QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
			}
		}
	} else {
		QMessageBox::warning(this, "Save Error", fmt::format("Can not save project: externally referenced projects not clean.").c_str(), QMessageBox::Ok);
	}
	return false;
}

bool MainWindow::saveAsActiveProjectWithNewID() {
	return saveAsActiveProject(true);
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

void MainWindow::updateUpgradeMenu() {
	auto maxFeatureLevel = static_cast<int>(ramses_base::BaseEngineBackend::maxFeatureLevel);
	auto currentFeatureLevel = racoApplication_->activeRaCoProject().project()->featureLevel();
	if ((currentFeatureLevel < maxFeatureLevel) && (!racoApplication_->activeProjectPath().empty())) {
		ui->menuUpgrade->clear();
		for (int fl = currentFeatureLevel + 1; fl <= maxFeatureLevel; fl++) {
			ui->menuUpgrade->addAction(QString::fromStdString(fmt::format("&{}. Feature Level {}", fl, fl)), [fl, this]() {
				upgradeActiveProject(fl);
			});
		}
		ui->menuUpgrade->setDisabled(false);
	} else {
		ui->menuUpgrade->setDisabled(true);
	}
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
			return saveActiveProject();
		}
	}
	return continueWithAction;
}

QString MainWindow::getActiveProjectFolder() {
	return QString::fromStdString(racoApplication_->activeProjectFolder());
}

void MainWindow::restoreCachedLayout() {
	auto cachedLayoutInfo = dockManager_->getCachedLayoutInfo();
	if (cachedLayoutInfo.empty()) {
		createInitialWidgets();

#ifdef Q_OS_WIN
		// explicit maximization of docks needed or else RaCo will not look properly maximized on Windows
		dockManager_->showMaximized();
#endif
		showMaximized();
	} else {
		regenerateLayoutDocks(cachedLayoutInfo);

		dockManager_->restoreCachedLayoutState();
	}
}

void MainWindow::restoreCustomLayout(const QString& layoutName) {
	// reset needed to delete all dock widgets of the previous layout
	resetDockManager();

	auto extraLayoutInfo = dockManager_->getCustomLayoutInfo(layoutName);
	regenerateLayoutDocks(extraLayoutInfo);

	dockManager_->openPerspective(layoutName);
}

void MainWindow::regenerateLayoutDocks(const RaCoDockManager::LayoutDocks& docks) {
	ui->actionNewPreview->setEnabled(true);
	ui->actionNewAbstractSceneView->setEnabled(true);
	auto hasPreview = false;
	bool hasAbstractSceneView = false;
	for (const auto& [savedDockType, savedDockName] : docks) {
		auto dockNameString = savedDockName.toStdString();
		auto dockNameCString = dockNameString.c_str();
		if (savedDockType == DockWidgetTypes::PREFABS) {
			createAndAddPrefabTree(dockNameCString, nullptr);
		} else if (savedDockType == DockWidgetTypes::PROJECT_BROWSER) {
			createAndAddProjectBrowser(dockNameCString, nullptr);
		} else if (savedDockType == DockWidgetTypes::PROJECT_SETTINGS) {
			createAndAddProjectSettings(dockNameCString);
		} else if (savedDockType == DockWidgetTypes::PROPERTY_BROWSER) {
			createAndAddPropertyBrowser(dockNameCString);
		} else if (savedDockType == DockWidgetTypes::PERFORMANCE_TABLE) {
			createAndAddPerformanceTable(dockNameCString, nullptr);
		} else if (savedDockType == DockWidgetTypes::RAMSES_PREVIEW) {
			if (!hasPreview) {
				createAndAddPreview(dockNameCString);
				// prevent loading of multiple preview windows
				hasPreview = true;
			}
		} else if (savedDockType == DockWidgetTypes::ABSTRACT_SCENE_VIEW) {
			if (!hasAbstractSceneView) {
				createAndAddAbstractSceneView(dockNameCString);
				// prevent loading of multiple windows
				hasAbstractSceneView = true;
			}
		} else if (savedDockType == DockWidgetTypes::RESOURCES) {
			createAndAddResourceTree(dockNameCString, nullptr);
		} else if (savedDockType == DockWidgetTypes::SCENE_GRAPH) {
			createAndAddSceneGraphTree(dockNameCString);
		}  else if (savedDockType == DockWidgetTypes::RENDER_VIEW) {
			createAndAddRenderView(dockNameCString, nullptr);
		} else if (savedDockType == DockWidgetTypes::UNDO_STACK) {
			createAndAddUndoView(dockNameCString);
		} else if (savedDockType == DockWidgetTypes::ERROR_VIEW) {
			createAndAddErrorView(dockNameCString);
		} else if (savedDockType == DockWidgetTypes::LOG_VIEW) {
			createAndAddLogView(dockNameCString);
		} else if (savedDockType == DockWidgetTypes::TRACE_PLAYER) {
			createAndAddTracePlayer();
		} else if (savedDockType == DockWidgetTypes::PYTHON_RUNNER) {
			createAndAddPythonRunner(dockNameCString);
		} else {
			LOG_DEBUG(log_system::COMMON, "Ignoring unknown dock type '{}'.", savedDockType.toStdString());
		}
	}
}

void MainWindow::resetDockManager() {
	delete dockManager_;
	dockManager_ = createDockManager();
}

void MainWindow::activeProjectFileChanged() {
	updateApplicationTitle();
	promptToReloadActiveProject();
}

void MainWindow::updateActiveProjectConnection() {
	QObject::disconnect(activeProjectFileConnection_);
	if (!racoApplication_->activeProjectPath().empty()) {
		activeProjectFileConnection_ = QObject::connect(
			&racoApplication_->activeRaCoProject(),
			&application::RaCoProject::activeProjectFileChanged,
			this,
			&MainWindow::activeProjectFileChanged,
			Qt::QueuedConnection);
	}
}

void MainWindow::updateProjectSavedConnection() {
	QObject::disconnect(projectSavedConnection_);
	projectSavedConnection_ = QObject::connect(&racoApplication_->activeRaCoProject(), &application::RaCoProject::projectSuccessfullySaved, [this]() {
		recentFileMenu_->addRecentFile(racoApplication_->activeProjectPath().c_str());
		updateApplicationTitle();
	});
}

void MainWindow::focusToSelection(const QString& objectID, const QString& objectProperty) {
	if (racoApplication_->activeRaCoProject().project()->getInstanceByID(objectID.toStdString())) {
		if (treeDockManager_.getTreeDockAmount() != 0 && treeDockManager_.docksContainObject(objectID)) {
			Q_EMIT focusRequestedForTreeDock(objectID, objectProperty);
		} else {
			Q_EMIT focusRequestedForPropertyBrowser(objectID, objectProperty);
		}
	}
}

void MainWindow::showMeshImportErrorMessage(const std::string& filePath, const std::string& meshError) {
	auto filePathQString = QString::fromStdString(filePath);
	auto dialogText = meshError.empty() ? QString{"Ramses Composer encountered an unknown error while importing assets from %1.\nConsult with the logs or file contents to find the error."}.arg(filePathQString)
										: QString{"Ramses Composer encountered the following error while importing assets from %1:\n\n%2"}.arg(filePathQString).arg(meshError.c_str());

	QMessageBox importErrorBox(QMessageBox::Critical, "Mesh Import Error", dialogText, QMessageBox::Ok, this);
	importErrorBox.setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	importErrorBox.exec();
}
