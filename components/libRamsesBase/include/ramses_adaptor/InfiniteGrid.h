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

#include "ramses_base/RamsesHandles.h"

namespace raco::ramses_adaptor {

class InfiniteGrid {
public:
	InfiniteGrid(ramses::Scene* scene);

	void setup(ramses_base::RamsesRenderPass renderPass, bool depthTesting, float axisColorFac, int renderOrder);

	void enable(glm::vec3 origin, glm::vec3 u, glm::vec3 v, int idx_u, int idx_v, bool full_grid, glm::vec2 enable);
	void disable();

	void setScale(float scale);

private:
	ramses::Scene* scene_;
	ramses_base::RamsesRenderPass renderPass_;

	float scale_ = 1.0;

	ramses_base::RamsesRenderGroup renderGroup_;
	ramses_base::RamsesEffect effect_;
	ramses_base::RamsesAppearance appearance_;
	ramses_base::RamsesGeometry geometry_;
	ramses_base::RamsesMeshNode meshNode_;
};

}  // namespace raco::ramses_adaptor