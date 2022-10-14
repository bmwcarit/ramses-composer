#include "node_logic/NodeLogic.h"
#include "PropertyData/PropertyType.h"
#include "time_axis/TimeAxisCommon.h"
#include "visual_curve/VisualCurvePosManager.h"
#include <QDebug>

namespace raco::node_logic {
NodeLogic::NodeLogic(raco::core::CommandInterface *commandInterface, QObject *parent)
    : QObject{parent}, commandInterface_{commandInterface} {
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateKeyFram_From_AnimationLogic, this, &NodeLogic::slotUpdateKeyFrame);
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateActiveAnimation_From_AnimationLogic, this, &NodeLogic::slotUpdateActiveAnimation);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &NodeLogic::slotResetNodeData);
    connect(&signalProxy::GetInstance(), &signalProxy::sigValueHandleChanged_From_NodeUI, this, &NodeLogic::slotValueHandleChanged);
}

void NodeLogic::setCommandInterface(core::CommandInterface *commandInterface) {
    commandInterface_ = commandInterface;
}

void NodeLogic::Analyzing(NodeData *pNode) {
	if (!pNode )
		return;
    handleMapMutex_.lock();
	if (pNode->objectID() != "" && pNode->objectID() != "objectID") {
		auto it = nodeObjectIDHandleReMap_.find(pNode->objectID());
		if (it != nodeObjectIDHandleReMap_.end()) {
			raco::core::ValueHandle valueHandle = nodeObjectIDHandleReMap_.find(pNode->objectID())->second;
			initBasicProperty(valueHandle, pNode);
		}
	}
    handleMapMutex_.unlock();

	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		Analyzing(&(it->second));
    }
}

bool NodeLogic::getValueHanlde(std::string property, core::ValueHandle &valueHandle) {

    QString qstrPropety = QString::fromStdString(property);
    QStringList list = qstrPropety.split(".");
	if (!list.isEmpty() && list.contains("uniforms")) {
		property = "materials.material." + property;
		qstrPropety = QString::fromStdString(property);
		list = qstrPropety.split(".");
	}

    auto func = [&](core::ValueHandle &tempHandle, std::string tempProp)->bool {
          if (tempHandle.hasProperty(tempProp)) {
              tempHandle = tempHandle.get(tempProp);
          } else {
              return false;
          }
          return tempHandle.isProperty();
    };

    bool bValid = false;
    if (valueHandle.isObject()) {
        for (int i{0}; i < list.size(); i++) {
            QString str = list[i];
			bValid = func(valueHandle, str.toStdString());
			if (!bValid) {
                i++;
				if (i >= list.size())
                    break;
                str += "." + list[i];
				bValid = func(valueHandle, str.toStdString());
				if (!bValid)
                    break;
            }
        }
    }
	return bValid;
}

void NodeLogic::setProperty(core::ValueHandle handle, std::string property, float value) {
    if (getValueHanlde(property, handle) && commandInterface_) {
        commandInterface_->set(handle, value);
    }
}

std::map<std::string, core::ValueHandle> &NodeLogic::getNodeNameHandleReMap() {
    QMutexLocker locker(&handleMapMutex_);
    return nodeObjectIDHandleReMap_;
}

void NodeLogic::setNodeNameHandleReMap(std::map<std::string, core::ValueHandle> nodeNameHandleReMap) {
    QMutexLocker locker(&handleMapMutex_);
	nodeObjectIDHandleReMap_.clear();
    nodeObjectIDHandleReMap_ = std::move(nodeNameHandleReMap);
}

bool NodeLogic::getHandleFromObjectID(const std::string &objectID, core::ValueHandle &handle) {
    QMutexLocker locker(&handleMapMutex_);
    auto it = nodeObjectIDHandleReMap_.find(objectID);
    if (it == nodeObjectIDHandleReMap_.end()) {
        return false;
    }
    handle = it->second;
    return true;
}

bool NodeLogic::hasHandleFromObjectID(const std::string &objectID) {
    QMutexLocker locker(&handleMapMutex_);
    auto it = nodeObjectIDHandleReMap_.find(objectID);
    if (it == nodeObjectIDHandleReMap_.end()) {
        return false;
    }
    return true;
}

void NodeLogic::AnalyzeHandle() {
	raco::guiData::NodeDataManager &nodeManager = NodeDataManager::GetInstance();

	Analyzing(&(nodeManager.root()));
}

