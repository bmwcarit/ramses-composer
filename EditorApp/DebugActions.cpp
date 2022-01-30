/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "DebugActions.h"

#include "common_widgets/DebugLayout.h"
#include "core/Context.h"
#include "core/CommandInterface.h"
#include "core/PathManager.h"
#include "components/Naming.h"
#include "components/RaCoPreferences.h"
#include "ui_mainwindow.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/Texture.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include <QDesktopServices>
#include <QUrl>

QMetaObject::Connection openLogFileConnection;
QMetaObject::Connection actionDumpObjectTree;

void configureDebugActions(Ui::MainWindow* ui, QWidget* widget, raco::core::CommandInterface* commandInterface) {
	using raco::components::Naming;
	using namespace raco::user_types;

	// Debug actions
	if (openLogFileConnection) QObject::disconnect(openLogFileConnection);
	openLogFileConnection = QObject::connect(ui->actionOpenLogFileDirectory, &QAction::triggered, []() { QDesktopServices::openUrl(QUrl::fromLocalFile(raco::core::PathManager::logFileDirectory().string().c_str())); });
	if (actionDumpObjectTree) QObject::disconnect(actionDumpObjectTree);
	actionDumpObjectTree = QObject::connect(ui->actionDumpObjectTree, &QAction::triggered, [widget]() { raco::debug::dumpLayoutInfo(widget); });
}
