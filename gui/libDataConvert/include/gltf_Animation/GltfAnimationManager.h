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

#define SYMBOL_POINT        std::string(".")
#define SYMBOL_UNDERLINE    std::string("_")
#define PROP_X              std::string(".x")
#define PROP_Y              std::string(".y")
#define PROP_Z              std::string(".z")
#define GLTF_OBJECT_NAME            std::string("objectName")
#define GLTF_ANIMATION_CHANNELS     std::string("animationChannels")
#define GLTF_ANIMATION_OUTPUTS      std::string("animationOutputs")

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
    std::vector<std::string> animationNodes_;
    QString curAnimation_;
};

#endif // GLTFANIMATIONMANAGER_H
