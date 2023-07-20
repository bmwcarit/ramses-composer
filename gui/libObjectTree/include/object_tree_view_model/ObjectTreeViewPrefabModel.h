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

#include "ObjectTreeViewDefaultModel.h"

namespace raco::object_tree::model {

class ObjectTreeViewPrefabModel : public ObjectTreeViewDefaultModel {
	Q_OBJECT

public:
	ObjectTreeViewPrefabModel(raco::core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore, const std::vector<std::string>& allowedCreatableUserTypes = {});

	QVariant data(const QModelIndex& index, int role) const override;
};

}  // namespace raco::object_tree::model