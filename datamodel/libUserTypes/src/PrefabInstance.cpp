/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/PrefabInstance.h"
#include "core/Context.h"
#include "core/Handles.h"

namespace raco::user_types {

Table* findItem(Table& table, SEditorObject obj) {
	for (size_t i{0}; i < table.size(); i++) {
		Table& item = table.get(i)->asTable();
		if (item.get(0)->asRef() == obj) {
			return &item;
		}
	}
	return nullptr;
}

Table* findItemByValue(Table& table, SEditorObject obj) {
	for (size_t i{0}; i < table.size(); i++) {
		Table& item = table.get(i)->asTable();
		if (item.get(1)->asRef() == obj) {
			return &item;
		}
	}
	return nullptr;
}

int findItemIndex(Table& table, SEditorObject obj) {
	for (size_t i{0}; i < table.size(); i++) {
		Table& item = table.get(i)->asTable();
		if (item.get(0)->asRef() == obj) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

SEditorObject PrefabInstance::mapToInstance(SEditorObject obj, SPrefab prefab, SPrefabInstance instance) {
	if (obj == prefab) {
		return instance;
	}
	if (Table* item = findItem(*(instance->mapToInstance_) ,obj)) {
		return item->get(1)->asRef();
	}
	return nullptr;
}

SEditorObject PrefabInstance::mapFromInstance(SEditorObject obj, SPrefabInstance instance) {
	if (Table* item = findItemByValue(*(instance->mapToInstance_), obj)) {
		return item->get(0)->asRef();
	}
	return nullptr;
}


void PrefabInstance::removePrefabInstanceChild(BaseContext& context, const SEditorObject& prefabChild) {
	int index = findItemIndex(*mapToInstance_, prefabChild);
	if (index != -1) {
		context.removeProperty({shared_from_this(), {"mapToInstance"}}, index);
	}
}

void PrefabInstance::addChildMapping(BaseContext& context, const SEditorObject& prefabChild, const SEditorObject& instanceChild) {
	if (Table* item = findItem(*mapToInstance_, prefabChild)) {
		*item->get(1) = instanceChild;
	} else {
		auto newItem = mapToInstance_->addProperty(PrimitiveType::Table);
		auto key = newItem->asTable().addProperty("prefabChild", PrimitiveType::Ref);
		auto val = newItem->asTable().addProperty("instChild", PrimitiveType::Ref);
		*key = prefabChild;
		*val = instanceChild;
	}
	context.changeMultiplexer().recordValueChanged(ValueHandle(shared_from_this(), {"mapToInstance"}));
}


}  // namespace raco::user_types
