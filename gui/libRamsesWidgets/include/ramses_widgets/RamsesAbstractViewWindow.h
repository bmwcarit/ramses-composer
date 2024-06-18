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

#include "RendererBackend.h"
#include <ramses/framework/RamsesFrameworkTypes.h>

namespace raco::ramses_widgets {

class RamsesAbstractViewWindow final {
public:
	struct State {
		ramses::sceneId_t sceneId{ramses::sceneId_t::Invalid()};
		QSize viewportSize{0, 0};

		bool operator!=(const State& other) const {
			return this->sceneId != other.sceneId || this->viewportSize != other.viewportSize;
		}
	};

	explicit RamsesAbstractViewWindow(void* windowHandle, RendererBackend& rendererBackend);
	~RamsesAbstractViewWindow();

	const State& currentState();
	State& nextState();

	void commit(bool forceUpdate = false);

private:
	void reset();

	void* windowHandle_;
	RendererBackend& rendererBackend_;

	bool errorState_ = false;

	ramses::displayId_t displayId_;

	State current_{};
	State next_{};
};

}  // namespace raco::ramses_widgets