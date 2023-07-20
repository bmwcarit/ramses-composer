/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_creator/ObjectCreator.h"

#include "core/EditorObject.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScriptModule.h"

#include <QFileInfo>

namespace raco::object_tree::object_creator {

raco::core::SEditorObject ObjectCreator::createNewObjectFromFile(const QFileInfo& fileInfo) {
	const auto absolutePath = fileInfo.absoluteFilePath();
	const auto relativePath{utils::u8path(absolutePath.toStdString()).normalizedRelativePath(utils::u8path(commandInterface_->project()->currentFolder())).string()};
	const auto fileName = fileInfo.fileName();
	const auto baseName = fileInfo.baseName().toStdString();

	core::SEditorObject newObject{};
	std::vector<core::SEditorObject> nodes;
	const auto newObjectName = commandInterface_->project()->findAvailableUniqueName(nodes.begin(), nodes.end(), nullptr, baseName);
	const auto compositeCommandName = "Create " + newObjectName;

	if (fileName.endsWith(".lua")) {
		if (acceptLuaInterfaces_ || acceptLuaScripts_) {
			if (acceptLuaScripts_ && containsRunFunction(absolutePath)) {
				commandInterface_->executeCompositeCommand([this, &newObjectName, &relativePath, &newObject]() {
					newObject = commandInterface_->createObject(user_types::LuaScript::typeDescription.typeName, newObjectName);
					commandInterface_->set({newObject, &user_types::LuaScript::uri_}, relativePath);
				},
					compositeCommandName);
			} else {
				commandInterface_->executeCompositeCommand([this, &newObjectName, &relativePath, &newObject]() {
					newObject = commandInterface_->createObject(user_types::LuaInterface::typeDescription.typeName, newObjectName);
					commandInterface_->set({newObject, &user_types::LuaInterface::uri_}, relativePath);
				},
					compositeCommandName);
			}
		} else if (acceptLuaModules_) {
			commandInterface_->executeCompositeCommand([this, &newObjectName, &relativePath, &newObject]() {
				newObject = commandInterface_->createObject(user_types::LuaScriptModule::typeDescription.typeName, newObjectName);
				commandInterface_->set({newObject, &user_types::LuaScriptModule::uri_}, relativePath);
			},
				compositeCommandName);
		}
	} else {
		std::for_each(extensionToObject.begin(), extensionToObject.end(), [this, &fileName, &newObject, &newObjectName, &relativePath, &compositeCommandName](const auto& extensionParam) {
			const auto extension = extensionParam.first;
			const auto typeName = extensionParam.second.first;
			const auto propertyName = extensionParam.second.second;
			if (fileName.endsWith(QString::fromStdString(extension))) {
				commandInterface_->executeCompositeCommand([&]() {
					newObject = commandInterface_->createObject(typeName, newObjectName);
					commandInterface_->set({newObject, {propertyName}}, relativePath);
				},
					compositeCommandName);
			}
		});
	}

	return newObject;
}

bool ObjectCreator::containsRunFunction(const QString& absPath) const {
	QFile file{absPath};
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error(fmt::format("Error opening file {}", absPath.toLatin1()));
	}

	const auto byteArray = file.readAll();
	file.close();

	const QString fileContent = QString::fromUtf8(byteArray);
	const QRegularExpression regExp("(?m)^function run");
	const QRegularExpressionMatch match = regExp.match(fileContent);

	return match.hasMatch();
}

}  // namespace raco::object_tree::view
