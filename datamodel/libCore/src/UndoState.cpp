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
#include "FolderData/FolderDataManager.h"

#include <cassert>

namespace raco::core {
using namespace raco::visualCurve;

UndoState::UndoState() {

}

void UndoState::saveCurrentUndoState() {
    posData_ = raco::guiData::VisualCurvePosManager::GetInstance().convertDataStruct();
    curveData_ = raco::guiData::CurveManager::GetInstance().convertCurveData();
    folderData_ = raco::guiData::FolderDataManager::GetInstance().converFolderData();
    nodeData_ = raco::guiData::NodeDataManager::GetInstance().convertCurveData();
}

void UndoState::push(STRUCT_VISUAL_CURVE_POS data) {
    posData_ = data;
}

void UndoState::push(STRUCT_NODE node) {
    nodeData_ = node;
}

STRUCT_VISUAL_CURVE_POS UndoState::visualPosData() {
    return posData_;
}

STRUCT_FOLDER UndoState::folderData() {
    return folderData_;
}

std::list<STRUCT_CURVE> UndoState::curveData() {
    return curveData_;
}

STRUCT_NODE UndoState::nodeData() {
    return nodeData_;
}

}  // namespace raco::core
