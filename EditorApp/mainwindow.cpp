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
#include "common_widgets/ErrorView.h"
#include "common_widgets/ExportDialog.h"
#include "common_widgets/LogView.h"
#include "common_widgets/MeshAssetImportDialog.h"
#include "common_widgets/PreferencesView.h"
#include "common_widgets/RunScriptDialog.h"
#include "common_widgets/TracePlayerWidget.h"
#include "common_widgets/UndoView.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/Handles.h"
#include "core/PathManager.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/BasicTypes.h"
#include "data_storage/Value.h"
#include "gui_python_api/GUIPythonAPI.h"
#include "log_system/log.h"
#include "object_tree_view/ObjectTreeDock.h"
#include "object_tree_view/ObjectTreeView.h"
#include "object_tree_view_model/ObjectTreeViewDefaultModel.h"
#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "object_tree_view_model/ObjectTreeViewSortProxyModels.h"
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
#include "core/ProjectMigration.h"
#include "core/Serialization.h"
#include "ui_mainwindow.h"

#include "user_types/AnchorPoint.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
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
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"
#include "user_types/Timer.h"
#include "user_types/Texture.h"

#include "utils/FileUtils.h"
#include "versiondialog.h"
#include "utils/u8path.h"

#include "python_api/PythonAPI.h"
#include "DockAreaWidget.h"
#include "DockSplitter.h"
#include "ads_globals.h"

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
#include <algorithm>
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

	if (PathManager::layoutFilePath().existsFile()) {
		auto settings = raco::core::PathManager::layoutSettings();
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
	const auto& backgroundColor = *application->activeRaCoProject().project()->settings()->backgroundColor_;
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
}

ads::CDockAreaWidget* createAndAddPropertyBrowser(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* application) {
	auto propertyBrowser = new raco::property_browser::PropertyBrowserWidget(application->dataChangeDispatcher(), application->activeRaCoProject().commandInterface(), application->sceneBackendImpl(), mainWindow);
	QObject::connect(propertyBrowser->model(), &raco::property_browser::PropertyBrowserModel::objectSelectionRequested, mainWindow, &MainWindow::focusToObject);
	QObject::connect(mainWindow, &MainWindow::objectFocusRequestedForPropertyBrowser, propertyBrowser, &raco::property_browser::PropertyBrowserWidget::setValueHandleFromObjectId);
	connectPropertyBrowserAndTreeDockManager(propertyBrowser, treeDockManager);
	auto* dockWidget = createDockWidget(MainWindow::DockWidgetTypes::PROPERTY_BROWSER, mainWindow);
	dockWidget->setWidget(propertyBrowser, ads::CDockWidget::ForceNoScrollArea);
	dockWidget->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::RightDockWidgetArea, dockWidget);
}

void createAndAddProjectSettings(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::application::RaCoProject* project, SDataChangeDispatcher dataChangeDispatcher, CommandInterface* commandInterface, SceneBackend* sceneBackend) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PROJECT_SETTINGS, mainWindow);
	dock->setObjectName(dockObjName);
	auto propertyBrowser = new raco::property_browser::PropertyBrowserWidget(dataChangeDispatcher, commandInterface, sceneBackend, mainWindow);
	propertyBrowser->setValueHandle({project->project()->settings()});
	propertyBrowser->setLockable(false);
	dock->setWidget(propertyBrowser);
	dockManager->addDockWidget(ads::RightDockWidgetArea, dock);
}

