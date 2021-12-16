/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "style/Icons.h"

namespace raco::style {

Icons& Icons::instance() {
	if (Icons::instance_ == nullptr) {
		Icons::instance_ = new Icons{};
		Icons::icons_ = {
			{Pixmap::done, QPixmap{":doneIcon"}},
			{Pixmap::trash, QPixmap{":trashIcon"}},
			{Pixmap::expanded, QPixmap{":expandedIcon"}},
			{Pixmap::collapsed, QPixmap{":collapsedIcon"}},
			{Pixmap::linkable, QPixmap{":linkableIcon"}},
			{Pixmap::linked, QPixmap{":linkedIcon"}},
			{Pixmap::parent_is_linked, QPixmap{":parentLinkedIcon"}},
			{Pixmap::unlinkable, QPixmap{":unlinkableIcon"}},
			{Pixmap::link_broken, QPixmap{":linkBrokenIcon"}},
			{Pixmap::locked, QPixmap{":lockedIcon"}},
			{Pixmap::unlocked, QPixmap{":unlockedIcon"}},
			{Pixmap::close, QPixmap{":closeIcon"}},
			{Pixmap::undock, QPixmap{":undockIcon"}},
			{Pixmap::menu, QPixmap{":menuIcon"}},
			{Pixmap::open_in_new, QPixmap{":openInNewIcon"}},
			{Pixmap::go_to, QPixmap{":gotoIcon"}},
			{Pixmap::increment, QPixmap{":incrementIcon"}},
			{Pixmap::decrement, QPixmap{":decrementIcon"}},
			{Pixmap::warning, QPixmap{":warningIcon"}},
			{Pixmap::error, QPixmap{":errorIcon"}},

			{Pixmap::typeNode, QPixmap{":typeNodeIcon"}},
			{Pixmap::typeCamera, QPixmap{":typeCameraIcon"}},
			{Pixmap::typeMesh, QPixmap{":typeMeshIcon"}},
			{Pixmap::typeMaterial, QPixmap{":typeMaterialIcon"}},
			{Pixmap::typeTexture, QPixmap{":typeTextureIcon"}},
			{Pixmap::typeCubemap, QPixmap{":typeCubemapIcon"}},
			{Pixmap::typeScript, QPixmap{":typeScriptIcon"}},
			{Pixmap::typePrefabInternal, QPixmap{":typePrefabInternalIcon"}},
			{Pixmap::typePrefabExternal, QPixmap{":typePrefabExternalIcon"}},
			{Pixmap::typePrefabInstance, QPixmap{":typePrefabInstanceIcon"}},
			{Pixmap::typeLuaScriptModule, QPixmap{":typeLuaScriptModuleIcon"}},
			{Pixmap::typeAnimationChannel, QPixmap{":typeAnimationChannelIcon"}},
			{Pixmap::typeAnimation, QPixmap{":typeAnimationIcon"}}
		};
	}
	return *Icons::instance_;
}

QMap<Pixmap, QPixmap> Icons::icons_{};
Icons* Icons::instance_{nullptr};

}  // namespace raco::style