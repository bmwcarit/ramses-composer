/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/PropertyCopyPaste.h"

#include "common_widgets/RaCoClipboard.h"

namespace raco::property_browser {

bool PropertyCopyPaste::canCopyValue(PropertyBrowserItem* item) {
	return item->hasSingleValue();
}

void PropertyCopyPaste::copyValue(PropertyBrowserItem* item) {
	if (item->hasSingleValue()) {
		RaCoClipboard::set(serialization::serializeProperty(*item->project(), *item->valueHandles().begin()->constValueRef()), MimeTypes::PROPERTY);
	}
}

void PropertyCopyPaste::copyValuePlainText(PropertyBrowserItem* item) {
	const auto valueRef = item->valueHandles().cbegin()->constValueRef();
	copyValuePlainText(valueRef);
}

void PropertyCopyPaste::copyChildValuePlainText(PropertyBrowserItem* item, int index) {
	const auto valueRef = item->children().at(index)->valueHandles().cbegin()->constValueRef();
	copyValuePlainText(valueRef);
}

void PropertyCopyPaste::copyValuePlainText(const core::ValueBase* valueBase) {
	std::string value{};

	switch (valueBase->type()) {
		case serialization::PrimitiveType::Bool:
			value = std::to_string(valueBase->asBool());
			break;
		case serialization::PrimitiveType::Double:
			value = std::to_string(valueBase->asDouble());
			break;
		case serialization::PrimitiveType::Int:
			value = std::to_string(valueBase->asInt());
			break;
		case serialization::PrimitiveType::Int64:
			value = std::to_string(valueBase->asInt64());
			break;
		case serialization::PrimitiveType::String: 
			value = valueBase->asString();
			break;
	}

	if (!value.empty()) {
		const auto mimeType = "text/plain";
		RaCoClipboard::set(value, mimeType);
	}
}

void PropertyCopyPaste::pasteInt(PropertyBrowserItem* item, core::ValueBase* value) {
	std::optional<int> newValue;
	switch (value->type()) {
		case data_storage::PrimitiveType::Int: {
			newValue = value->asInt();
			break;
		}
		case data_storage::PrimitiveType::Int64:
			newValue = static_cast<int>(value->asInt64());
			break;
		case data_storage::PrimitiveType::String:
			try {
				newValue = std::stoi(value->asString());
			} catch (std::invalid_argument&) {
				break;
			} catch (std::out_of_range&) {
				break;
			}
			break;
		case data_storage::PrimitiveType::Double:
			newValue = static_cast<int>(value->asDouble());
			break;
		case data_storage::PrimitiveType::Bool:
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
		case data_storage::PrimitiveType::Struct:
			break;
	}

	if (newValue.has_value()) {
		// Target property may be an enum so we have to check:
		if (std::all_of(item->valueHandles().begin(), item->valueHandles().end(), [&item, &newValue](const core::ValueHandle& handle) {
				return item->commandInterface()->canSet(handle, newValue.value());
			})) {
			item->set(newValue.value());
		}
	}
}

void PropertyCopyPaste::pasteBool(PropertyBrowserItem* item, core::ValueBase* value) {
	switch (value->type()) {
		case data_storage::PrimitiveType::Bool:
			item->set(value->asBool());
			break;
		case data_storage::PrimitiveType::String:
			if (value->asString() == "true") {
				item->set(true);
			} else if (value->asString() == "false") {
				item->set(false);
			}
		case data_storage::PrimitiveType::Int:
		case data_storage::PrimitiveType::Int64:
		case data_storage::PrimitiveType::Double:
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
		case data_storage::PrimitiveType::Struct:
			break;
	}
}

void PropertyCopyPaste::pasteInt64(PropertyBrowserItem* item, core::ValueBase* value) {
	switch (value->type()) {
		case data_storage::PrimitiveType::Int:
			item->set(static_cast<int64_t>(value->asInt()));
			break;
		case data_storage::PrimitiveType::Int64:
			item->set(value->asInt64());
			break;
		case data_storage::PrimitiveType::String:
			try {
				int64_t result = std::stol(value->asString());
				item->set(result);
			} catch (std::invalid_argument&) {
				break;
			} catch (std::out_of_range&) {
				break;
			}
			break;
		case data_storage::PrimitiveType::Double:
			item->set(static_cast<int64_t>(value->asDouble()));
			break;
		case data_storage::PrimitiveType::Bool:
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
		case data_storage::PrimitiveType::Struct:
			break;
	}
}

void PropertyCopyPaste::pasteDouble(PropertyBrowserItem* item, core::ValueBase* value) {
	switch (value->type()) {
		case data_storage::PrimitiveType::Int:
			item->set(static_cast<double>(value->asInt()));
			break;
		case data_storage::PrimitiveType::Int64:
			item->set(static_cast<double>(value->asInt64()));
			break;
		case data_storage::PrimitiveType::String:
			try {
				double result = std::stod(value->asString());
				item->set(result);
			} catch (std::invalid_argument&) {
				break;
			} catch (std::out_of_range&) {
				break;
			}
			break;
		case data_storage::PrimitiveType::Double:
			item->set(value->asDouble());
			break;
		case data_storage::PrimitiveType::Bool:
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
		case data_storage::PrimitiveType::Struct:
			break;
	}
}

void PropertyCopyPaste::pasteString(PropertyBrowserItem* item, core::ValueBase* value) {
	switch (value->type()) {
		case data_storage::PrimitiveType::Int:
			item->set(std::to_string(value->asInt()));
			break;
		case data_storage::PrimitiveType::Int64:
			item->set(std::to_string(value->asInt64()));
			break;
		case data_storage::PrimitiveType::String:
			item->set(value->asString());
			break;
		case data_storage::PrimitiveType::Double:
			item->set(std::to_string(value->asDouble()));
			break;
		case data_storage::PrimitiveType::Bool:
			item->set(value->asBool() ? "true" : "false");
			break;
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
		case data_storage::PrimitiveType::Struct:
			break;
	}
}

void PropertyCopyPaste::pasteRef(PropertyBrowserItem* item, core::ValueBase* value) {
	if (value->type() == data_storage::PrimitiveType::Ref) {
		auto valueRef = value->asRef();
		if (std::all_of(item->valueHandles().begin(), item->valueHandles().end(), [&item, &valueRef](const core::ValueHandle& handle) {
				return core::Queries::isValidReferenceTarget(*item->project(), handle, valueRef);
			})) {
			item->set(valueRef);
		}
	}
}

template <typename T>
void PropertyCopyPaste::pasteVector(PropertyBrowserItem* item, core::ValueBase* value) {
	size_t childCount = item->children().length();
	switch (value->type()) {
		case data_storage::PrimitiveType::Struct: {
			data_storage::ReflectionInterface& substructure = value->getSubstructure();
			auto substructureChildCount = substructure.size();
			for (int i = 0; i < std::min(childCount, substructureChildCount); ++i) {
				switch (substructure[i]->type()) {
					case data_storage::PrimitiveType::Int: {
						item->children().at(i)->set(static_cast<T>(substructure[i]->asInt()));
						break;
					}
					case data_storage::PrimitiveType::Double: {
						item->children().at(i)->set(static_cast<T>(substructure[i]->asDouble()));
						break;
					}
					default:
						break;
				}
			}
			return;
		}
		case data_storage::PrimitiveType::Int:
		case data_storage::PrimitiveType::Int64:
		case data_storage::PrimitiveType::String:
		case data_storage::PrimitiveType::Double:
		case data_storage::PrimitiveType::Bool:
		case data_storage::PrimitiveType::Ref:
		case data_storage::PrimitiveType::Table:
			return;
	}
}

bool PropertyCopyPaste::isVector(PropertyBrowserItem* item) {
	return std::all_of(item->valueHandles().begin(), item->valueHandles().end(), [](const core::ValueHandle& handle) {
		return handle.isVec2f() || handle.isVec3f() || handle.isVec4f() || handle.isVec2i() || handle.isVec3i() || handle.isVec4i();
	});
}

bool PropertyCopyPaste::isVector(core::ValueBase* value) {
	auto* td = &value->asStruct().getTypeDescription();
	return td == &core::Vec2f::typeDescription ||
		   td == &core::Vec3f::typeDescription ||
		   td == &core::Vec4f::typeDescription ||
		   td == &core::Vec2i::typeDescription ||
		   td == &core::Vec3i::typeDescription ||
		   td == &core::Vec4i::typeDescription;
}

void PropertyCopyPaste::pasteStruct(PropertyBrowserItem* item, core::ValueBase* value) {
	if (value->type() != data_storage::PrimitiveType::Struct) {
		return;
	}

	if (isVector(item) && isVector(value)) {
		if (item->children().at(0)->type() == data_storage::PrimitiveType::Int) {
			pasteVector<int>(item, value);
		} else {
			pasteVector<double>(item, value);
		}
	} else {
		for (const auto& child : item->children()) {
			auto propName = child->getPropertyName();
			if (value->asStruct().hasProperty(propName)) {
				pastePropertyOfSameType(child, value->asStruct()[propName]);
			}
		}
	}
}

void PropertyCopyPaste::pasteTable(PropertyBrowserItem* item, core::ValueBase* value) {
	if (value->type() != data_storage::PrimitiveType::Table) {
		return;
	}

	if (item->query<core::TagContainerAnnotation>()) {
		if (value->query<core::TagContainerAnnotation>()) {
			auto srcTags = value->asTable().asVector<std::string>();
			if (std::all_of(item->valueHandles().begin(), item->valueHandles().end(), [&item, &srcTags](const core::ValueHandle& handle) {
					return item->commandInterface()->canSetTags(handle, srcTags);
				})) {
				item->setTags(srcTags);
			}
		}
		return;
	}

	if (value->query<core::TagContainerAnnotation>()) {
		return;
	}

	if (item->query<core::RenderableTagContainerAnnotation>()) {
		if (value->query<core::RenderableTagContainerAnnotation>()) {
			std::vector<std::pair<std::string, int>> renderableTags;
			const auto& table = value->asTable();
			for (size_t index = 0; index < table.size(); index++) {
				renderableTags.emplace_back(std::make_pair(table.name(index), table.get(index)->asInt()));
			}
			if (std::all_of(item->valueHandles().begin(), item->valueHandles().end(), [&item, &renderableTags](const core::ValueHandle& handle) {
					return item->commandInterface()->canSetRenderableTags(handle, renderableTags);
				})) {
				item->setTags(renderableTags);
			}
		}
		return;
	}

	if (value->query<core::RenderableTagContainerAnnotation>()) {
		return;
	}

	auto& substructure = value->getSubstructure();
	for (const auto& child : item->children()) {
		auto propName = child->getPropertyName();
		if (substructure.hasProperty(propName)) {
			pastePropertyOfSameType(child, substructure[propName]);
		}
	}
}

void PropertyCopyPaste::pastePropertyOfSameType(PropertyBrowserItem* item, data_storage::ValueBase* value) {
	if (value->type() == item->type()) {
		pasteProperty(item, value);
	}
}

void PropertyCopyPaste::pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
	switch (item->type()) {
		case data_storage::PrimitiveType::Bool:
			pasteBool(item, value);
			break;
		case data_storage::PrimitiveType::Int:
			pasteInt(item, value);
			break;
		case data_storage::PrimitiveType::Int64:
			pasteInt64(item, value);
			break;
		case data_storage::PrimitiveType::Double:
			pasteDouble(item, value);
			break;
		case data_storage::PrimitiveType::String:
			pasteString(item, value);
			break;
		case data_storage::PrimitiveType::Ref:
			pasteRef(item, value);
			break;
		case data_storage::PrimitiveType::Table:
			pasteTable(item, value);
			break;
		case data_storage::PrimitiveType::Struct:
			pasteStruct(item, value);
			break;
	}
}

bool PropertyCopyPaste::canPasteValue(PropertyBrowserItem* item) {
	auto json = RaCoClipboard::get(MimeTypes::PROPERTY);
	return serialization::deserializeProperty(*item->project(), json) != nullptr;
}

void PropertyCopyPaste::pasteValue(PropertyBrowserItem* item) {
	auto json = RaCoClipboard::get(MimeTypes::PROPERTY);
	if (auto value = serialization::deserializeProperty(*item->project(), json)) {
		std::string desc = fmt::format("Paste value into property '{}'", item->getPropertyPath());
		item->commandInterface()->executeCompositeCommand(
			[item, &value]() {
				pasteProperty(item, value.get());
			},
			desc);
	}
}

}  // namespace raco::property_browser
