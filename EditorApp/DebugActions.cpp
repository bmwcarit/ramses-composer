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
QMetaObject::Connection actionCreateDummyScene;

void configureDebugActions(Ui::MainWindow* ui, QWidget* widget, raco::core::CommandInterface* commandInterface) {
	using raco::components::Naming;
	using namespace raco::user_types;

	// Debug actions
	if (openLogFileConnection) QObject::disconnect(openLogFileConnection);
	openLogFileConnection = QObject::connect(ui->actionOpenLogFile, &QAction::triggered, []() { QDesktopServices::openUrl(QUrl{raco::core::PathManager::logFilePath().c_str(), QUrl::TolerantMode}); });
	if (actionDumpObjectTree) QObject::disconnect(actionDumpObjectTree);
	actionDumpObjectTree = QObject::connect(ui->actionDumpObjectTree, &QAction::triggered, [widget]() { raco::debug::dumpLayoutInfo(widget); });
	if (actionCreateDummyScene) QObject::disconnect(actionCreateDummyScene);
	actionCreateDummyScene = QObject::connect(ui->actionCreateDummyScene, &QAction::triggered, [commandInterface]() {
		const auto& prefs = raco::components::RaCoPreferences::instance();

		auto mesh = commandInterface->createObject(raco::user_types::Mesh::typeDescription.typeName, Naming::format("DuckMesh"));
		commandInterface->set(raco::core::ValueHandle{mesh, &Mesh::bakeMeshes_}, true);
		commandInterface->set(raco::core::ValueHandle{mesh, &Mesh::uri_}, 
			(raco::core::PathManager::defaultResourceDirectory() / prefs.meshSubdirectory.toStdString() / "Duck.glb").generic_string());
		auto material = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, Naming::format("DuckMaterial"));
		commandInterface->set(raco::core::ValueHandle{material, &Material::uriVertex_},
			(raco::core::PathManager::defaultResourceDirectory() / prefs.shaderSubdirectory.toStdString() / "simple_texture.vert").generic_string());
		commandInterface->set(raco::core::ValueHandle{material, &Material::uriFragment_}, 
			(raco::core::PathManager::defaultResourceDirectory() / prefs.shaderSubdirectory.toStdString() / "simple_texture.frag").generic_string());
		auto texture = commandInterface->createObject(raco::user_types::Texture::typeDescription.typeName, Naming::format("DuckTexture"));
	commandInterface->set(raco::core::ValueHandle{texture, &Texture::uri_},
			(raco::core::PathManager::defaultResourceDirectory() / prefs.imageSubdirectory.toStdString() / "DuckCM.png").generic_string());

		auto node = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, Naming::format("DuckNode"));
		auto meshNode = commandInterface->createObject(raco::user_types::MeshNode::typeDescription.typeName, Naming::format("DuckMeshNode"));
		commandInterface->moveScenegraphChildren({meshNode}, node);

		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::mesh_}, mesh);
		commandInterface->set(raco::core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);

		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::translation_, &Vec3f::y}, -1.7);
		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::rotation_, &Vec3f::y}, -160.0);
		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::scale_, &Vec3f::x}, 2.0);
		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::scale_, &Vec3f::y}, 2.0);
		commandInterface->set(raco::core::ValueHandle{meshNode, &MeshNode::scale_, &Vec3f::z}, 2.0);

		commandInterface->set(raco::core::ValueHandle{material, {"uniforms", "u_Tex"}}, texture);
	});
}
