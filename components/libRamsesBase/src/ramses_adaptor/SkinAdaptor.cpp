/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/SkinAdaptor.h"

#include "ramses_base/RamsesHandles.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"

#include <spdlog/fmt/fmt.h>

namespace raco::ramses_adaptor {

SkinAdaptor::SkinAdaptor(SceneAdaptor* sceneAdaptor, user_types::SSkin skin)
	: UserTypeObjectAdaptor(sceneAdaptor, skin),
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject(), &user_types::Skin::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject(), &user_types::Skin::targets_}, [this](auto) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject(), &user_types::Skin::joints_}, [this](auto) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject(), &user_types::Skin::skinIndex_}, [this]() { tagDirty(); })},
	  dirtySubscription_{sceneAdaptor->dispatcher()->registerOnPreviewDirty(editorObject(), [this]() { tagDirty(); })} {
}

bool SkinAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	skinBindings_.clear();

	std::vector<ramses_base::RamsesNodeBinding> jointBindings;
	const auto& jointsArray = editorObject()->joints_;
	bool emptySlots = false;
	for (auto index = 0; index < jointsArray->size(); index++) {
		auto node = **jointsArray->get(index);
		ramses_base::RamsesNodeBinding nodeBinding = lookupNodeBinding(sceneAdaptor_, node);
		if (nodeBinding) {
			if (index != jointBindings.size()) {
				emptySlots = true;
			}
			jointBindings.emplace_back(nodeBinding);
		}
	}
	if (emptySlots) {
		jointBindings.clear();
	}

	auto data = editorObject()->skinData();
	if (data) {
		if (emptySlots) {
			std::string errorMsg = fmt::format("Cannot create Skin '{}': all joints nodes must be consecutive and valid nodes", editorObject()->objectName());
			LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
			errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()}, errorMsg);
		} else {
			std::vector<MeshNodeAdaptor*> targetAdaptors;
			const auto& targetsArray = editorObject()->targets_;
			for (auto index = 0; index < targetsArray->size(); index++) {
				auto meshnode = **targetsArray->get(index);
				auto meshNodeAdaptor = sceneAdaptor_->lookup<MeshNodeAdaptor>(meshnode);
				if (meshNodeAdaptor) {
					targetAdaptors.emplace_back(meshNodeAdaptor);
				}
			}

			if (!targetAdaptors.empty()) {
				std::vector<std::string> errorMessages;
				for (auto index = 0; index < targetAdaptors.size(); index++) {
					const auto meshNodeAdaptor = targetAdaptors[index];
					if (auto appearance = meshNodeAdaptor->privateAppearance()) {
						if (auto appearanceBinding = meshNodeAdaptor->appearanceBinding()) {
							ramses::UniformInput uniform = (*appearance)->getEffect().findUniformInput(core::SkinData::INV_BIND_MATRICES_UNIFORM_NAME).value();

							std::string name = targetAdaptors.size() == 1 ? editorObject()->objectName() : fmt::format("{}_SkinBinding_{}", editorObject()->objectName(), index);
							auto skinBinding = ramses_base::ramsesSkinBinding(&sceneAdaptor_->logicEngine(), jointBindings, data->inverseBindMatrices, appearanceBinding, uniform, name, editorObject()->objectIDAsRamsesLogicID());
							if (skinBinding) {
								skinBindings_.emplace_back(skinBinding);
							} else {
								errorMessages.emplace_back(sceneAdaptor_->scene()->getRamsesClient().getRamsesFramework().getLastError().value().message);
							}
						} else {
							std::string errorMsg = fmt::format("Cannot create Skin '{}': private material of the meshnode '{}' doesn't have a valid AppearanceBinding", editorObject()->objectName(), meshNodeAdaptor->editorObject()->objectName());
							LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
							errorMessages.emplace_back(errorMsg);
						}
					} else {
						std::string errorMsg = fmt::format("Cannot create Skin '{}': meshnode '{}' must have a valid private material", editorObject()->objectName(), meshNodeAdaptor->editorObject()->objectName());
						LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
						errorMessages.emplace_back(errorMsg);
					}
				}
				
				if (errorMessages.empty()) {
					errors->removeError({editorObject()});
				} else {
					errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()},
						fmt::format("{}", fmt::join(errorMessages, "\n")));
				}

			} else {
				std::string errorMsg = fmt::format("Cannot create Skin '{}': no meshnodes selected", editorObject()->objectName());
				LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()}, errorMsg);
			}
		}
	}

	tagDirty(false);
	return true;
}

std::vector<ExportInformation> SkinAdaptor::getExportInformation() const {
	std::vector<ExportInformation> info;
	for (auto binding : skinBindings_) {
		info.emplace_back(ExportInformation{"SkinBinding", binding->getName()});
	}
	return info;
}

}  // namespace raco::ramses_adaptor