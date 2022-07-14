#include "gltf_Animation/GltfAnimationManager.h"


GltfAnimationManager::GltfAnimationManager(raco::core::CommandInterface *commandInterface, QObject *parent)
    : QObject{parent} ,
      commandInterface_(commandInterface) {
    connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigUpdateGltfAnimation, this, &GltfAnimationManager::slotUpdateGltfAnimation);
}

void GltfAnimationManager::commandInterface(raco::core::CommandInterface *commandInterface) {
    commandInterface_ = commandInterface;
}

void GltfAnimationManager::slotUpdateGltfAnimation(const raco::core::ValueHandle &valueHandle) {
    animationChannels_.clear();
    animationNodes_.clear();
    std::string animation;
    for (int i{0}; i < valueHandle.size(); ++i) {
        raco::core::ValueHandle tempHandle = valueHandle[i];
        if (tempHandle.getPropName().compare("objectName") == 0) {
            animation = tempHandle.asString();
        }
        if (tempHandle.getPropName().compare("animationChannels") == 0) {
            if (tempHandle.type() == raco::core::PrimitiveType::Table) {
                for (int j{0}; j < tempHandle.size(); ++j) {
                    raco::core::ValueHandle aniHandle = tempHandle[j];
                    // get animation channel data
                    if (aniHandle.type() == raco::core::PrimitiveType::Ref) {
                        aniHandle = aniHandle.asRef();
                        raco::user_types::AnimationChannel *aniChannel = dynamic_cast<raco::user_types::AnimationChannel *>(aniHandle.rootObject().get());
                        animationChannels_.push_back(aniChannel);
                    }
                }
            }
        }
        if (tempHandle.getPropName().compare("animationOutputs") == 0) {
            if (tempHandle.type() == raco::core::PrimitiveType::Table) {
                for (int j{0}; j < tempHandle.size(); ++j) {
                    raco::core::ValueHandle aniHandle = tempHandle[j];

                    // get link node object ID map by handle; key-path value-object ID
                    auto linkEnds = raco::core::Queries::getLinksConnectedToProperty(*commandInterface_->project(), aniHandle, true, false);
                    std::map<std::string, std::string> sortedLinkEnds;
                    for (const auto& linkEnd : linkEnds) {
                        auto linkDesc = linkEnd->descriptor();
                        sortedLinkEnds[linkDesc.end.getFullPropertyPath()] = linkDesc.end.object()->objectID();
                    }
                    if (sortedLinkEnds.size() > 0) {
                        auto it = sortedLinkEnds.begin();
                        animationNodes_.emplace(it->first, it->second);
                    }
                }
            }
        }
    }
    std::string animationID = valueHandle[0].asString();
    updateGltfAnimation(animation);
    Q_EMIT raco::signal::signalProxy::GetInstance().sigDeleteAniamtionNode(animationID);
}

void GltfAnimationManager::updateGltfAnimation(std::string animation) {
    if (animation.empty()) {
        curAnimation_ = "";
        return;
    }
    curAnimation_ = QString::fromStdString(animation);
    if (animationChannels_.size() == animationNodes_.size()) {
        // create animation
        raco::guiData::animationDataManager::GetInstance().InsertAmimation(animation);
        raco::guiData::animationDataManager::GetInstance().SetActiveAnimation(animation);
        Q_EMIT raco::signal::signalProxy::GetInstance().sigUpdateActiveAnimation_From_AnimationLogic(curAnimation_);
        Q_EMIT raco::signal::signalProxy::GetInstance().sigInitAnimationView();
        //
        auto nodeIt = animationNodes_.begin();
        for (int i{0}; i < animationChannels_.size(); ++i) {
            raco::user_types::AnimationChannel *aniChannel = animationChannels_.at(i);
            std::string path = nodeIt->first;
            QString qstrNode = QString::fromStdString(path).section("/", -1);
            std::string node = qstrNode.split(".").at(0).toStdString();
            std::string property = qstrNode.split(".").at(1).toStdString();

            raco::guiData::NodeData *nodeData = raco::guiData::NodeDataManager::GetInstance().searchNodeByName(node);
            if (nodeData && aniChannel) {
                std::vector<float> keyFrames = aniChannel->currentSamplerData_.get()->input;
                std::vector<std::vector<float>> propertyData = aniChannel->currentSamplerData_.get()->output;
                raco::core::MeshAnimationInterpolation interpolation = aniChannel->currentSamplerData_.get()->interpolation;

                // insert curves
                updateOneGltfCurve(nodeData, keyFrames, propertyData, interpolation, property, node);
            }
            nodeIt++;
        }
    }
    Q_EMIT raco::signal::signalProxy::GetInstance().sigInitCurveView();
    Q_EMIT raco::signal::signalProxy::GetInstance().sigRepaintTimeAixs_From_CurveUI();
}

