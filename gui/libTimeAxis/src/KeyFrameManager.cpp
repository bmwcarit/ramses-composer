#include "time_axis/KeyFrameManager.h"

namespace raco::time_axis {

KeyFrameManager::KeyFrameManager() {

}

void KeyFrameManager::setCurAnimation(QString animationName) {
    curAnimation_ = animationName;
}

void KeyFrameManager::setCurNodeName(QString nodeName) {
    curNodeName_ = nodeName;
}

QSet<int> KeyFrameManager::getMeshNodeKeyFrameList() {
    QSet<int> keyFrameList;
    QMap<QString, QSet<int>> curAnimationKeyMap = keyFrameMap_.value(curAnimation_);
    for (auto it : curAnimationKeyMap.toStdMap()) {
        if (it.first.contains(curNodeName_)) {
            for (int key : qAsConst(it.second)) {
                if (!keyFrameList.contains(key)) {
                    keyFrameList.insert(key);
                }
            }
        }
    }
    return keyFrameList;
}

void KeyFrameManager::setClickedFrame(int frame) {
    selFrame_ = frame;
}

int KeyFrameManager::getClickedFrame() {
    return selFrame_;
}

bool KeyFrameManager::createKeyFrame(int keyFrame) {
    QMap<QString, QSet<int>> curAnimationKeyMap = keyFrameMap_.value(curAnimation_);
    QSet<int> keyFrameList = curAnimationKeyMap.value(curNodeName_);
    if (!keyFrameList.contains(keyFrame)) {
        keyFrameList.insert(keyFrame);
        curAnimationKeyMap.insert(curNodeName_, keyFrameList);
        keyFrameMap_.insert(curAnimation_, curAnimationKeyMap);
        return true;
    }

    return false;
}

bool KeyFrameManager::delKeyFrame() {
    int keyFrame = selFrame_;
    QMap<QString, QSet<int>> curAnimationKeyMap;
    QSet<int> keyFrameList;
    if (keyFrameMap_.contains(curAnimation_)) {
        curAnimationKeyMap = keyFrameMap_.value(curAnimation_);
        if (curAnimationKeyMap.contains(curNodeName_)) {
            keyFrameList =  curAnimationKeyMap.value(curNodeName_);
            if (!keyFrameList.contains(keyFrame)) {
                keyFrameList.remove(keyFrame);
                selFrame_ = -1;
                curAnimationKeyMap.insert(curNodeName_, keyFrameList);
                keyFrameMap_.insert(curAnimation_, curAnimationKeyMap);
                return true;
            }
        }
    }
    return false;
}

void KeyFrameManager::refreshKeyFrameList(std::map < std::string, std::map<std::string, std::string>> bindingMap) {
    keyFrameMap_.clear();
    for (const auto &it : bindingMap) {
        QMap<QString, QSet<int>> curAnimationKeyMap;
        for (const auto &bindingIt : it.second) {
            QSet<int> keyFrameList;
            std::string curveName = bindingIt.second;
            Curve* curve = CurveManager::GetInstance().getCurve(curveName);
            if (curve) {
                std::list<Point*> pointList = curve->getPointList();
                for (Point* point : pointList) {
                    int keyFrame = point->getKeyFrame();
                    if (!keyFrameList.contains(keyFrame)) {
                        keyFrameList.insert(keyFrame);
                    }
                }
                curAnimationKeyMap.insert(QString::fromStdString(bindingIt.second), keyFrameList);
            }
        }
        keyFrameMap_.insert(QString::fromStdString(it.first), curAnimationKeyMap);
    }
}

void KeyFrameManager::clearKeyFrameList() {
    keyFrameMap_.clear();
}
}