void NodeLogic::initBasicProperty(raco::core::ValueHandle valueHandle, NodeData *node) {
	QString strObjId, strPropName;

	for (int i{0}; i < valueHandle.size(); i++) {
		if (!valueHandle[i].isObject()) {
			raco::core::ValueHandle tempHandle = valueHandle[i];
			QString str = QString::fromStdString(tempHandle.getPropName());
			if (QString::fromStdString(tempHandle.getPropName()).compare("objectID") == 0) {
				strObjId = QString::fromStdString(tempHandle.asString());
				strPropName = QString::fromStdString(tempHandle.getPropertyPath());
			}

			if (QString::fromStdString(tempHandle.getPropName()).compare("translation") == 0) {
				Vec3 trans;
				trans.x = tempHandle.get("x").asDouble();
				trans.y = tempHandle.get("y").asDouble();
				trans.z = tempHandle.get("z").asDouble();
				node->insertSystemData("translation", trans);
			} else if (QString::fromStdString(tempHandle.getPropName()).compare("rotation") == 0) {
				Vec3 rota;
				rota.x = tempHandle.get("x").asDouble();
				rota.y = tempHandle.get("y").asDouble();
				rota.z = tempHandle.get("z").asDouble();
				node->insertSystemData("rotation", rota);

			} else if (QString::fromStdString(tempHandle.getPropName()).compare("scale") == 0) {
				Vec3 scal;
				scal.x = tempHandle.get("x").asDouble();
				scal.y = tempHandle.get("y").asDouble();
				scal.z = tempHandle.get("z").asDouble();
				node->insertSystemData("scale", scal);

			}  
            initBasicProperty(valueHandle[i], node);
		}
    }
}

void NodeLogic::slotUpdateActiveAnimation(QString animation) {
    curAnimation_ = animation;
}

void NodeLogic::preOrderReverse(NodeData *pNode, const int &keyFrame, const std::string &sampleProperty) {
    if (!pNode)
        return;

    if (pNode->getBindingySize() != 0) {
        std::map<std::string, std::string> bindingDataMap;
        pNode->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty, bindingDataMap);

        // TODO SETPROPERTY
        setPropertyByCurveBinding(pNode->objectID(), bindingDataMap, keyFrame);
    }
    for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
        preOrderReverse(&(it->second), keyFrame, sampleProperty);
    }
}

void NodeLogic::setPropertyByCurveBinding(const std::string &objecID, const std::map<std::string, std::string> &map, const int &keyFrame) {
    QMutexLocker locker(&handleMapMutex_);
    auto iter = nodeObjectIDHandleReMap_.find(objecID);
    if (iter != nodeObjectIDHandleReMap_.end()) {
        for (const auto &bindingIt : map) {
            if (CurveManager::GetInstance().getCurve(bindingIt.second)) {
                double value{0};
                EInterPolationType type;
                if (CurveManager::GetInstance().getPointType(bindingIt.second, keyFrame, type)) {
                    if (getKeyValue(bindingIt.second, type, keyFrame, value)) {
                        setProperty(iter->second, bindingIt.first, value);
                    }
                }
            }
        }
    }
}

void NodeLogic::delNodeBindingByCurveName(std::string curveName) {
	NodeDataManager::GetInstance().delCurveBindingByName(curveName);
	Q_EMIT sig_initCurveBindingWidget__NodePro();
}

