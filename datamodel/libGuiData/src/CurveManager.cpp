#include "CurveData/CurveManager.h"

namespace raco::guiData {

CurveManager::CurveManager() {

}

CurveManager &CurveManager::GetInstance() {
    // TODO: 在此处插入 return 语句
    static CurveManager Instance;
    return Instance;
}

CurveManager::~CurveManager() {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        delete (*it);
        (*it) = nullptr;
    }
    curveList_.clear();
}

bool CurveManager::addCurve(Curve *curve) {
    if (curve == nullptr) {
        return false;
    }

    std::string curveName = curve->getCurveName();

    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveName) == 0) {
            return false;
        }
        it++;
    }
    curveList_.push_back(curve);
    return true;
}

bool CurveManager::delCurve(const std::string &curveNmae) {
    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveNmae) == 0) {
            it = curveList_.erase(it);
            return true;
        }
        it++;
    }
    return false;
}

std::list<Curve *> CurveManager::search(const std::string &curveNmae) {
    std::list<Curve*> tempCurveList;

    std::regex regex(curveNmae);
    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if (std::regex_search((*it)->getCurveName(), regex)) {
            tempCurveList.push_back((*it));
        }
        it++;
    }

    return tempCurveList;
}

bool CurveManager::getCurveValue(std::string curve, int keyFrame, double &value) {
    if (getCurve(curve)) {
        Curve* tempCurve = getCurve(curve);
        if (tempCurve->getDataValue(keyFrame, value)) {
            return true;
        }
    }
    return false;
}

Curve *CurveManager::getCurve(const std::string &curveName) {
    Curve* curve{nullptr};
    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveName) == 0) {
            curve = (*it);
            break;
        }
        it++;
    }
    return curve;
}

bool CurveManager::modifyCurveName(const std::string &curveName, const std::string &modifyName) {
    Curve* curve{nullptr};

    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveName) == 0) {
            curve = (*it);
        }
        if ((*it)->getCurveName().compare(modifyName) == 0) {
            return false;
        }
        it++;
    }
    if (curve) {
		curve->setCurveName(modifyName);
        return true;
    }
    return false;
}

bool CurveManager::copyCurve(const std::string &curveName) {
    Curve* curve{nullptr};

    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveName) == 0) {
            curve = (*it);
        }
        it++;
    }
    if (curve) {
        Curve* cpCurve = new Curve();
        cpCurve->setCurveName(curveName + "_cp");
        cpCurve->setDataType(curve->getDataType());
        for (auto it : curve->getPointList()) {
            Point *cpPoint = new Point;
            cpPoint->setDataValue(it->getDataValue());
            cpPoint->setKeyFrame(it->getKeyFrame());
            cpPoint->setInterPolationType(it->getInterPolationType());
            cpPoint->setLeftTagent(it->getLeftTagent());
            cpPoint->setRightTagent(it->getRightTagent());
            cpCurve->insertPoint(cpPoint);
        }
        addCurve(cpCurve);
        return true;
    }
    return false;
}

bool CurveManager::hasCurve(const std::string &curveName) {
    auto it = curveList_.begin();
    while (it != curveList_.end()) {
        if ((*it)->getCurveName().compare(curveName) == 0) {
            return true;
        }
        it++;
    }
    return false;
}

bool CurveManager::clearCurve() {
    for (auto it : curveList_) {
        delete it;
        it = nullptr;
    }
    curveList_.clear();
    return true;
}

std::list<Curve *> CurveManager::getCurveList() {
    return curveList_;
}
}
