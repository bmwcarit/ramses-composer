/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/MeshNode.h"

#include "user_types/Mesh.h"
#include "user_types/EngineTypeAnnotation.h"

#include "core/Context.h"
#include "core/CoreFormatter.h"
#include "core/EngineInterface.h"
#include "core/Errors.h"
#include "core/Project.h"
#include "user_types/UserObjectFactory.h"

#include <memory>

namespace raco::user_types {

std::vector<std::string> MeshNode::getMaterialNames() {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(*mesh_);
	if (mesh) {
		return mesh->materialNames();
	}
	return std::vector<std::string>();
}

void MeshNode::createMaterialSlot(BaseContext& context, std::string const& name) {
	// TODO ideally we should replace this by a struct property
	auto container = std::make_unique<Value<Table>>();
	(*container)->addProperty("material", new Value<SMaterial>());
	(*container)->addProperty("private", UserObjectFactory::staticCreateProperty<bool, DisplayNameAnnotation>({false}, {"Private Material"}));
	(*container)->addProperty("options", UserObjectFactory::staticCreateProperty<BlendOptions, DisplayNameAnnotation>({}, {"Options"}));
	(*container)->addProperty("uniforms", PrimitiveType::Table);
	context.addProperty({shared_from_this(), &MeshNode::materials_}, name, std::move(container));
}

void MeshNode::updateMaterialSlots(BaseContext& context, std::vector<std::string> const& materialNames) {
	for (auto matName : materialNames) {
		if (!materials_->hasProperty(matName)) {
			createMaterialSlot(context, matName);
		}
	}
	std::vector<std::string> toRemove;
	for (size_t i = 0; i < materials_->size(); i++) {
		if (std::find(materialNames.begin(), materialNames.end(), materials_->name(i)) == materialNames.end()) {
			toRemove.emplace_back(materials_->name(i));
		}
	}
	for (auto name : toRemove) {
		ValueHandle matHandle{shared_from_this(), &MeshNode::materials_};
		context.removeProperty(matHandle, materials_.asTable().index(name));
	}
}


size_t MeshNode::numMaterialSlots() {
	return materials_->size();
}

std::string MeshNode::materialName(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return materials_->name(materialSlot);
	}
	return {};
}

SMaterial MeshNode::getMaterial(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return std::dynamic_pointer_cast<Material>(materials_->get(materialSlot)->asTable().get("material")->asRef());
	}
	return nullptr;
}

bool MeshNode::materialPrivate(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return materials_->get(materialSlot)->asTable().get("private")->asBool();
	}
	return false;
}

Table* MeshNode::getUniformContainer(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return &materials_->get(materialSlot)->asTable().get("uniforms")->asTable();
	}
	return nullptr;
}

BlendOptions* MeshNode::getOptions(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return dynamic_cast<BlendOptions*>(&materials_->get(materialSlot)->asTable().get("options")->asStruct());
	}
	return nullptr;
};

ValueHandle MeshNode::getMaterialHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), &MeshNode::materials_)[materialSlot].get("material");
	}
	return ValueHandle();
}

ValueHandle MeshNode::getMaterialPrivateHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), &MeshNode::materials_)[materialSlot].get("private");
	}
	return ValueHandle();
}

ValueHandle MeshNode::getUniformContainerHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), &MeshNode::materials_)[materialSlot].get("uniforms");
	}
	return ValueHandle();
}

ValueHandle MeshNode::getMaterialOptionsHandle(size_t materialSlot) {
	if (materialSlot < materials_->size()) {
		return ValueHandle(shared_from_this(), &MeshNode::materials_)[materialSlot].get("options");
	}
	return ValueHandle();
}

PropertyInterface MeshNode::makeInterfaceFromProperty(std::string_view name, const ValueBase* property) {
	auto engineType = property->query<EngineTypeAnnotation>()->type();
	if (engineType == EnginePrimitive::Array || engineType == EnginePrimitive::Struct) {
		PropertyInterface container(name, engineType);
		for (size_t index = 0; index < property->asTable().size(); index++) {
			container.children.emplace_back(makeInterfaceFromProperty(property->asTable().name(index), property->asTable().get(index)));
		}
		return container;
	}
	return PropertyInterface(name, engineType);
}

void MeshNode::updateUniformContainer(BaseContext& context, const std::string& materialName, const Table* src, ValueHandle& destUniforms) {
	core::PropertyInterfaceList uniformDescription;
	if (src) {
		for (size_t i = 0; i < src->size(); i++) {
			uniformDescription.emplace_back(makeInterfaceFromProperty(src->name(i), src->get(i)));
		}
	}

	OutdatedPropertiesStore& cache = cachedUniformValues_[materialName];
	auto lookup = [src, &cache](const std::string& fullPropPath, core::EnginePrimitive engineType) -> const ValueBase* {
		auto it = cache.find(std::make_pair(fullPropPath, engineType));
		if (it != cache.end()) {
			return it->second.get();
		} 
		// Copy value from material if not found in cache.
		// To get the property name we need to remove the leading '/' from the property path.
		if (src->hasProperty(fullPropPath.substr(1))) {
			return src->get(fullPropPath.substr(1));
		}
		return nullptr;
	};

	syncTableWithEngineInterface(context, uniformDescription, destUniforms, cache, false, true, lookup);
}