ads::CDockAreaWidget* createAndAddObjectTree(const char* title, const char* dockObjName, raco::object_tree::model::ObjectTreeViewDefaultModel* dockModel, raco::object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel* sortFilterModel, ads::DockWidgetArea area, MainWindow* mainWindow, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, ads::CDockAreaWidget* dockArea) {
	auto* dockObjectView = new raco::object_tree::view::ObjectTreeDock(title, mainWindow);
	QObject::connect(dockModel, &raco::object_tree::model::ObjectTreeViewDefaultModel::meshImportFailed, mainWindow, &MainWindow::showMeshImportErrorMessage);
	dockModel->buildObjectTree();
	auto newTreeView = new raco::object_tree::view::ObjectTreeView(title, dockModel, sortFilterModel);
	if (sortFilterModel && sortFilterModel->sortingEnabled()) {
		newTreeView->sortByColumn(raco::object_tree::model::ObjectTreeViewDefaultModel::COLUMNINDEX_TYPE, Qt::SortOrder::AscendingOrder);
	}
	dockObjectView->setTreeView(newTreeView);
	treeDockManager.addTreeDock(dockObjectView);
	dockModel->setParent(dockObjectView);

	dockObjectView->setObjectName(dockObjName);
	return dockManager->addDockWidget(area, dockObjectView, dockArea);
}

ads::CDockAreaWidget* createAndAddProjectBrowser(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication, ads::CDockAreaWidget* dockArea) {
	auto* model = new raco::object_tree::model::ObjectTreeViewExternalProjectModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects());
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::PROJECT_BROWSER, dockObjName, model, new raco::object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel(mainWindow), ads::BottomDockWidgetArea, mainWindow, dockManager, treeDockManager, dockArea);
}

ads::CDockAreaWidget* createAndAddResourceTree(MainWindow* mainWindow, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::application::RaCoApplication* racoApplication, ads::CDockAreaWidget* dockArea) {
	using namespace raco::user_types;

	static const std::vector<std::string> allowedCreateableUserTypes{
		AnchorPoint::typeDescription.typeName,
		AnimationChannel::typeDescription.typeName,
		CubeMap::typeDescription.typeName,
		LuaScriptModule::typeDescription.typeName,
		Material::typeDescription.typeName,
		Mesh::typeDescription.typeName,
		Texture::typeDescription.typeName,
		Timer::typeDescription.typeName,
		RenderBuffer::typeDescription.typeName,
		RenderTarget::typeDescription.typeName,
		RenderLayer::typeDescription.typeName,
		RenderPass::typeDescription.typeName};

	auto* model = new raco::object_tree::model::ObjectTreeViewResourceModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);
	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::RESOURCES, dockObjName, model, new raco::object_tree::model::ObjectTreeViewTopLevelSortFilterProxyModel(mainWindow),
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
		Animation::typeDescription.typeName,
		LuaScript::typeDescription.typeName,
		LuaInterface::typeDescription.typeName};

	auto* model = new raco::object_tree::model::ObjectTreeViewPrefabModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);

	return createAndAddObjectTree(
		MainWindow::DockWidgetTypes::PREFABS, dockObjName, model, new raco::object_tree::model::ObjectTreeViewTopLevelSortFilterProxyModel(mainWindow),
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
		Animation::typeDescription.typeName,
		LuaScript::typeDescription.typeName,
		LuaInterface::typeDescription.typeName};

	auto* model = new raco::object_tree::model::ObjectTreeViewDefaultModel(racoApplication->activeRaCoProject().commandInterface(), racoApplication->dataChangeDispatcher(), racoApplication->externalProjects(), allowedCreateableUserTypes);
	return createAndAddObjectTree(MainWindow::DockWidgetTypes::SCENE_GRAPH, dockObjName, model, new raco::object_tree::model::ObjectTreeViewDefaultSortFilterProxyModel(mainWindow, false),
		ads::LeftDockWidgetArea, mainWindow, dockManager, treeDockManager, nullptr);
}

