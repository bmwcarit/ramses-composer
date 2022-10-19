#include "VisualCurveData/VisualCurvePosManager.h"

namespace raco::guiData {
VisualCurvePosManager &VisualCurvePosManager::GetInstance() {
    static VisualCurvePosManager Instance;
    return Instance;
}

VisualCurvePosManager::VisualCurvePosManager() {

}

STRUCT_VISUAL_CURVE_POS VisualCurvePosManager::convertDataStruct() {
    STRUCT_VISUAL_CURVE_POS pos;
    pos.curFrame_ = curFrame_;
    pos.centerLinePos_ = centerLinePos_;
    pos.cursorShow_ = cursorShow_;
    pos.pressAction_ = pressAction_;
    pos.curPointInfo_ = curPointInfo_;
    pos.keyPointMap_ = keyPointMap_;
    pos.hidenCurveList_ = hidenCurveList_;
    return pos;
}

void VisualCurvePosManager::merge(QVariant data) {
    if (data.canConvert<STRUCT_VISUAL_CURVE_POS>()) {
        STRUCT_VISUAL_CURVE_POS pos = data.value<STRUCT_VISUAL_CURVE_POS>();

        curFrame_ = pos.curFrame_;
        centerLinePos_  = pos.centerLinePos_;
        cursorShow_ = pos.cursorShow_;
        pressAction_ = pos.pressAction_;
        curPointInfo_ = pos.curPointInfo_;
        keyPointMap_ = pos.keyPointMap_;
        hidenCurveList_ = pos.hidenCurveList_;
    }
}

void VisualCurvePosManager::setKeyBoardType(KEY_BOARD_TYPE type) {
    keyBoardType_ = type;
}

KEY_BOARD_TYPE VisualCurvePosManager::getKeyBoardType() {
    return keyBoardType_;
}

double VisualCurvePosManager::getCenterLinePos() {
    return centerLinePos_;
}

void VisualCurvePosManager::setCenterLinePos(double value) {
    centerLinePos_ = value;
}

int VisualCurvePosManager::getCurFrame() {
    return curFrame_;
}

void VisualCurvePosManager::setCurFrame(int frame) {
    curFrame_ = frame;
}

int VisualCurvePosManager::getCurX() {
    return curX_;
}

int VisualCurvePosManager::getCurY() {
    return curY_;
}

double VisualCurvePosManager::getEachFrameWidth() {
    return eachFrameWidth_;
}

double VisualCurvePosManager::getEachValueWidth() {
    return eachValueWidth_;
}

void VisualCurvePosManager::setCurPos(int x, int y) {
    curX_ = x;
    curY_ = y;
}

void VisualCurvePosManager::setWidth(double frameWidth, double valueWidth) {
    eachFrameWidth_ = frameWidth;
    eachValueWidth_ = valueWidth;
}

void VisualCurvePosManager::setCursorShow(bool show) {
    cursorShow_ = show;
}

bool VisualCurvePosManager::getCursorShow() {
    return cursorShow_;
}

void VisualCurvePosManager::insertSameKeyPoint(int index, SAME_KEY_TYPE type) {
    sameKeyPointsInfo_.insert(index, type);
}

QMap<int, SAME_KEY_TYPE> VisualCurvePosManager::getSameKeyPointsInfo() {
    return sameKeyPointsInfo_;
}

void VisualCurvePosManager::clearSameKeyPoints() {
    sameKeyPointsInfo_.clear();
}

SAME_KEY_TYPE VisualCurvePosManager::getSameKeyType() {
    return sameKeyType_;
}

void VisualCurvePosManager::setSameKeyType(SAME_KEY_TYPE type) {
    sameKeyType_ = type;
}

int VisualCurvePosManager::getSameKeyPointIndex() {
    return sameKeyPointIndex_;
}

void VisualCurvePosManager::setSameKeyPointIndex(int index) {
    sameKeyPointIndex_ = index;
}

MOUSE_PRESS_ACTION VisualCurvePosManager::getPressAction() {
    return pressAction_;
}

void VisualCurvePosManager::setPressAction(MOUSE_PRESS_ACTION action) {
    pressAction_ = action;
}

void VisualCurvePosManager::insertHidenCurve(std::string curve) {
    hidenCurveList_.push_back(curve);
}

void VisualCurvePosManager::deleteHidenCurve(std::string curve) {
    hidenCurveList_.removeOne(curve);
}

bool VisualCurvePosManager::hasHidenCurve(std::string curve) {
    return hidenCurveList_.contains(curve);
}

void VisualCurvePosManager::resetCurrentPointInfo() {
    QPair<std::string, int>().swap(curPointInfo_);
}

QPair<std::string, int> VisualCurvePosManager::getCurrentPointInfo() {
    return curPointInfo_;
}

void VisualCurvePosManager::setCurrentPointInfo(std::string curve, int index) {
    curPointInfo_.first = curve;
    curPointInfo_.second = index;
}

void VisualCurvePosManager::setCurrentPointInfo(std::string curve) {
    curPointInfo_.first = curve;
}

void VisualCurvePosManager::setCurrentPointInfo(int index) {
    curPointInfo_.second = index;
}

QMap<std::string, QList<SKeyPoint> > VisualCurvePosManager::getKeyPointMap() {
    return keyPointMap_;
}

void VisualCurvePosManager::setKeyPointMap(QMap<std::string, QList<SKeyPoint> > map) {
    keyPointMap_ = map;
}

void VisualCurvePosManager::clearKeyPointMap() {
    keyPointMap_.clear();
}

void VisualCurvePosManager::addKeyPointList(std::string curve, QList<SKeyPoint> points) {
    keyPointMap_.insert(curve, points);
}


void VisualCurvePosManager::deleteKeyPointList(std::string curve) {
    keyPointMap_.remove(curve);
}

bool VisualCurvePosManager::getCurKeyPoint(SKeyPoint &point) {
    if (keyPointMap_.contains(curPointInfo_.first)) {
        QList<SKeyPoint> pointList = keyPointMap_.value(curPointInfo_.first);
        if (pointList.size() > curPointInfo_.second) {
            point = pointList.at(curPointInfo_.second);
            return true;
        }
    }
    return false;
}

bool VisualCurvePosManager::insertKeyPoint(int index, std::string curve, SKeyPoint keyPoint) {
    if (keyPointMap_.contains(curve)) {
        QList<SKeyPoint> list = keyPointMap_.value(curve);
        list.insert(index, keyPoint);
        keyPointMap_.remove(curve);
        keyPointMap_.insert(curve, list);
        return true;
    }
    return false;
}

bool VisualCurvePosManager::getKeyPoint(std::string curve, int index, SKeyPoint &point) {
    if (keyPointMap_.contains(curve)) {
        QList<SKeyPoint> pointList = keyPointMap_.value(curve);
        if (pointList.size() > index && index >= 0) {
            point = pointList.at(index);
            return true;
        }
    }
    return false;
}

bool VisualCurvePosManager::getCurKeyPointList(QList<SKeyPoint> &pointList) {
    if (keyPointMap_.contains(curPointInfo_.first)) {
        pointList = keyPointMap_.value(curPointInfo_.first);
        return true;
    }
    return false;
}

bool VisualCurvePosManager::getKeyPointList(std::string curve, QList<SKeyPoint> &points) {
    if (keyPointMap_.contains(curve)) {
        points = keyPointMap_.value(curve);
        return true;
    }
    return false;
}

void VisualCurvePosManager::swapCurKeyPointList(QList<SKeyPoint> pointList) {
    keyPointMap_.remove(curPointInfo_.first);
    keyPointMap_.insert(curPointInfo_.first, pointList);
}

void VisualCurvePosManager::addMultiSelPoint(int index) {
    multiSelPoints_.append(index);
}

bool VisualCurvePosManager::hasMultiSelPoint(int index) {
    return multiSelPoints_.contains(index);
}

void VisualCurvePosManager::delMultiSelPoint(int index) {
    if (multiSelPoints_.contains(index)) {
        multiSelPoints_.removeOne(index);
    }
}

QList<int> VisualCurvePosManager::getMultiSelPoints() {
    return multiSelPoints_;
}

void VisualCurvePosManager::clearMultiSelPoints() {
    multiSelPoints_.clear();
}

void VisualCurvePosManager::replaceCurKeyPoint(SKeyPoint point) {
    auto list = keyPointMap_.value(curPointInfo_.first);
    list.replace(curPointInfo_.second, point);
    keyPointMap_.remove(curPointInfo_.first);
    keyPointMap_.insert(curPointInfo_.first, list);
}
}
