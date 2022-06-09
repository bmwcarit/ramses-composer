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
#include <QPixmap>

namespace raco::style {

typedef QString IconPath;

class Icons {
public:
	const QIcon done{":doneIcon"};
	const QIcon remove{":removeIcon"};
	const QIcon expanded{":expandedIcon"};
	const QIcon collapsed{":collapsedIcon"};
	const QIcon linkable{":linkableIcon"};
	const QIcon linked{":linkedIcon"};
	const QIcon parentIsLinked{":parentLinkedIcon"};
	const QIcon unlinkable{":unlinkableIcon"};
	const QIcon linkBroken{":linkBrokenIcon"};
	const QIcon locked{":lockedIcon"};
	const QIcon unlocked{":unlockedIcon"};
	const QIcon close{":closeIcon"};
	const QIcon undock{":undockIcon"};
	const QIcon menu{":menuIcon"};
	const QIcon openInNew{":openInNewIcon"};
	const QIcon goTo{":gotoIcon"};
	const QIcon goToLeft{":gotoLeftIcon"};
	const QIcon goToLeftRight{":gotoLeftRightIcon"};
	const QIcon increment{":incrementIcon"};
	const QIcon decrement{":decrementIcon"};
	const QIcon warning{":warningIcon"};
	const QIcon error{":errorIcon"};
	const QIcon typeNode{":typeNodeIcon"};
	const QIcon typeCamera{":typeCameraIcon"};
	const QIcon typeMesh{":typeMeshIcon"};
	const QIcon typeMaterial{":typeMaterialIcon"};
	const QIcon typeTexture{":typeTextureIcon"};
	const QIcon typeCubemap{":typeCubemapIcon"};
	const QIcon typeLuaScript{":typeLuaScriptIcon"};
	const QIcon typePrefabInternal{":typePrefabInternalIcon"};
	const QIcon typePrefabExternal{":typePrefabExternalIcon"};
	const QIcon typePrefabInstance{":typePrefabInstanceIcon"};
	const QIcon typeLuaScriptModule{":typeLuaScriptModuleIcon"};
	const QIcon typeAnimationChannel{":typeAnimationChannelIcon"};
	const QIcon typeAnimation{":typeAnimationIcon"};
	const QIcon typeTimer{":typeTimerIcon"};
    const QIcon typeZoom{":typeZoomIcon"};
    const QIcon animationNext{":typeAnimationNextIcon"};
    const QIcon animationPrevious{":typeAnimationPreviousIcon"};
    const QIcon animationStart{":typeAnimationStartIcon"};
    const QIcon animationStop{":typeAnimationStopIcon"};
    const QIcon appLogo{":applicationLogo"};

	static const Icons& instance();

	Icons(const Icons &) = delete;
	Icons &operator=(const Icons &) = delete;

private:
	Icons();
	~Icons();
};

}  // namespace raco::style
