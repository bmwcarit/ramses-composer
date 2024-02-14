/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/WidgetFactory.h"

#include "DockWidget.h"
#include "property_browser/controls/ImageWidget.h"
#include "property_browser/controls/TexturePreviewWidget.h"
#include "property_browser/PropertyBrowserItem.h"

#include "property_browser/editors/ArrayEditor.h"
#include "property_browser/editors/BoolEditor.h"
#include "property_browser/editors/DoubleEditor.h"
#include "property_browser/editors/EnumerationEditor.h"
#include "property_browser/editors/IntEditor.h"
#include "property_browser/editors/Int64Editor.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/RefEditor.h"
#include "property_browser/editors/StringEditor.h"
#include "property_browser/editors/TagContainerEditor.h"
#include "property_browser/editors/TexturePreviewEditor.h"
#include "property_browser/editors/URIEditor.h"
#include "property_browser/editors/VecNTEditor.h"

#include "user_types/UserTypeAnnotations.h"

#include <QLabel>
#include <QScreen>
#include <QWidget>

namespace raco::property_browser {

QWidget* WidgetFactory::createPropertyLabel(PropertyBrowserItem* item, QWidget* parent) {
	if (item->query<user_types::TexturePreviewEditorAnnotation>()) {
		return new TexturePreviewWidget{item, parent};
	}
	QLabel* label = new QLabel{item->displayName().c_str(), parent};
	label->setForegroundRole(QPalette::BrightText);
	label->setEnabled(item->editable());
	QObject::connect(item, &PropertyBrowserItem::editableChanged, label, &QWidget::setEnabled);
	return label;
}

PropertyEditor* WidgetFactory::createPropertyEditor(PropertyBrowserItem* item, QWidget* label, QWidget* parent) {
	using PrimitiveType = core::PrimitiveType;

	switch (item->type()) {
		case PrimitiveType::Bool:
			if (item->query<user_types::TexturePreviewEditorAnnotation>()) {
				return new TexturePreviewEditor{item, label, parent};
			} else {
				return new BoolEditor{item, parent};
			}
		case PrimitiveType::Int:
			if (item->query<core::EnumerationAnnotation>()) {
				return new EnumerationEditor{item, parent};
			} else {
				return new IntEditor{item, parent};
			}
		case PrimitiveType::Int64:
			return new Int64Editor{item, parent};
			
		case PrimitiveType::Double:
			return new DoubleEditor{item, parent};
		case PrimitiveType::String:
			return (item->query<core::URIAnnotation>()) ? new URIEditor{item, parent} : new StringEditor{item, parent};
		case PrimitiveType::Ref:
			return new RefEditor{item, parent};
		case PrimitiveType::Struct: {
			auto typeDesc = &item->valueHandles().begin()->constValueRef()->asStruct().getTypeDescription();
			if (typeDesc == &core::Vec2f::typeDescription) {
				return new Vec2fEditor{item, parent};
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				return new Vec3fEditor{item, parent};
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				return new Vec4fEditor{item, parent};
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				return new Vec2iEditor{item, parent};
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				return new Vec3iEditor{item, parent};
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				return new Vec4iEditor{item, parent};
			}
			return new PropertyEditor{ item, parent };
		}
		case PrimitiveType::Table:
			if (item->isTagContainerProperty()) {
				return new TagContainerEditor{ item, parent };
			}
			return new PropertyEditor{ item, parent };
		case PrimitiveType::Array:
			if (item->query<core::ResizableArray>()) {
				return new ArrayEditor{item, parent};
			} 
			return new PropertyEditor{item, parent};
		default:
			// used for group headlines
			return new PropertyEditor{item, parent};
	};
}

LinkEditor* WidgetFactory::createLinkControl(PropertyBrowserItem* item, QWidget* parent, QWidget* propertyEditor) {
	const auto linkEditor{new LinkEditor(item, parent)};
	linkEditor->setControl(propertyEditor);
	return linkEditor;
}

PropertyEditor* WidgetFactory::createPropertyControl(PropertyBrowserItem* item, QWidget* label, QWidget* parent) {
	auto propertyEditor = createPropertyEditor(item, label, parent);
	propertyEditor->setToolTip(item->propertyControlToolTipText());
	QObject::connect(item, &PropertyBrowserItem::propertyControlToolTipTextChanged, propertyEditor, [propertyEditor](const QString& text) {
		propertyEditor->setToolTip(text);
	});
	return propertyEditor;
}

std::tuple<QWidget*, LinkEditor*> WidgetFactory::createPropertyWidgets(PropertyBrowserItem* item, QWidget* parent) {
	QWidget* label = createPropertyLabel(item, parent);
	PropertyEditor* editor = createPropertyControl(item, label, parent);

	return {label, createLinkControl(item, parent, editor)};
}

}  // namespace raco::property_browser
