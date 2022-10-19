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

struct STRUCT_POS_DATA {
    STRUCT_VISUAL_CURVE_POS pos;
    bool canMerge{false};
};

struct STRUCT_FOLDER_DATA {
    STRUCT_FOLDER folder;
    bool canMerge{false};
};

struct STRUCT_CURVE_DATA {
    std::list<STRUCT_CURVE> list;
    bool canMerge{false};
};

class UndoState {
public:
    explicit UndoState();

    void push(STRUCT_VISUAL_CURVE_POS pos);
    void push(STRUCT_FOLDER folder);
    void push(std::list<STRUCT_CURVE> list);
    void push(STRUCT_ANIMATION data);
    bool canMergeVisualCurve();
    bool canMergeFolderData();
    bool canMergeCurveData();
    STRUCT_POS_DATA visualPosData();
    STRUCT_FOLDER_DATA folderData();
    STRUCT_CURVE_DATA curveData();
    STRUCT_ANIMATION animationData();
private:
    STRUCT_POS_DATA posData_;
    STRUCT_FOLDER_DATA folderData_;
    STRUCT_CURVE_DATA curveData_;
    STRUCT_ANIMATION animationData_;
};

}  // namespace raco::core
