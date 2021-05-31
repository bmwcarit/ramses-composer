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

#include <QIcon>
#include <QMap>
#include <QPixmap>

namespace raco::style {

enum class Pixmap {
	done,
	trash,

	unlocked,
	locked,
	expanded,
	collapsed,

	increment,
	decrement,

	linkable,
	linked,
	parent_is_linked,
	unlinkable,
	link_broken,

	warning,
	error,

	close,
	undock,
	menu,
	open_in_new,
	go_to,

	typeNode,
	typeCamera,
	typeMesh,
	typeMaterial,
	typeTexture,
	typeCubemap,
	typeScript,
	typePrefabInternal,
	typePrefabExternal,
	typePrefabInstance
};

class Icons {
public:
	static QIcon icon(Pixmap pixmap, const QWidget* widget = nullptr) {
		return Icons::instance().createIcon(pixmap, widget);
	}
	static QPixmap pixmap(Pixmap pixmap) {
		return Icons::instance().icons_[pixmap];
	}

private:
	QIcon createIcon(Pixmap pixmap, const QWidget* widget = nullptr) {
		return QIcon(QPixmap(icons_[pixmap]));
	}
	static Icons& instance();
	static Icons* instance_;
	static QMap<Pixmap, QPixmap> icons_;
};

}  // namespace raco::style
