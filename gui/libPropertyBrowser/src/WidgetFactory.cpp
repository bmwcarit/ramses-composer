/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/WidgetFactory.h"

#include "property_browser/PropertyBrowserItem.h"

#include "property_browser/editors/BoolEditor.h"
#include "property_browser/editors/DoubleEditor.h"
#include "property_browser/editors/EnumerationEditor.h"
#include "property_browser/editors/IntEditor.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/RefEditor.h"
#include "property_browser/editors/StringEditor.h"
#include "property_browser/editors/TagContainerEditor.h"
#include "property_browser/editors/URIEditor.h"
#include "property_browser/editors/VecNTEditor.h"

#include "user_types/RenderLayer.h"

#include <QLabel>
#include <QWidget>

namespace raco::property_browser {

QLabel* WidgetFactory::createPropertyLabel(PropertyBrowserItem* item, QWidget* parent) {
	QLabel* label = new QLabel{item->displayName().c_str(), parent};
	label->setForegroundRole(QPalette::BrightText);
	QObject::connect(item, &PropertyBrowserItem::displayNameChanged, label, &QLabel::setText);
	return label;
}

QWidget* WidgetFactory::createPropertyControl(PropertyBrowserItem* item, QWidget* parent) {
	using PrimitiveType = core::PrimitiveType;

	switch (item->type()) {
		case PrimitiveType::Bool:
			if (item->query<core::EnumerationAnnotation>()) {
				return new EnumerationEditor{item, parent};
			} else {
				return new BoolEditor{item, parent};
			}
		case PrimitiveType::Int:
			if (item->query<core::EnumerationAnnotation>()) {
				return new EnumerationEditor{item, parent};
			} else {
				return new IntEditor{item, parent};
			}
		case PrimitiveType::Double:
			return new DoubleEditor{item, parent};
		case PrimitiveType::String:
			return (item->query<core::URIAnnotation>()) ? new URIEditor{item, parent} : new StringEditor{item, parent};
		case PrimitiveType::Vec2f:
			return new Vec2fEditor{item, parent};
		case PrimitiveType::Vec3f:
			return new Vec3fEditor{item, parent};
		case PrimitiveType::Vec4f:
			return new Vec4fEditor{item, parent};
		case PrimitiveType::Vec2i:
			return new Vec2iEditor{item, parent};
		case PrimitiveType::Vec3i:
			return new Vec3iEditor{item, parent};
		case PrimitiveType::Vec4i:
			return new Vec4iEditor{item, parent};
		case PrimitiveType::Ref:
			return new RefEditor{item, parent};
		case PrimitiveType::Table:
			if (item->query<core::TagContainerAnnotation>() || item->query<core::RenderableTagContainerAnnotation>()) {
				return new TagContainerEditor{ item, parent };
			}
			return new QWidget{ parent };
		default:
			// used for group headlines
			return new QWidget{parent};
	};
}

LinkEditor* WidgetFactory::createLinkControl(PropertyBrowserItem* item, QWidget* parent) {
	return new LinkEditor(item, parent);
}

}  // namespace raco::property_browser