ads::CDockAreaWidget* createAndAddUndoView(raco::application::RaCoApplication* application, const char* dockObjName, raco::application::RaCoProject* project, MainWindow* mainWindow, RaCoDockManager* dockManager, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::UNDO_STACK, mainWindow);
	dock->setWidget(new raco::common_widgets::UndoView(project->undoStack(), application->dataChangeDispatcher(), mainWindow));
	dock->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* createAndAddErrorView(MainWindow* mainWindow, raco::application::RaCoApplication* application, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::common_widgets::LogViewModel* logViewModel, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* errorView = new raco::common_widgets::ErrorView(application->activeRaCoProject().commandInterface(), application->dataChangeDispatcher(), false, logViewModel);
	QObject::connect(errorView, &raco::common_widgets::ErrorView::objectSelectionRequested, &treeDockManager, &raco::object_tree::view::ObjectTreeDockManager::selectObjectAcrossAllTreeDocks);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::ERROR_VIEW, mainWindow);
	dock->setWidget(errorView);
	dock->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* createAndAddLogView(MainWindow* mainWindow, raco::application::RaCoApplication* application, const char* dockObjName, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager, raco::common_widgets::LogViewModel* logViewModel, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* logView = new raco::common_widgets::LogView(logViewModel);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::LOG_VIEW, mainWindow);
	dock->setWidget(logView);
	dock->setObjectName(dockObjName);
	return dockManager->addDockWidget(ads::BottomDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* createAndAddPythonRunner(MainWindow* mainWindow, raco::application::RaCoApplication* application, const char* dockObjName, std::map<QString, qint64>& scriptEntries, std::map<QString, qint64>& commandLineParamEntries, RaCoDockManager* dockManager, ads::CDockAreaWidget* dockArea = nullptr) {
	auto* pythonRunner = new raco::common_widgets::RunScriptDialog(scriptEntries, commandLineParamEntries, mainWindow);
	auto* dock = createDockWidget(MainWindow::DockWidgetTypes::PYTHON_RUNNER, mainWindow);
	dock->setWidget(pythonRunner);
	dock->setObjectName(dockObjName);

	QObject::connect(pythonRunner, &raco::common_widgets::RunScriptDialog::pythonScriptRunRequested, [mainWindow, application, dock, pythonRunner](const QString& scriptPath, const QStringList& arguments) {
		pythonRunner->setScriptIsRunning(true);
		pythonRunner->repaint();
		std::vector<std::string> pos_argv_s;
		pos_argv_s.emplace_back(scriptPath.toStdString());
		for (auto arg : arguments) {
			pos_argv_s.emplace_back(arg.toStdString());
		}
		std::vector<const char*> pos_argv_cp;
		for (auto& s : pos_argv_s) {
			pos_argv_cp.emplace_back(s.c_str());
		}

		auto currentRunStatus = raco::python_api::runPythonScript(application, QCoreApplication::applicationFilePath().toStdWString(), scriptPath.toStdString(), mainWindow->pythonSearchPaths(), pos_argv_cp);
		pythonRunner->addPythonOutput(currentRunStatus.stdOutBuffer, currentRunStatus.stdErrBuffer);
		pythonRunner->setScriptIsRunning(false);
	});

	return dockManager->addDockWidget(ads::RightDockWidgetArea, dock, dockArea);
}

ads::CDockAreaWidget* createAndAddTracePlayer(MainWindow* mainWindow, RaCoDockManager* dockManager, raco::components::TracePlayer* tracePlayer) {
	if (auto existingTraceplayerDock{dockManager->findDockWidget(MainWindow::DockWidgetTypes::TRACE_PLAYER)}) {
		return existingTraceplayerDock->dockAreaWidget();
	}

	auto* newTraceplayerDock{createDockWidget(MainWindow::DockWidgetTypes::TRACE_PLAYER, mainWindow)};
	newTraceplayerDock->setMinimumSizeHintMode(ads::CDockWidget::eMinimumSizeHintMode::MinimumSizeHintFromContent);

	auto* traceplayerWidget{new raco::common_widgets::TracePlayerWidget(newTraceplayerDock->objectName(), tracePlayer)};
	newTraceplayerDock->setWidget(traceplayerWidget, ads::CDockWidget::ForceNoScrollArea);

	ads::CDockWidget* existingPreviewDock{nullptr};
	constexpr auto isRamsesPreviewWidget{
		[](const ads::CDockWidget* dockWidget) { return dockWidget->windowTitle() == MainWindow::DockWidgetTypes::RAMSES_PREVIEW; }};
	const auto& dockWidgetsMap{dockManager->dockWidgetsMap()};
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

	return dockManager->addDockWidget(ads::TopDockWidgetArea, newTraceplayerDock, previewDockArea);
}

void createInitialWidgets(MainWindow* mainWindow, raco::ramses_widgets::RendererBackend& rendererBackend, raco::application::RaCoApplication* application, RaCoDockManager* dockManager, raco::object_tree::view::ObjectTreeDockManager& treeDockManager) {
	createAndAddPreview(mainWindow, "defaultPreview", dockManager, rendererBackend, application);

	auto leftDockArea = createAndAddSceneGraphTree(mainWindow, "defaultSceneGraph", dockManager, treeDockManager, application);
	leftDockArea = createAndAddResourceTree(mainWindow, "defaultResourceTree", dockManager, treeDockManager, application, leftDockArea);
	createAndAddPrefabTree(mainWindow, "defaultPrefabTree", dockManager, treeDockManager, application, leftDockArea);

	createAndAddUndoView(application, "defaultUndoView", &application->activeRaCoProject(), mainWindow, dockManager, leftDockArea);

	createAndAddPropertyBrowser(mainWindow, "defaultPropertyBrowser", dockManager, treeDockManager, application);
}

}  // namespace