void MeshNode::checkMeshMaterialAttributMatch(BaseContext& context) {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(*mesh_);
	SMaterial material = getMaterial(0);

	std::string errors;
	if (mesh && material && mesh->meshData()) {
		for (const auto& attrib : material->attributes()) {
			std::string name = attrib.name;

			static const std::unordered_map<core::MeshData::VertexAttribDataType, EnginePrimitive> meshAttribTypeMap = {
				{core::MeshData::VertexAttribDataType::VAT_Float, EnginePrimitive::Double},
				{core::MeshData::VertexAttribDataType::VAT_Float2, EnginePrimitive::Vec2f},
				{core::MeshData::VertexAttribDataType::VAT_Float3, EnginePrimitive::Vec3f},
				{core::MeshData::VertexAttribDataType::VAT_Float4, EnginePrimitive::Vec4f}};

			int index = mesh->meshData()->attribIndex(name);
			if (index != -1) {
				auto meshAttribType = meshAttribTypeMap.at(mesh->meshData()->attribDataType(index));
				if (attrib.type != meshAttribType) {
					// types don't match
					errors += fmt::format("Attribute '{}' type mismatch: Material '{}' requires type '{}' but Mesh '{}' provides type '{}'.\n",
						name, material->objectName(), attrib.type, mesh->objectName(), meshAttribType);
				}
			} else {
				// attribute not found by name in mesh attributes
				errors += fmt::format("Attribute '{}' required by Material '{}' not found in Mesh '{}'.\n", name, material->objectName(), mesh->objectName());
			}
		}
		if (!errors.empty()) {
			errors = "Attribute mismatch:\n\n" + errors.substr(0, errors.length() - 1);
		}
	}

	context.updateBrokenLinkErrors(shared_from_this());
	if (!errors.empty()) {
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, ValueHandle{shared_from_this(), &MeshNode::mesh_}, errors);
	} else {
		context.errors().removeError(ValueHandle{shared_from_this(), &MeshNode::mesh_});
	}
}

void MeshNode::onAfterContextActivated(BaseContext& context) {
	// This handlers is needed to cover a corner case during paste.
	// Normally BaseContext::performExternalFileReload will be called during paste and will in turn 
	// call onAfterReferencedObjectChanged. In these cases the additional onAfterContextActivated handler
	// will cause duplicate work.
	// In case the mesh and material references are lost during paste however the onAfterReferencedObjectChanged handler
	// will not be called and we need this onAfterContextActivated handler to update the dynamic properties.
	auto matnames = getMaterialNames();
	updateMaterialSlots(context, matnames);

	bool changed = false;
	for (size_t matIndex = 0; matIndex < numMaterialSlots(); matIndex++) {
		ValueHandle uniformsHandle = getUniformContainerHandle(matIndex);
		auto material = getMaterial(matIndex);
		Table* materialUniforms = nullptr;
		if (material && materialPrivate(matIndex)) {
			materialUniforms = &*material->uniforms_;
		}
		updateUniformContainer(context, materialName(matIndex), materialUniforms, uniformsHandle);
		changed = true;
	}

	if (changed) {
		checkMeshMaterialAttributMatch(context);
	} else {
		context.updateBrokenLinkErrors(shared_from_this());
	}
}

void MeshNode::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	SMesh mesh = std::dynamic_pointer_cast<Mesh>(changedObject.rootObject());
	if (mesh) {
		std::vector<std::string> matnames = mesh->materialNames();
		updateMaterialSlots(context, matnames);
		checkMeshMaterialAttributMatch(context);
	}

	SMaterial material = std::dynamic_pointer_cast<Material>(changedObject.rootObject());
	if (material) {
		bool changed = false;
		for (size_t matIndex = 0; matIndex < numMaterialSlots(); matIndex++) {
			if (material == getMaterial(matIndex)) {
				ValueHandle uniformsHandle = getUniformContainerHandle(matIndex);
				Table* materialUniforms = nullptr;
				if (materialPrivate(matIndex)) {
					materialUniforms = &*material->uniforms_;
				}
				updateUniformContainer(context, materialName(matIndex), materialUniforms, uniformsHandle);
				changed = true;
			}
		}
		if (changed) {
			checkMeshMaterialAttributMatch(context);
		}
	}
}

void MeshNode::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&MeshNode::mesh_)) {
		auto matnames = getMaterialNames();
		updateMaterialSlots(context, matnames);
		checkMeshMaterialAttributMatch(context);
	}
	ValueHandle materialsHandle(shared_from_this(), &MeshNode::materials_);
	if (materialsHandle.contains(value) && value.depth() == 3 && value.getPropName() == "material") {
		std::string materialName = value.parent().getPropName();
		ValueHandle uniformsHandle = value.parent().get("uniforms");
		const Table& uniforms = uniformsHandle.constValueRef()->asTable();
		bool privateMaterial = value.parent().get("private").asBool();

		SMaterial material = value.asTypedRef<Material>();
		Table* materialUniforms = nullptr;
		if (material && privateMaterial) {
			materialUniforms = &*material->uniforms_;
		}
		updateUniformContainer(context, materialName, materialUniforms, uniformsHandle);
		checkMeshMaterialAttributMatch(context);
	}

	if (materialsHandle.contains(value) && value.depth() == 3 && value.getPropName() == "private") {
		std::string materialName = value.parent().getPropName();
		ValueHandle uniformsHandle = value.parent().get("uniforms");
		const Table& uniforms = uniformsHandle.constValueRef()->asTable();

		SMaterial material = value.parent().get("material").asTypedRef<Material>();
		Table* materialUniforms = nullptr;
		if (material && value.asBool()) {
			materialUniforms = &*material->uniforms_;

			auto optionsHandle = value.parent().get("options");
			context.set(optionsHandle, *material->options_);
		}

		updateUniformContainer(context, materialName, materialUniforms, uniformsHandle);
		// Synthetic change record: needed since toggling the 'private' flag will change the hidden status
		// of the options; see Queries::isHidden
		context.changeMultiplexer().recordValueChanged(value.parent().get("options"));
	}

	if (value.isRefToProp(&MeshNode::objectName_)) {
		context.updateBrokenLinkErrors(shared_from_this());
	}
}

}  // namespace raco::user_types