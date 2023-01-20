/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/PropertyEditor.h"
#include "common_widgets/RaCoClipboard.h"
#include "core/EngineInterface.h"
#include "core/Serialization.h"
#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

PropertyEditor::PropertyEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: QWidget{parent}, item_{item} {
	setEnabled(item->editable());
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);
}

void PropertyEditor::copyValue() {
	raco::RaCoClipboard::set(raco::serialization::serializeProperty(*item_->project(), *item_->valueHandle().constValueRef()), raco::MimeTypes::PROPERTY);
}

void PropertyEditor::pasteInt(PropertyBrowserItem* item, core::ValueBase* value) {
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
		if (item->commandInterface()->canSet(item->valueHandle(), newValue.value())) {
			item->set(newValue.value());
		}
	}
}

void PropertyEditor::pasteBool(PropertyBrowserItem* item, core::ValueBase* value) {
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

void PropertyEditor::pasteInt64(PropertyBrowserItem* item, core::ValueBase* value) {
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

void PropertyEditor::pasteDouble(PropertyBrowserItem* item, core::ValueBase* value) {
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

void PropertyEditor::pasteString(PropertyBrowserItem* item, core::ValueBase* value) {
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

void PropertyEditor::pasteRef(PropertyBrowserItem* item, core::ValueBase* value) {
	if (value->type() == data_storage::PrimitiveType::Ref) {
		auto valueRef = value->asRef();
		if (core::Queries::isValidReferenceTarget(*item->project(), item->valueHandle(), valueRef)) {
			item->set(valueRef);
		}
	}
}

template <typename T>
T PropertyEditor::getFittingVectorValueForPasting(data_storage::ReflectionInterface& substructure, const std::string& keyInIntVector, const std::string& keyInDoubleVector) {
	if (substructure.hasProperty(keyInIntVector)) {
		return static_cast<T>(substructure[keyInIntVector]->asInt());
	}
	if (substructure.hasProperty(keyInDoubleVector)) {
		return static_cast<T>(substructure[keyInDoubleVector]->asDouble());
	}

	return static_cast<T>(0);
}

template <typename T>
void PropertyEditor::pasteVector(PropertyBrowserItem* item, core::ValueBase* value) {
	const auto childCount = item->children().length();
	switch (value->type()) {
		case data_storage::PrimitiveType::Struct: {
			data_storage::ReflectionInterface& substructure = value->getSubstructure();
			item->children().at(0)->set(getFittingVectorValueForPasting<T>(substructure, "i1", "x"));
			item->children().at(1)->set(getFittingVectorValueForPasting<T>(substructure, "i2", "y"));
			if (childCount > 2) {
				item->children().at(2)->set(getFittingVectorValueForPasting<T>(substructure, "i3", "z"));
				if (childCount > 3) {
					item->children().at(3)->set(getFittingVectorValueForPasting<T>(substructure, "i4", "w"));
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

bool PropertyEditor::isVector(PropertyBrowserItem* item) {
	auto vh = item->valueHandle();
	return vh.isVec2f() || vh.isVec3f() || vh.isVec4f() || vh.isVec2i() || vh.isVec3i() || vh.isVec4i();
}

bool PropertyEditor::isVector(core::ValueBase* value) {
	auto* td = &value->asStruct().getTypeDescription();
	return td == &core::Vec2f::typeDescription ||
		   td == &core::Vec3f::typeDescription ||
		   td == &core::Vec4f::typeDescription ||
		   td == &core::Vec2i::typeDescription ||
		   td == &core::Vec3i::typeDescription ||
		   td == &core::Vec4i::typeDescription;
}

void PropertyEditor::pasteStruct(PropertyBrowserItem* item, core::ValueBase* value) {
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
			auto propName = child->valueHandle().getPropName();
			if (value->asStruct().hasProperty(propName)) {
				pastePropertyOfSameType(child, value->asStruct()[propName]);
			}
		}
	}
}

void PropertyEditor::pasteTable(PropertyBrowserItem* item, core::ValueBase* value) {
	if (value->type() != data_storage::PrimitiveType::Table) {
		return;
	}

	if (item->query<raco::core::TagContainerAnnotation>()) {
		if (value->query<raco::core::TagContainerAnnotation>()) {
			auto srcTags = value->asTable().asVector<std::string>();
			if (item->commandInterface()->canSetTags(item->valueHandle(), srcTags)) {
				item->setTags(srcTags);
			}
		}
		return;
	}

	if (value->query<raco::core::TagContainerAnnotation>()) {
		return;
	}

	if (item->query<raco::core::RenderableTagContainerAnnotation>()) {
		if (value->query<raco::core::RenderableTagContainerAnnotation>()) {
			std::vector<std::pair<std::string, int>> renderableTags;
			const auto& table = value->asTable();
			for (size_t index = 0; index < table.size(); index++) {
				renderableTags.emplace_back(std::make_pair(table.name(index), table.get(index)->asInt()));
			}
			if (item->commandInterface()->canSetRenderableTags(item->valueHandle(), renderableTags)) {
				item->setTags(renderableTags);
			}
		}
		return;
	}

	if (value->query<raco::core::RenderableTagContainerAnnotation>()) {
		return;
	}

	auto& substructure = value->getSubstructure();
	for (const auto& child : item->children()) {
		auto propName = child->valueHandle().getPropName();
		if (substructure.hasProperty(propName)) {
			pastePropertyOfSameType(child, substructure[propName]);
		}
	}
}

void PropertyEditor::pastePropertyOfSameType(PropertyBrowserItem* item, data_storage::ValueBase* value) {
	if (value->type() == item->type()) {
		pasteProperty(item, value);
	}
}

void PropertyEditor::pasteProperty(PropertyBrowserItem* item, data_storage::ValueBase* value) {
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

void PropertyEditor::pasteValue() {
	auto json = raco::RaCoClipboard::get(raco::MimeTypes::PROPERTY);
	if (auto value = serialization::deserializeProperty(*item_->project() , json)) {
		pasteProperty(item_, value.get());
	}
}

}  // namespace raco::property_browser