MainWindow::MainWindow(raco::application::RaCoApplication* racoApplication, raco::ramses_widgets::RendererBackend* rendererBackend, const std::vector<std::wstring>& pythonSearchPaths, QWidget* parent)
	: QMainWindow(parent),
	  pythonSearchPaths_(pythonSearchPaths),
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

	upgradeMenu_ = new QMenu("&Upgrade Feature Level", this);
	ui->menuFile->insertMenu(ui->actionSave, upgradeMenu_);
	upgradeMenu_->setDisabled(true);

	dockManager_ = createDockManager(this);
	setWindowIcon(QIcon(":applicationLogo"));

	logViewModel_ = new raco::common_widgets::LogViewModel(this);

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
		auto file = QFileDialog::getOpenFileName(this, "Open", QString::fromStdString(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project).string()), "Ramses Composer Assembly (*.rca);; All files (*.*)");
		if (file.size() > 0) {
			openProject(file);
		}
	});
	QObject::connect(ui->actionNew, &QAction::triggered, [this]() {
		openProject();
	});
	QObject::connect(ui->actionExport, &QAction::triggered, this, [this]() {
		auto dialog = new raco::common_widgets::ExportDialog(racoApplication_, logViewModel_, this);
		dialog->exec();
	});
	QObject::connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);

	new EditMenu(racoApplication_, &treeDockManager_, ui->menuEdit);

	QObject::connect(ui->actionPreferences, &QAction::triggered, [this]() {
		auto dialog = new raco::common_widgets::PreferencesView(this);
		dialog->resize(500, 500);
		dialog->exec();
		racoApplication_->setApplicationFeatureLevel(raco::components::RaCoPreferences::instance().featureLevel);
	});

	// View actions
	QObject::connect(ui->actionNewPreview, &QAction::triggered, [this]() { createAndAddPreview(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, *rendererBackend_, racoApplication_); });
	QObject::connect(ui->actionNewPropertyBrowser, &QAction::triggered, [this]() { createAndAddPropertyBrowser(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_); });
	QObject::connect(ui->actionNewProjectBrowser, &QAction::triggered, [this]() { createAndAddProjectBrowser(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewSceneGraphTree, &QAction::triggered, [this]() { createAndAddSceneGraphTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_); });
	QObject::connect(ui->actionNewResourcesTree, &QAction::triggered, [this]() { createAndAddResourceTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewPrefabTree, &QAction::triggered, [this]() { createAndAddPrefabTree(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, racoApplication_, nullptr); });
	QObject::connect(ui->actionNewUndoView, &QAction::triggered, [this]() { createAndAddUndoView(racoApplication_, EditorObject::normalizedObjectID("").c_str(), &racoApplication_->activeRaCoProject(), this, dockManager_); });
	QObject::connect(ui->actionNewErrorView, &QAction::triggered, [this]() { createAndAddErrorView(this, racoApplication_, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, logViewModel_); });
	QObject::connect(ui->actionNewLogView, &QAction::triggered, [this]() { createAndAddLogView(this, racoApplication_, EditorObject::normalizedObjectID("").c_str(), dockManager_, treeDockManager_, logViewModel_); });
	QObject::connect(ui->actionNewPythonRunner, &QAction::triggered, [this]() { createAndAddPythonRunner(this, racoApplication_, EditorObject::normalizedObjectID("").c_str(), pythonScriptCache_, pythonScriptArgumentCache_, dockManager_); });
	QObject::connect(ui->actionRestoreDefaultLayout, &QAction::triggered, [this]() {
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
			saveDockManagerCustomLayouts();
		}
	});

	QObject::connect(ui->actionManageLayouts, &QAction::triggered, [this]() {
		SavedLayoutsDialog(dockManager_, this).exec();
		saveDockManagerCustomLayouts();
	});

	QObject::connect(ui->actionTracePlayer, &QAction::triggered, [this]() { createAndAddTracePlayer(this, dockManager_, &racoApplication_->activeRaCoProject().tracePlayer()); });
	QObject::connect(ui->actionProjectSettings, &QAction::triggered, [this]() { createAndAddProjectSettings(this, EditorObject::normalizedObjectID("").c_str(), dockManager_, &racoApplication_->activeRaCoProject(), racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->sceneBackendImpl()); });

	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());
	// Help actions
	QObject::connect(ui->actionAbout, &QAction::triggered, [this] {
		VersionDialog about(this);
		about.exec();
	});

	QObject::connect(this, &MainWindow::objectFocusRequestedForTreeDock, &treeDockManager_, &raco::object_tree::view::ObjectTreeDockManager::selectObjectAcrossAllTreeDocks);

	updateProjectSavedConnection();

	restoreSettings();
	restoreCachedLayout();

	// Setup
	updateApplicationTitle();
	updateSavedLayoutMenu();
	raco::gui_python_api::setup(&treeDockManager_);

	// Will we support Mac?
	setUnifiedTitleAndToolBarOnMac(true);

	renderTimerId_ = startTimer(timerInterval60Fps);
}

