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

#include "components/DataChangeDispatcher.h"
#include "components/DebugInstanceCounter.h"
#include "object_tree_view/ObjectTreeDockManager.h"
#include "ramses_widgets/RendererBackend.h"
#include <QLabel>
#include <QMainWindow>
#include <QToolButton>
#include <memory>

namespace raco::ramses_adaptor {
class AbstractSceneAdaptor;
}

namespace raco::ramses_widgets {

class AbstractViewContentWidget;

class AbstractViewMainWindow final : public QMainWindow {
	Q_OBJECT
	DEBUG_INSTANCE_COUNTER(AbstractViewMainWindow);

public:
	AbstractViewMainWindow(RendererBackend& rendererBackend,
		ramses_adaptor::AbstractSceneAdaptor* abstractScene,
		object_tree::view::ObjectTreeDockManager* objectTreeDockManager,
		core::CommandInterface* commandInterface,
		QWidget* parent = nullptr);
	~AbstractViewMainWindow();

	void focusCamera(const std::vector<core::SEditorObject>& objects);

Q_SIGNALS:
	void selectionRequested(const QString objectID, const QString objectProperty = {});

public Q_SLOTS:
	void onSelectionChanged(const core::SEditorObjectSet& objects);

private:
	enum class HighlightMode {
		None,
		Color,
		Transparency
	};

	void updateHighlighted();

	AbstractViewContentWidget* previewWidget_;
	ramses_adaptor::AbstractSceneAdaptor* abstractScene_;
	object_tree::view::ObjectTreeDockManager* treeDockManager_;

	HighlightMode highlightMode_ = HighlightMode::None;
};

}  // namespace raco::ramses_widgets