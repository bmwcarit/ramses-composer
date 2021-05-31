/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "components/DataChangeDispatcher.h"
#include "core/Undo.h"
#include <QGridLayout>
#include <QListView>
#include <QStandardItemModel>
#include <QWidget>

namespace raco::common_widgets {

class UndoView : public QWidget {
public:
	explicit UndoView(raco::core::UndoStack* undoStack, raco::components::SDataChangeDispatcher dispatcher, QWidget* parent);

protected:
	void rebuild();

private:
	raco::components::Subscription sub_;
	raco::core::UndoStack* undoStack_;
	QListView* list_;
	QStandardItemModel* model_;
};

}  // namespace raco::common_widgets
