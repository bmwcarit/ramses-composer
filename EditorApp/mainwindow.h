/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "RaCoDockManager.h"
#include "common_widgets/TimingsWidget.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "ramses_widgets/RendererBackend.h"
#include "common_widgets/log_model/LogViewModel.h"
#include "node_logic/NodeLogic.h"
#include "curve/CurveNameWidget.h"
#include "material_logic/materalLogic.h"
#include "data_Convert/ProgramManager.h"
#include "curve/CurveLogic.h"

#include <QListWidget>
#include <QMainWindow>
#include <QTextEdit>

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
		static inline const char* ANIMATION_VIEW{"animation View"};
		static inline const char* CURVE_VIEW{"Curve View"};
		static inline const char* PROPERTY_VIEW{"property View"};
		static inline const char* TIME_AXIS_VIEW{"Time Axis View"};
	};

	explicit MainWindow(
		raco::application::RaCoApplication* racoApplication,
		raco::ramses_widgets::RendererBackend* rendererBackend,
		QWidget* parent = nullptr);
	~MainWindow();

    void initLogic();
	void setNewPreviewMenuEntryEnabled(bool enabled);
	void updateApplicationTitle();
	void updateSavedLayoutMenu();

public Q_SLOTS:
	void showMeshImportErrorMessage(const std::string& filePath, const std::string& meshError);
	void slotCreateCurveAndBinding(QString property, QString curve, QVariant value);
	void slotCreateCurve(QString property, QString curve, QVariant value);
    void setResourceHandles(const std::map<std::string, raco::core::ValueHandle> &map);
    void updateNodeHandles(const QString &title, const std::map<std::string, raco::core::ValueHandle> &map);
protected:
	void timerEvent(QTimerEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void restoreSettings();
	/** @returns if user canceled the dirty resolution */
	bool resolveDirtiness();
	QString getActiveProjectFolder();
	void restoreCachedLayout();
	void restoreCustomLayout(const QString& layoutName);
	void regenerateLayoutDocks(const RaCoDockManager::LayoutDocks& docks);
	void saveDockManagerCustomLayouts();

protected Q_SLOTS:
	void openProject(const QString& file = {});
	bool saveActiveProject();
	bool saveAsActiveProject();
	void importScene();
	void resetDockManager();
	void updateActiveProjectConnection();

Q_SIGNALS:
    void getResourceHandles();
	void viewportChanged(const QSize& sceneSize);

    void axesChanged(const bool& z_up);
    void displayGridChanged(const bool& enable);
    void sceneUpdated(const bool& z_up);
private:
	Ui::MainWindow* ui;
	OpenRecentMenu* recentFileMenu_;
	RaCoDockManager* dockManager_;
	QListWidget* sceneObjectList_;
	raco::ramses_widgets::RendererBackend* rendererBackend_;
	raco::application::RaCoApplication* racoApplication_;
	raco::object_tree::view::ObjectTreeDockManager treeDockManager_;
	raco::common_widgets::TimingsModel timingsModel_{this};
    raco::dataConvert::ProgramManager programManager_;
	QMetaObject::Connection activeProjectFileConnection_;
	raco::common_widgets::LogViewModel* logViewModel_;
	raco::node_logic::NodeLogic* nodeLogic_{nullptr};
    CurveLogic *curveLogic_{nullptr};
	CurveNameWidget* curveNameWidget_{nullptr};
	raco::material_logic::MateralLogic* materialLogic_{nullptr};

	int renderTimerId_ = 0;
};