bool NodeLogic::getKeyValue(std::string curve, EInterPolationType type, int keyFrame, double &value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    switch (type) {
    case EInterPolationType::LINER: {
        CurveManager::GetInstance().getCurveValue(curve, keyFrame, type, value);
        return true;
    }
    case EInterPolationType::BESIER_SPLINE: {
        QList<raco::time_axis::SKeyPoint> pointList;
        if (VisualCurvePosManager::GetInstance().getKeyPointList(curve, pointList)) {
            for (int i{0}; i < pointList.size(); ++i) {
                if (i + 1 < pointList.size()) {
                    raco::time_axis::SKeyPoint pointF1 = pointList[i];
                    raco::time_axis::SKeyPoint pointF2 = pointList[i + 1];
                    if (pointF1.keyFrame < keyFrame && pointF2.keyFrame > keyFrame) {
                        QList<QPointF> srcPoints, destPoints;

                        srcPoints.push_back(QPointF(pointF1.x, pointF1.y));
                        srcPoints.push_back(pointF1.rightPoint);
                        srcPoints.push_back(pointF2.leftPoint);
                        srcPoints.push_back(QPointF(pointF2.x, pointF2.y));

                        time_axis::createNBezierCurve(srcPoints, destPoints, 0.01);

                        // point border value
                        double dIndex = (100.0 / (pointF2.keyFrame - pointF1.keyFrame) * (keyFrame - pointF1.keyFrame)) - 1;
                        int iIndex = dIndex;
                        if (iIndex >= destPoints.size() - 1) {
                            iIndex = destPoints.size() - 1;
                            time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex], value);
                            return true;
                        }
                        if (iIndex < 0) {
                            iIndex = 0;
                        }

                        // point value
                        double offset = dIndex - iIndex;
                        double lastValue, nextValue;
                        time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex], lastValue);
                        time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex + 1], nextValue);
                        value = (nextValue - lastValue) * offset + lastValue;
                        return true;
                    }
                }
            }
            return CurveManager::GetInstance().getCurveValue(curve, keyFrame, EInterPolationType::LINER, value);
        }
        break;
    }
    case EInterPolationType::HERMIT_SPLINE: {
        QList<raco::time_axis::SKeyPoint> pointList;
        if (VisualCurvePosManager::GetInstance().getKeyPointList(curve, pointList)) {
            for (int i{0}; i < pointList.size(); ++i) {
                if (i + 1 < pointList.size()) {
                    raco::time_axis::SKeyPoint pointF1 = pointList[i];
                    raco::time_axis::SKeyPoint pointF2 = pointList[i + 1];
                    if (pointF1.keyFrame < keyFrame && pointF2.keyFrame > keyFrame) {
                        QList<QPointF> srcPoints, destPoints;

                        srcPoints.push_back(QPointF(pointF1.x, pointF1.y));
                        srcPoints.push_back(QPointF(pointF2.x, pointF2.y));
                        srcPoints.push_back(QPointF(pointF1.rightPoint.x() - pointF1.x, pointF1.rightPoint.y() - pointF1.y));
                        srcPoints.push_back(QPointF(pointF2.x - pointF2.leftPoint.x(), pointF2.y - pointF2.leftPoint.y()));

                        time_axis::createHermiteCurve(srcPoints, destPoints, 0.01);

                        // point border value
                        double dIndex = (100.0 / (pointF2.keyFrame - pointF1.keyFrame) * (keyFrame - pointF1.keyFrame)) - 1;
                        int iIndex = dIndex;
                        if (iIndex >= destPoints.size() - 1) {
                            iIndex = destPoints.size() - 1;
                            time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex], value);
                            return true;
                        }
                        if (iIndex < 0) {
                            iIndex = 0;
                        }

                        // point value
                        double offset = dIndex - iIndex;
                        double lastValue, nextValue;
                        time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex], lastValue);
                        time_axis::pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, destPoints[iIndex + 1], nextValue);
                        value = (nextValue - lastValue) * offset + lastValue;
                        return true;
                    }
                }
            }
            return CurveManager::GetInstance().getCurveValue(curve, keyFrame, EInterPolationType::LINER, value);
        }
        break;
    }
    case EInterPolationType::STEP: {
        CurveManager::GetInstance().getCurveValue(curve, keyFrame, type, value);
        return true;
    }
    }

    return false;
}

void NodeLogic::slotUpdateKeyFrame(int keyFrame) {
    preOrderReverse(&NodeDataManager::GetInstance().root(), keyFrame, curAnimation_.toStdString());
}

void NodeLogic::slotResetNodeData() {
    NodeDataManager::GetInstance().clearNodeData();
}

void NodeLogic::slotValueHandleChanged(const core::ValueHandle &handle) {
    core::ValueHandle tempHandle = handle;
	while (tempHandle != NULL) {
		if (tempHandle.hasProperty("objectID")) {
			std::string objectID = tempHandle.get("objectID").asString();
			if (hasHandleFromObjectID(objectID)) {
				NodeData *data = NodeDataManager::GetInstance().searchNodeByID(objectID);
				if (data) {
                    if (tempHandle.hasProperty("objectName")) {
                        std::string nodeName = tempHandle.get("objectName").asString();
                        data->setName(nodeName);
                    }
                    initBasicProperty(tempHandle, data);
					return;
				}
			}
		}
		tempHandle = tempHandle.parent();
    }
}
}