void GltfAnimationManager::updateOneGltfCurve(raco::guiData::NodeData *nodeData, std::vector<float> keyFrames, std::vector<std::vector<float>> propertyData, raco::core::MeshAnimationInterpolation interpolation, std::string property, std::string node) {
    auto insertBindingItem = [=](std::string prop, std::string curve)->bool {
        std::string animation = curAnimation_.toStdString();
        std::map<std::string, std::string> bindingMap;
        if (nodeData->NodeExtendRef().curveBindingRef().getPropCurve(animation, bindingMap)) {
            auto it = bindingMap.find(prop);
            if (it != bindingMap.end()) {
                return false;
            }
            nodeData->NodeExtendRef().curveBindingRef().insertBindingDataItem(animation, prop, curve);
        } else {
            std::map<std::string, std::string> bindingMap;
            bindingMap.emplace(prop, curve);
            nodeData->NodeExtendRef().curveBindingRef().insertAnimation(animation, bindingMap);
        }
        return true;
    };

    if (keyFrames.empty()) {
        return;
    }

    std::string propX = property + ".x";
    std::string propY = property + ".y";
    std::string propZ = property + ".z";
    std::string curveX = curAnimation_.toStdString() + "_" + node + "." + propX;
    std::string curveY = curAnimation_.toStdString() + "_" + node + "." + propY;
    std::string curveZ = curAnimation_.toStdString() + "_" + node + "." + propZ;

    float lastX{0};
    float lastY{0};
    float lastZ{0};

    int invalidIndexX{0};
    int invalidIndexY{0};
    int invalidIndexZ{0};
    for (int i{0}; i < keyFrames.size(); ++i) {
        int keyFrame = qRound(keyFrames.at(i) * 24);
        std::vector<float> data = propertyData.at(i);

        if (data[0] != lastX || i == 0) {
            insertBindingItem(propX, curveX);
            insertCurve(keyFrame, data[0], curveX, interpolation);
            lastX = data[0];
            invalidIndexX++;
        }
        if (data[1] != lastY || i == 0) {
            insertBindingItem(propY, curveY);
            insertCurve(keyFrame, data[1], curveY, interpolation);
            lastY = data[1];
            invalidIndexY++;
        }
        if (data[2] != lastZ || i == 0) {
            insertBindingItem(propZ, curveZ);
            insertCurve(keyFrame, data[2], curveZ, interpolation);
            lastZ = data[2];
            invalidIndexZ++;
        }
    }
    if (invalidIndexX == 1) {
        nodeData->NodeExtendRef().curveBindingRef().deleteBindingDataItem(curAnimation_.toStdString(), propX, curveX);
        if (raco::guiData::CurveManager::GetInstance().getCurve(curveX)) {
            raco::guiData::CurveManager::GetInstance().delCurve(curveX);
        }
    }
    if (invalidIndexY == 1) {
        nodeData->NodeExtendRef().curveBindingRef().deleteBindingDataItem(curAnimation_.toStdString(), propY, curveY);
        if (raco::guiData::CurveManager::GetInstance().getCurve(curveY)) {
            raco::guiData::CurveManager::GetInstance().delCurve(curveY);
        }
    }
    if (invalidIndexZ == 1) {
        nodeData->NodeExtendRef().curveBindingRef().deleteBindingDataItem(curAnimation_.toStdString(), propZ, curveZ);
        if (raco::guiData::CurveManager::GetInstance().getCurve(curveZ)) {
            raco::guiData::CurveManager::GetInstance().delCurve(curveZ);
        }
    }
}

bool GltfAnimationManager::insertCurve(int keyFrame, float data, std::string curve, raco::core::MeshAnimationInterpolation interpolation) {
    raco::guiData::Curve *curveData = raco::guiData::CurveManager::GetInstance().getCurve(curve);
    if (curveData == nullptr) {
        curveData = new raco::guiData::Curve();
    }
    curveData->setCurveName(curve);
    curveData->setDataType(raco::guiData::EDataType::Type_FLOAT);
    raco::guiData::Point *point = new raco::guiData::Point;
    point->setKeyFrame(keyFrame);
    point->setDataValue(static_cast<double>(data));

    raco::guiData::EInterPolationType type = raco::guiData::EInterPolationType::LINER;

    switch(interpolation) {
    case raco::core::MeshAnimationInterpolation::Linear:{
        type = raco::guiData::EInterPolationType::LINER;
        break;
    }
    case raco::core::MeshAnimationInterpolation::Step:{
        type = raco::guiData::EInterPolationType::STEP;
        break;
    }
    case raco::core::MeshAnimationInterpolation::CubicSpline: {
        type = raco::guiData::EInterPolationType::BESIER_SPLINE;
        break;
    }
    }
    point->setInterPolationType(type);
    curveData->insertPoint(point);
    raco::guiData::CurveManager::GetInstance().addCurve(curveData);

    return true;
}
