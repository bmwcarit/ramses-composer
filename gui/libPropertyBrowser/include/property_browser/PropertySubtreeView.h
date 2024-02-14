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

#include "core/SceneBackendInterface.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeChildrenContainer.h"

namespace raco::property_browser {
class PropertyControl;
class PropertyEditor;

// TODO use AbstractPropertyBrowserItem instead of the PropertyBrowserItem everywhere in the PropertySubtreeView
class PropertySubtreeView final : public QWidget {
	Q_OBJECT
	Q_PROPERTY(float highlight MEMBER highlight_ NOTIFY update)
public:
	explicit PropertySubtreeView(core::SceneBackendInterface* sceneBackend, PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent, PropertySubtreeView* parentSubtree);
	PropertyBrowserItem const* item() const { return item_; }
public Q_SLOTS:
	void playHighlightAnimation(int duration, float start, float end);
	void updateChildrenContainer();
	void ensurePropertyVisible();

protected:
	void paintEvent(QPaintEvent* event) override;
	Q_SLOT void updateErrors();

private:
	enum LayoutRows{		
		ErrorRow = 0,
		LabelRow = 1,
		ChildrenContainerRow = 2		
	};

	void setLabelAreaWidth(int labelAreaWidth);
	int getLabelAreaWidth() const;
	bool updateLabelAreaWidth(bool initialize = true);
	void collectTabWidgets(QObject* item, QWidgetList& tabWidgets);
	void recalculateTabOrder();
	void registerLabelContextMenu(QWidget* labelWidget, PropertyBrowserItem* item);
	void drawHighlight(float intensity);

	core::SceneBackendInterface* sceneBackend_;

	PropertyBrowserItem* item_{nullptr};
	PropertyBrowserModel* model_{nullptr};
	PropertyBrowserVBoxLayout layout_{nullptr};
	PropertyBrowserVBoxLayout* errorLayout_{nullptr};
	QWidget* decorationWidget_{nullptr};
	QWidget* label_{nullptr};
	QWidget* controlWidget_{nullptr};
	QWidget* errorContainer_{nullptr};

	PropertySubtreeView* parentSubtree_{nullptr};
	PropertySubtreeChildrenContainer* childrenContainer_{nullptr};
	int labelAreaWidth_{0};
	int labelMinWidth_{0};
	int subtreeMaxWidth_{0};

	float highlight_{0};
};

}  // namespace raco::property_browser
