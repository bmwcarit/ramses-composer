/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "RaCoDockManager.h"
#include "common_widgets/TimingsWidget.h"
#include "common_widgets/log_model/LogViewModel.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "ramses_widgets/RendererBackend.h"

#include <QListWidget>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace ads {
class CDockWidget;
}  // namespace ads

namespace raco::application {
	class RaCoApplication;
}
class OpenRecentMenu;

class MainWindow : public QMainWindow {
	Q_OBJECT

	using ExtraLayoutInfo = QMap<QString, QVector<QPair<QString, QString>>>;

public:
	struct DockWidgetTypes {
		static inline const char* PREFABS{"Prefabs"};
		static inline const char* PROJECT_BROWSER{"Project Browser"};
		static inline const char* PROJECT_SETTINGS{"Project Settings"};
		static inline const char* PROPERTY_BROWSER{"Property Browser"};
		static inline const char* RAMSES_PREVIEW{"Ramses Preview"};
		static inline const char* RESOURCES{"Resources"};
		static inline const char* SCENE_GRAPH{"Scene Graph"};
		static inline const char* UNDO_STACK{"Undo Stack"};
		static inline const char* ERROR_VIEW{"Error View"};
		static inline const char* LOG_VIEW{"Log View"};
		static inline const char* PYTHON_RUNNER{"Python Runner"};
		static inline const char* TRACE_PLAYER{"Trace Player"};
	};

	explicit MainWindow(
		raco::application::RaCoApplication* racoApplication,
		raco::ramses_widgets::RendererBackend* rendererBackend,
		const std::vector<std::wstring>& pythonSearchPaths,
		QWidget* parent = nullptr);
	~MainWindow();

	void setNewPreviewMenuEntryEnabled(bool enabled);
	void updateApplicationTitle();
	void updateSavedLayoutMenu();
	void updateUpgradeMenu();

	const std::vector<std::wstring>& pythonSearchPaths() const;

public Q_SLOTS:
	void showMeshImportErrorMessage(const std::string& filePath, const std::string& meshError);
	void focusToObject(const QString& objectID);

protected:
	void timerEvent(QTimerEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void restoreSettings();
	/** @returns if user canceled the dirty resolution */
	bool resolveDirtiness();
	QString getActiveProjectFolder();
	void restoreCachedLayout();
	void restoreCustomLayout(const QString& layoutName);
	void regenerateLayoutDocks(const RaCoDockManager::LayoutDocks& docks);
	void saveDockManagerCustomLayouts();
	bool isUpgradePrevented();

protected Q_SLOTS:
	void openProject(const QString& file = {}, int featureLevel = -1, bool generateNewObjectIDs = false);
	bool saveActiveProject();
	bool upgradeActiveProject(int newFeatureLevel);
	bool saveAsActiveProject(bool newID = false);
	bool saveAsActiveProjectWithNewID();
	void resetDockManager();
	void updateActiveProjectConnection();
	void updateProjectSavedConnection();

Q_SIGNALS:
	void viewportChanged(const QSize& sceneSize);
	void objectFocusRequestedForPropertyBrowser(const QString& objectID);
	void objectFocusRequestedForTreeDock(const QString& objectID);

private:
	Ui::MainWindow* ui;
	OpenRecentMenu* recentFileMenu_;
	RaCoDockManager* dockManager_;
	QListWidget* sceneObjectList_;
	const std::vector<std::wstring> pythonSearchPaths_;

	raco::ramses_widgets::RendererBackend* rendererBackend_;
	raco::application::RaCoApplication* racoApplication_;
	raco::object_tree::view::ObjectTreeDockManager treeDockManager_;
	raco::common_widgets::TimingsModel timingsModel_{this};
	QMetaObject::Connection activeProjectFileConnection_;
	QMetaObject::Connection projectSavedConnection_;
	raco::common_widgets::LogViewModel* logViewModel_;
	std::map<QString, qint64> pythonScriptCache_;
	std::map<QString, qint64> pythonScriptArgumentCache_;

	int renderTimerId_ = 0;

	QFileInfo getDragAndDropFileInfo(const QDropEvent* event);
};
