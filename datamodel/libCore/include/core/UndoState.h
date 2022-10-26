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

#include "core/Project.h"
#include "core/ChangeBase.h"
#include "core/StructCommon.h"

#include <functional>
#include <string>
#include <memory.h>

namespace raco::core {

class UndoState {
public:
    explicit UndoState();

    void saveCurrentUndoState();
    void push(STRUCT_VISUAL_CURVE_POS data);
    void push(STRUCT_NODE node);
    STRUCT_VISUAL_CURVE_POS visualPosData();
    STRUCT_FOLDER folderData();
    std::list<STRUCT_CURVE> curveData();
    STRUCT_NODE nodeData();
private:
    STRUCT_VISUAL_CURVE_POS posData_;
    STRUCT_FOLDER folderData_;
    std::list<STRUCT_CURVE> curveData_;
    STRUCT_NODE nodeData_;
};

}  // namespace raco::core
