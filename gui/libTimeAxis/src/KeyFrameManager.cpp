#include "time_axis/KeyFrameManager.h"

namespace raco::time_axis {

KeyFrameManager::KeyFrameManager() {

}

void KeyFrameManager::setCurNodeName(QString nodeName) {
    curNodeName_ = nodeName;
}

QSet<int> KeyFrameManager::getMeshNodeKeyFrameList() {
    return keyFrameMap_.value(curNodeName_);
}

void KeyFrameManager::setClickedFrame(int frame) {
    selFrame_ = frame;
}

int KeyFrameManager::getClickedFrame() {
    return selFrame_;
}

bool KeyFrameManager::createKeyFrame(int keyFrame) {
    QSet<int> keyFrameList = keyFrameMap_.value(curNodeName_);
    if (!keyFrameList.contains(keyFrame)) {
        keyFrameList.insert(keyFrame);
		keyFrameMap_.insert(curNodeName_, keyFrameList);
        return true;
    }

    return false;
}

bool KeyFrameManager::delKeyFrame() {
    int keyFrame = selFrame_;
    QSet<int> keyFrameList;
    if (keyFrameMap_.contains(curNodeName_)) { 
        if (!keyFrameList.contains(keyFrame)) {
            keyFrameList.remove(keyFrame);
            selFrame_ = -1;
			keyFrameMap_.insert(curNodeName_, keyFrameList);
            return true;
        }
    }
    return false;
}

void KeyFrameManager::refreshKeyFrameList(std::map<std::string, std::string> bindingMap) {
    QSet<int> keyFrameList = keyFrameMap_.value(curNodeName_);
    keyFrameList.clear();
    for (const auto &it : bindingMap) {
        std::string curveName = it.second;
        Curve* curve = CurveManager::GetInstance().getCurve(curveName);
        if (curve) {
            std::list<Point*> pointList = curve->getPointList();
            for (Point* point : pointList) {
                int keyFrame = point->getKeyFrame();
                if (!keyFrameList.contains(keyFrame)) {
                    keyFrameList.insert(keyFrame);
                }
            }
        }
    }
    keyFrameMap_.insert(curNodeName_, keyFrameList);
}

void KeyFrameManager::clearKeyFrameList() {
    keyFrameMap_.clear();
}
}
