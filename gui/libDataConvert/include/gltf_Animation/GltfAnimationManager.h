#ifndef GLTFANIMATIONMANAGER_H
#define GLTFANIMATIONMANAGER_H

#include <QObject>
#include "signal/SignalProxy.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "core/Queries.h"
#include "NodeData/nodeManager.h"
#include "AnimationData/animationData.h"
#include "CurveData/CurveManager.h"

class GltfAnimationManager : public QObject {
    Q_OBJECT
public:
    explicit GltfAnimationManager(raco::core::CommandInterface* commandInterface, QObject *parent = nullptr);
    void commandInterface(raco::core::CommandInterface* commandInterface);
public Q_SLOTS:
    void slotUpdateGltfAnimation(const raco::core::ValueHandle &valueHandle);
private:
    void updateGltfAnimation(std::string animation);
    void updateOneGltfCurve(raco::guiData::NodeData *nodeData, std::vector<float> keyFrames, std::vector<std::vector<float>> propertyData, raco::core::MeshAnimationInterpolation interpolation, std::string property, std::string node);
    bool insertCurve(int keyFrame, float data, std::string curve, raco::core::MeshAnimationInterpolation interpolation);
private:
    raco::core::CommandInterface* commandInterface_{nullptr};
    std::vector<raco::user_types::AnimationChannel *> animationChannels_;
    std::map<std::string, std::string> animationNodes_;
    QString curAnimation_;
};

#endif // GLTFANIMATIONMANAGER_H
