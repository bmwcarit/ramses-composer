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

#include <QMetaType>
#include <any>
#include <set>
#include "time_axis/TimeAxisCommon.h"

struct STRUCT_POINT {
    int keyFrame_;
    int interPolationType_{0};
    std::any data_{0.0};
    std::any leftTagent_{0.0};
    std::any rightTagent_{0.0};
    std::any leftData_{0.0};
    int leftKeyFrame_{0};
    std::any rightData_{0.0};
    int rightKeyFrame_{0};
};
Q_DECLARE_METATYPE(STRUCT_POINT)

struct STRUCT_CURVE {
    std::string curveName_;
    int dataType_{0};
    std::list<STRUCT_POINT> pointList;
};
Q_DECLARE_METATYPE(STRUCT_CURVE)

struct STRUCT_CURVE_PROP {
    std::string curve_;
    bool visible_{true};

    STRUCT_CURVE_PROP(std::string curve = std::string()) {
        curve_ = curve;
    }
};
Q_DECLARE_METATYPE(STRUCT_CURVE_PROP)

struct STRUCT_FOLDER {
    std::string folderName_;
    std::list<STRUCT_CURVE_PROP> curveList;
    std::list<STRUCT_FOLDER> folerList;
};
Q_DECLARE_METATYPE(STRUCT_FOLDER)

struct STRUCT_VISUAL_CURVE_POS {
    int curFrame_{0};
    double centerLinePos_{0};
    bool cursorShow_{true};
    raco::time_axis::MOUSE_PRESS_ACTION pressAction_;
    QPair<std::string, int> curPointInfo_;
    QMap<std::string, QList<raco::time_axis::SKeyPoint>> keyPointMap_;
    QList<std::string> hidenCurveList_;
};
Q_DECLARE_METATYPE(STRUCT_VISUAL_CURVE_POS)

struct STRUCT_ANIMATION_DATA {
    int startTime_;
    int endTime_;
    int loopCount_;
    int updateInterval_;
    double playSpeed_;
    std::set<std::string> nodeList_;
};
Q_DECLARE_METATYPE(STRUCT_ANIMATION_DATA)

struct STRUCT_ANIMATION {
    std::map<std::string, STRUCT_ANIMATION_DATA> animationMap_;
    std::string activeAnimation_;
};
Q_DECLARE_METATYPE(STRUCT_ANIMATION)

  // namespace raco::core
