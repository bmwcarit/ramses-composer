/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Undo.h"

#include "core/ChangeRecorder.h"
#include "core/Context.h"
#include "core/EditorObject.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/UserObjectFactoryInterface.h"
#include "core/Link.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"
#include "VisualCurveData/VisualCurvePosManager.h"
#include "visual_curve/VisualCurveWidget.h"

#include <cassert>

namespace raco::core {
using namespace raco::visualCurve;

UndoState::UndoState() {

}

void UndoState::push(STRUCT_VISUAL_CURVE_POS pos) {
    posData_.pos = pos;
    posData_.canMerge = true;
}

void UndoState::push(STRUCT_FOLDER folder) {
    folderData_.folder = folder;
    folderData_.canMerge = true;
}

void UndoState::push(std::list<STRUCT_CURVE> list) {
    curveData_.list = list;
    curveData_.canMerge = true;
}

void UndoState::push(STRUCT_ANIMATION data) {
    animationData_ = data;
}

bool UndoState::canMergeVisualCurve() {
    return posData_.canMerge;
}

bool UndoState::canMergeFolderData() {
    return folderData_.canMerge;
}

bool UndoState::canMergeCurveData() {
    return curveData_.canMerge;
}

STRUCT_POS_DATA UndoState::visualPosData() {
    return posData_;
}

STRUCT_FOLDER_DATA UndoState::folderData() {
    return folderData_;
}

STRUCT_CURVE_DATA UndoState::curveData() {
    return curveData_;
}

STRUCT_ANIMATION UndoState::animationData() {
    return animationData_;
}

}  // namespace raco::core