void MainWindow::saveDockManagerCustomLayouts() {
	auto settings = PathManager::layoutSettings();
	dockManager_->saveCustomLayouts(settings);
	settings.sync();
	if (settings.status() != QSettings::NoError) {
		LOG_ERROR(raco::log_system::COMMON, "Saving custom layout failed: {}", raco::core::PathManager::recentFilesStorePath().string());
		QMessageBox::critical(this, "Saving custom layout failed", QString("Custom layout data could not be saved to disk and will be lost after closing Ramses Composer. Check whether the application can write to its config directory.\nFile: ") 
			+ QString::fromStdString(PathManager::layoutFilePath().string()));
	}
}

void MainWindow::timerEvent(QTimerEvent* event) {
	auto startLoop = std::chrono::high_resolution_clock::now();
	racoApplication_->doOneLoop();

	const auto& viewport = racoApplication_->activeRaCoProject().project()->settings()->viewport_;
	const auto& backgroundColor = *racoApplication_->activeRaCoProject().project()->settings()->backgroundColor_;

	Q_EMIT viewportChanged({*viewport->i1_, *viewport->i2_});

	for (auto preview : findChildren<raco::ramses_widgets::PreviewMainWindow*>()) {
		preview->commit(racoApplication_->rendererDirty_);
		preview->displayScene(racoApplication_->sceneBackendImpl()->currentSceneId(), backgroundColor);
	}
	racoApplication_->rendererDirty_ = false;
	auto logicEngineExecutionEnd = std::chrono::high_resolution_clock::now();
	timingsModel_.addLogicEngineTotalExecutionDuration(std::chrono::duration_cast<std::chrono::microseconds>(logicEngineExecutionEnd - startLoop).count());
	racoApplication_->sceneBackendImpl()->flush();

	rendererBackend_->doOneLoop();
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (resolveDirtiness()) {
		killTimer(renderTimerId_);
		auto settings = raco::core::PathManager::layoutSettings();
		settings.setValue("geometry", saveGeometry());
		settings.setValue("windowState", saveState());
		dockManager_->saveCurrentLayoutInCache(settings);
		
		settings.sync();		
		if (settings.status() != QSettings::NoError) {
			LOG_WARNING(raco::log_system::COMMON, "Saving layout failed: {}", raco::core::PathManager::recentFilesStorePath().string());
		}
		QMainWindow::closeEvent(event);
		event->accept();
	} else {
		event->ignore();
	}
}

