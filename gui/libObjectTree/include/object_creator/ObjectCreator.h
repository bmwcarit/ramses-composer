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

#include "core/CommandInterface.h"
#include "core/Errors.h"

#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/Texture.h"

#include <QFileInfo>

namespace raco::object_tree::object_creator {

class ObjectCreator {
public:
	ObjectCreator(core::CommandInterface* commandInterface,
		bool acceptLuaModules,
		bool acceptLuaInterfaces,
		bool acceptLuaScripts) :
	commandInterface_(commandInterface),
	acceptLuaModules_(acceptLuaModules),
	acceptLuaInterfaces_(acceptLuaInterfaces),
	acceptLuaScripts_(acceptLuaScripts) {}

	core::SEditorObject createNewObjectFromFile(const QFileInfo& fileInfo);
	bool containsRunFunction(const QString& absPath) const;

private:
	core::CommandInterface* commandInterface_;
	bool acceptLuaModules_ = false;
	bool acceptLuaInterfaces_ = false;
	bool acceptLuaScripts_ = false;

	std::map<std::string, std::pair<std::string, std::string>> extensionToObject{
		{"gltf", {user_types::Mesh::typeDescription.typeName, "uri"}},
		{"glb", {user_types::Mesh::typeDescription.typeName, "uri"}},
		{"ctm", {user_types::Mesh::typeDescription.typeName, "uri"}},
		{"png", {user_types::Texture::typeDescription.typeName, "uri"}},
		{"vert", {user_types::Material::typeDescription.typeName, "uriVertex"}},
		{"frag", {user_types::Material::typeDescription.typeName, "uriFragment"}},
		{"geom", {user_types::Material::typeDescription.typeName, "uriGeometry"}},
		{"def", {user_types::Material::typeDescription.typeName, "uriDefines"}},
		{"vert.glsl", {user_types::Material::typeDescription.typeName, "uriVertex"}},
		{"frag.glsl", {user_types::Material::typeDescription.typeName, "uriFragment"}},
		{"geom.glsl", {user_types::Material::typeDescription.typeName, "uriGeometry"}}};
};

}
