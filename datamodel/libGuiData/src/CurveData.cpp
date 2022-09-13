#include "CurveData/CurveData.h"

namespace raco::guiData {

Point::Point(int keyFrame) {
    keyFrame_ = keyFrame;
}

void Point::setKeyFrame(const int &keyFrame) {
    keyFrame_ = keyFrame;
}

int Point::getKeyFrame() {
    return keyFrame_;
}

void Point::setInterPolationType(const EInterPolationType &interPolationType) {
    interPolationType_ = interPolationType;
}

EInterPolationType Point::getInterPolationType() {
    return interPolationType_;
}

void Point::setDataValue(const std::any &value) {
    data_ = value;
}

std::any Point::getDataValue() {
    return data_;
}

void Point::setLeftTagent(const std::any &value) {
    leftTagent_ = value;
}

std::any Point::getLeftTagent() {
    return leftTagent_;
}

void Point::setRightTagent(const std::any &value) {
    rightTagent_ = value;
}

std::any Point::getRightTagent() {
    return rightTagent_;
}

void Point::setLeftData(const std::any &value) {
    leftData_ = value;
}

std::any Point::getLeftData() {
    return leftData_;
}

void Point::setLeftKeyFrame(const int keyFrame) {
    leftKeyFrame_ = keyFrame;
}

int Point::getLeftKeyFrame() {
    return leftKeyFrame_;
}

void Point::setRightData(const std::any &value) {
    rightData_ = value;
}

std::any Point::getRightData() {
    return rightData_;
}

void Point::setRightKeyFrame(const int keyFrame) {
    rightKeyFrame_ = keyFrame;
}

int Point::getRightKeyFrame() {
    return rightKeyFrame_;
}

Curve::Curve() {

}

Curve::~Curve() {
    for (auto it = pointList_.begin(); it != pointList_.end(); it++) {
        delete (*it);
        (*it) = nullptr;
    }
    pointList_.clear();
}

void Curve::setCurvNodeInfo(const std::string &curveNodeInfo) {
    curveNodeInfo_ = curveNodeInfo;
}

std::string Curve::getCurveNodeInfo() {
    return curveNodeInfo_;
}

void Curve::setCurveName(const std::string &curveName) {
    curveName_ = curveName;
}

std::string Curve::getCurveName() {
    return curveName_;
}

void Curve::setDataType(const EDataType &dataType) {
    dataType_ = dataType;
}

EDataType Curve::getDataType() {
    return dataType_;
}

bool Curve::insertPoint(Point *point) {
    if (point == nullptr) {
        return false;
    }

    int pointKeyFrame = point->getKeyFrame();
    if (!pointList_.empty()) {
        auto it = pointList_.begin();
        while (it != pointList_.end()) {
            if ((*it)->getKeyFrame() == pointKeyFrame) {
                return false;
            }
            if ((*it)->getKeyFrame() > pointKeyFrame) {
                pointList_.insert(it, point);
                return true;
            }
            it++;
        }
    }

    pointList_.push_back(point);
    return true;
}

bool Curve::insertSamePoint(Point *point) {
    if (point == nullptr) {
        return false;
    }

    int pointKeyFrame = point->getKeyFrame();
    if (!pointList_.empty()) {
        auto it = pointList_.begin();
        while (it != pointList_.end()) {
            if ((*it)->getKeyFrame() == pointKeyFrame) {
                pointList_.insert(it, point);
                return true;
            }
            it++;
        }
    }
    return false;
}

bool Curve::delPoint(int keyFrame) {
    if (!pointList_.empty()) {
        auto it = pointList_.begin();
        while (it != pointList_.end()) {
            if ((*it)->getKeyFrame() == keyFrame) {
                pointList_.erase(it);
                return true;
            }
            it++;
        }
    }
    return false;
}

bool Curve::delSamePoint(int keyFrame) {
    if (!pointList_.empty()) {
        auto it = pointList_.begin();
        while (it != pointList_.end()) {
            if ((*it)->getKeyFrame() == keyFrame) {
                it++;
                if (it != pointList_.end()) {
                    pointList_.erase(it);
                }
                return true;
            }
            it++;
        }
    }
    return false;
}

std::list<Point *> Curve::getPointList() {
    return pointList_;
}
Point *Curve::getPoint(int keyFrame) {
    Point* point{nullptr};
    auto it = pointList_.begin();
    while (it != pointList_.end()) {
        if ((*it)->getKeyFrame() == keyFrame) {
            point = (*it);
            break;
        }
        it++;
    }
    return point;
}

bool Curve::sortPoint() {
    pointList_.sort([](Point* a, Point* b)->bool {
                return a->getKeyFrame() < b->getKeyFrame();
            });
    return true;
}

bool Curve::getDataValue(int curFrame, double &value) {
    std::any any;

    auto pointIt = pointList_.begin();
    while (pointIt != pointList_.end()) {
        auto pointItTemp = pointIt;
        pointIt++;
        if (pointIt != pointList_.end()) {
			if (curFrame <= (*pointIt)->getKeyFrame() && curFrame >= (*pointItTemp)->getKeyFrame()) {
				any = calculateLinerValue((*pointItTemp), (*pointIt), curFrame);
				if (any.type() == typeid(double)) {
					value = *any._Cast<double>();
				}
				return true;
            }
        }
        if (pointIt == pointList_.end()) {
			auto pointEnd = pointIt;
			pointEnd--;
			if (curFrame > (*pointEnd)->getKeyFrame()) {
				any = (*pointEnd)->getDataValue();
                if (any.type() == typeid(double)) {
                    value = *any._Cast<double>();
                }
                return true;
            }
        }
        if (pointItTemp == pointList_.begin()) {
            if (curFrame < (*pointItTemp)->getKeyFrame()) {
                any = (*pointItTemp)->getDataValue();
                if (any.type() == typeid(double)) {
                    value = *any._Cast<double>();
                }
                return true;
            }
        }
    }
    return false;
}

bool Curve::getStepValue(int curFrame, double &value) {
    std::any any;

    auto pointIt = pointList_.begin();
    while (pointIt != pointList_.end()) {
        auto pointItTemp = pointIt;
        pointIt++;
        std::any any;
        if (pointIt != pointList_.end()) {
            if (curFrame < (*pointIt)->getKeyFrame()) {
                any = (*pointItTemp)->getDataValue();
            } else if (curFrame == (*pointIt)->getKeyFrame()) {
                any = (*pointIt)->getDataValue();
            }
            if (any.type() == typeid(double)) {
                value = std::any_cast<double>(any);
                return true;
            }
        }
        if (pointIt == pointList_.end()) {
            auto pointEnd = pointIt;
            pointEnd--;
            if (curFrame > (*pointEnd)->getKeyFrame()) {
                any = (*pointEnd)->getDataValue();
                if (any.type() == typeid(double)) {
                    value = *any._Cast<double>();
                }
                return true;
            }
        }
        if (pointItTemp == pointList_.begin()) {
            if (curFrame < (*pointItTemp)->getKeyFrame()) {
                any = (*pointItTemp)->getDataValue();
                if (any.type() == typeid(double)) {
                    value = *any._Cast<double>();
                }
                return true;
            }
        }
    }
    return false;
}

bool Curve::getPointType(int curFrame, EInterPolationType &type) {
    auto pointIt = pointList_.begin();
    while (pointIt != pointList_.end()) {
        auto pointItTemp = pointIt;
        pointIt++;
        if (pointIt != pointList_.end()) {
            if (curFrame <= (*pointIt)->getKeyFrame() && curFrame >= (*pointItTemp)->getKeyFrame()) {
                type = (*pointItTemp)->getInterPolationType();
                return true;
            }
        }
        if (pointIt == pointList_.end()) {
            auto pointEnd = pointIt;
            pointEnd--;
            if (curFrame > (*pointEnd)->getKeyFrame()) {
                type = (*pointEnd)->getInterPolationType();
                return true;
            }
        }
        if (pointItTemp == pointList_.begin()) {
            if (curFrame < (*pointItTemp)->getKeyFrame()) {
                type = (*pointItTemp)->getInterPolationType();
                return true;
            }
        }
    }
    return false;
}

double Curve::calculateLinerValue(Point *firstPoint, Point *secondPoint, double curFrame) {
    if (firstPoint == nullptr || secondPoint == nullptr) {
        return false;
    }

    EInterPolationType type = secondPoint->getInterPolationType();
    double firstPointValue{0};
    double secondPointValue{0};
    if (firstPoint->getDataValue().has_value() && secondPoint->getDataValue().has_value()) {
        if (firstPoint->getDataValue().type() == typeid(double)) {
            firstPointValue = std::any_cast<double>(firstPoint->getDataValue());
            secondPointValue = std::any_cast<double>(secondPoint->getDataValue());
        } else if (firstPoint->getDataValue().type() == typeid(float)) {
            firstPointValue = std::any_cast<float>(firstPoint->getDataValue());
            secondPointValue = std::any_cast<float>(secondPoint->getDataValue());
        }
    }
    double t = (curFrame - firstPoint->getKeyFrame()) / (secondPoint->getKeyFrame() - firstPoint->getKeyFrame()) * (secondPointValue - firstPointValue) + firstPointValue;
    return t;
//    switch (type) {
//    case LINER: {
//        return 0;
//    }
//    case HERMIT_SPLINE: {
//		return 0;
//    }
//    case BESIER_SPLINE: {
//		return 0;
//    }
//	default:
//		return 0;
//    }
}

bool Curve::modifyPointKeyFrame(const int &keyFrame, const int &modifyKeyFrame) {
    Point* point{nullptr};

    if (!pointList_.empty()) {
        auto it = pointList_.begin();
        while (it != pointList_.end()) {
            if ((*it)->getKeyFrame() == keyFrame) {
                point = (*it);
            }
            if ((*it)->getKeyFrame() == modifyKeyFrame) {
                return false;
            }
            it++;
        }
    }
    if (point) {
        pointList_.remove(point);
		point->setKeyFrame(modifyKeyFrame);
        insertPoint(point);
        return true;
    }
    return false;
}
}