void MainWindow::restoreSettings() {
	if (PathManager::layoutFilePath().existsFile()) {
		auto settings = raco::core::PathManager::layoutSettings();
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState").toByteArray());
	}
}

void MainWindow::openProject(const QString& file, int featureLevel) {
	auto fileString = file.toStdString();
	if (!fileString.empty() && (!raco::utils::u8path(fileString).exists() || !raco::utils::u8path(fileString).userHasReadAccess())) {
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
		auto settings = raco::core::PathManager::layoutSettings();
		dockManager_->saveCurrentLayoutInCache(settings);
		settings.sync();
		if (settings.status() != QSettings::NoError) {
			LOG_WARNING(raco::log_system::COMMON, "Saving layout failed: {}", raco::core::PathManager::recentFilesStorePath().string());
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
				auto projectDirectory = raco::utils::u8path(projectPath).normalized().parent_path().string();
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
				racoApplication_->switchActiveRaCoProject(file, relinkCallback, true, loadFeatureLevel);
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

	} catch (const raco::application::FutureFileVersion& error) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file was created with newer version of {app_name}. Please upgrade.\n\nExpected File Version: {expected_file_version}\nFound File Version: {file_version}", fmt::arg("app_name", "Ramses Composer"), fmt::arg("expected_file_version", raco::serialization::RAMSES_PROJECT_FILE_VERSION), fmt::arg("file_version", error.fileVersion_)).c_str(), QMessageBox::Close);
	} catch (const ExtrefError& error) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("External reference update failed.\n\n{}", error.what()).c_str(), QMessageBox::Close);
	} catch (const std::exception& e) {
		racoApplication_->switchActiveRaCoProject({}, {});
		QMessageBox::warning(this, "File Load Error", fmt::format("Project file {} could not be loaded.\n\nReported error: {}\n\nCheck whether the file has been broken or corrupted.", fileString, e.what()).c_str(), QMessageBox::Close);
	}

	renderTimerId_ = startTimer(timerInterval60Fps);

	// Recreate our layout with new context
	dockManager_ = createDockManager(this);
	restoreCachedLayout();
	configureDebugActions(ui, this, racoApplication_->activeRaCoProject().commandInterface());

	updateApplicationTitle();
	updateActiveProjectConnection();
	updateProjectSavedConnection();
	updateUpgradeMenu();
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
	return false;
}

