/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/UndoView.h"

#include "common_widgets/NoContentMarginsLayout.h"
#include "core/Project.h"

#include <QMessageBox>

namespace raco::common_widgets {

UndoView::UndoView(core::UndoStack* undoStack, components::SDataChangeDispatcher dispatcher, QWidget* parent) : QWidget{parent}, undoStack_{undoStack}, sub_{dispatcher->registerOnUndoChanged([this]() { rebuild(); })} {
	auto* layout{new NoContentMarginsLayout<QGridLayout>{this}};
	list_ = new QListView{this};
	model_ = new QStandardItemModel{list_};
	list_->setModel(model_);
	list_->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	list_->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
	layout->addWidget(list_);
	QObject::connect(list_, &QListView::clicked, this, [this](const QModelIndex& index) {
		if (index.row() != undoStack_->getIndex())
			undoStack_->setIndex(static_cast<size_t>(index.row()));
	});
	rebuild();
}

void UndoView::rebuild() {
	model_->clear();
	for (size_t index{0}; index < undoStack_->size(); index++) {
		auto* item{new QStandardItem{QString{undoStack_->description(index).c_str()}}};
		item->setEditable(false);
		model_->appendRow(item);
	}
	list_->setCurrentIndex(model_->index(static_cast<int>(undoStack_->getIndex()), 0));
}

}  // namespace raco::common_widgets
