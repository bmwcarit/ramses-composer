/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Mesh.h"

#include "core/Context.h"
#include "core/Errors.h"
#include "core/MeshCacheInterface.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "Validation.h"

namespace raco::user_types {

void Mesh::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	uriListener_.reset();
}

void Mesh::onAfterContextActivated(BaseContext& context) {
	auto uriAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Mesh::uri_});
	uriListener_ = context.meshCache()->registerFileChangedHandler(uriAbsPath, {&context, shared_from_this(),
			[this, &context]() { updateMesh(context); }});
	updateMesh(context);
}

void Mesh::updateMesh(BaseContext& context) {
	context.errors().removeError(ValueHandle{shared_from_this()});

	MeshDescriptor desc;
	desc.absPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Mesh::uri_});
	desc.bakeAllSubmeshes = bakeMeshes_.asBool();
	desc.submeshIndex = meshIndex_.asInt();

	if (validateURI(context, {shared_from_this(), &Mesh::uri_})) {
		mesh_ = context.meshCache()->loadMesh(desc);
		if (!mesh_) {
			auto savedErrorString = context.meshCache()->getMeshError(desc.absPath);
			auto errorMessage = (savedErrorString.empty()) ? "Invalid mesh file." : "Error while importing mesh: " + savedErrorString;
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, {shared_from_this()}, errorMessage);
		} else {
			auto savedWarningString = context.meshCache()->getMeshWarning(desc.absPath);
			if (!savedWarningString.empty()) {
				context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::WARNING, {shared_from_this()}, savedWarningString);
			}
		}
	} else {
		mesh_.reset();
	}

	if (mesh_) {
		std::string infoText;
		auto selectedMesh = mesh_.get();

		static std::map<raco::core::MeshData::VertexAttribDataType, std::string> formatDescription = {
			{raco::core::MeshData::VertexAttribDataType::VAT_Float, "float"},
			{raco::core::MeshData::VertexAttribDataType::VAT_Float2, "vec2"},
			{raco::core::MeshData::VertexAttribDataType::VAT_Float3, "vec3"},
			{raco::core::MeshData::VertexAttribDataType::VAT_Float4, "vec4"},
		};

		infoText += "Mesh information\n\n";

		infoText += fmt::format("Triangles: {}\n", selectedMesh->numTriangles());
		infoText += fmt::format("Vertices: {}\n", selectedMesh->numVertices());
		//infoText += fmt::format("Submeshes: {}\n", selectedMesh->numSubmeshes());
		infoText += fmt::format("Total Asset File Meshes: {}\n", context.meshCache()->getTotalMeshCount(desc.absPath));
		infoText += "\nAttributes:";

		for (uint32_t i{0}; i < selectedMesh->numAttributes(); i++) {
			infoText += fmt::format("\nin {} {};", formatDescription[selectedMesh->attribDataType(i)], selectedMesh->attribName(i));
		}
		if (!infoText.empty()) {
			context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::INFORMATION, ValueHandle{shared_from_this()}, infoText);
		}

		ValueHandle matnames_handle{shared_from_this(), &Mesh::materialNames_};
		context.set(matnames_handle, std::vector<std::string>{"material"});
	}

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

std::vector<std::string> Mesh::materialNames() {
	return materialNames_->asVector<std::string>();
}

void Mesh::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&Mesh::uri_)) {
		auto uriAbsPath = PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &Mesh::uri_});
		uriListener_ = context.meshCache()->registerFileChangedHandler(uriAbsPath, {&context, shared_from_this(),
			[this, &context]() { updateMesh(context); }});
		updateMesh(context);
	} else if (value.isRefToProp(&Mesh::bakeMeshes_) || !bakeMeshes_.asBool() && value.isRefToProp(&Mesh::meshIndex_)) {
		context.changeMultiplexer().recordPreviewDirty(shared_from_this());
		updateMesh(context);
	}
}

}  // namespace raco::user_types