bool MainWindow::saveActiveProject() {
	if (racoApplication_->canSaveActiveProject()) {
		if (racoApplication_->activeProjectPath().empty()) {
			return saveAsActiveProject();
		} else {
			std::string errorMsg;
			if (racoApplication_->activeRaCoProject().save(errorMsg)) {
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

bool MainWindow::saveAsActiveProject() {
	if (racoApplication_->canSaveActiveProject()) {
		bool setProjectName = racoApplication_->activeProjectPath().empty();
		auto newPath = QFileDialog::getSaveFileName(this, "Save As...", QString::fromStdString(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project).string()), "Ramses Composer Assembly (*.rca)");
		if (newPath.isEmpty()) {
			return false;
		}
		if (!newPath.endsWith(".rca")) newPath += ".rca";
		std::string errorMsg;
		if (racoApplication_->activeRaCoProject().saveAs(newPath, errorMsg, setProjectName)) {
			updateActiveProjectConnection();
			updateProjectSavedConnection();
			return true;
		} else {
			updateApplicationTitle();
			QMessageBox::critical(this, "Save Error", fmt::format("Can not save project: Writing the project file '{}' failed with error '{}'", racoApplication_->activeProjectPath(), errorMsg).c_str(), QMessageBox::Ok);
		}
	} else {
		QMessageBox::warning(this, "Save Error", fmt::format("Can not save project: externally referenced projects not clean.").c_str(), QMessageBox::Ok);
	}
	return false;
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
	auto maxFeatureLevel = static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel);
	auto currentFeatureLevel = racoApplication_->activeRaCoProject().project()->featureLevel();
	if (currentFeatureLevel < maxFeatureLevel) {
		upgradeMenu_->clear();
		for (int fl = currentFeatureLevel + 1; fl <= maxFeatureLevel; fl++) {
			upgradeMenu_->addAction(QString::fromStdString(fmt::format("&{}. Feature Level {}", fl, fl)), [fl, this]() {
				upgradeActiveProject(fl);
			});
		}
		upgradeMenu_->setDisabled(false);	
	} else {
		upgradeMenu_->setDisabled(true);	
	}
}

const std::vector<std::wstring>& MainWindow::pythonSearchPaths() const {
	return pythonSearchPaths_;
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
		createInitialWidgets(this, *rendererBackend_, racoApplication_, dockManager_, treeDockManager_);

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
			createAndAddProjectSettings(this, dockNameCString, dockManager_, &racoApplication_->activeRaCoProject(), racoApplication_->dataChangeDispatcher(), racoApplication_->activeRaCoProject().commandInterface(), racoApplication_->sceneBackendImpl());
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
			createAndAddErrorView(this, racoApplication_, dockNameCString, dockManager_, treeDockManager_, logViewModel_);
		} else if (savedDockType == DockWidgetTypes::LOG_VIEW) {
			createAndAddLogView(this, racoApplication_, dockNameCString, dockManager_, treeDockManager_, logViewModel_);
		} else if (savedDockType == DockWidgetTypes::TRACE_PLAYER) {
			createAndAddTracePlayer(this, dockManager_, &racoApplication_->activeRaCoProject().tracePlayer());
		} else if (savedDockType == DockWidgetTypes::PYTHON_RUNNER) {
			createAndAddPythonRunner(this, racoApplication_, dockNameCString, pythonScriptCache_, pythonScriptArgumentCache_, dockManager_);
		} else {
			LOG_DEBUG(raco::log_system::COMMON, "Ignoring unknown dock type '{}'.", savedDockType.toStdString());
		}
	}
}

void MainWindow::resetDockManager() {
	delete dockManager_;
	dockManager_ = createDockManager(this);
}

void MainWindow::updateActiveProjectConnection() {
	QObject::disconnect(activeProjectFileConnection_);
	if (!racoApplication_->activeProjectPath().empty()) {
		activeProjectFileConnection_ = QObject::connect(&racoApplication_->activeRaCoProject(), &raco::application::RaCoProject::activeProjectFileChanged, [this]() {
			updateApplicationTitle();
		});
	}
}

void MainWindow::updateProjectSavedConnection() {
	QObject::disconnect(projectSavedConnection_);
	projectSavedConnection_ = QObject::connect(&racoApplication_->activeRaCoProject(), &raco::application::RaCoProject::projectSuccessfullySaved, [this]() {
		recentFileMenu_->addRecentFile(racoApplication_->activeProjectPath().c_str());
		updateApplicationTitle();
	});
}

void MainWindow::focusToObject(const QString& objectID) {
	if (treeDockManager_.getTreeDockAmount() != 0 && treeDockManager_.docksContainObject(objectID)) {
		Q_EMIT objectFocusRequestedForTreeDock(objectID);
	} else {
		Q_EMIT objectFocusRequestedForPropertyBrowser(objectID);
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

// As long as we can't cleverly create multiple previews on the same scene, the "New Preview" menu item should only be enabled to create one Ramses preview.
void MainWindow::setNewPreviewMenuEntryEnabled(bool enabled) {
	ui->actionNewPreview->setEnabled(enabled);
}
