#ifndef KeyFrameManagerEx_H
#define KeyFrameManagerEx_H

#include <QObject>
#include <QMap>
#include <set>
#include <QSet>
#include "CurveData/CurveManager.h"
#include "NodeData/nodeManager.h"
#include "qmutex.h"


using namespace raco::guiData;
namespace raco::time_axis {

class KeyFrameManager {
public:
    explicit KeyFrameManager();
    // 设置当前Node名称
    void setCurNodeName(QString nodeName);
    // 获取当前node keyframe列表
    QSet<int> getMeshNodeKeyFrameList();
    // 点击关键帧
    void setClickedFrame(int frame);
    // 获取点击关键帧
    int getClickedFrame();
    // 创建 keyframe
    bool createKeyFrame(int keyFrame);
    // 删除 keyframe
    bool delKeyFrame();
    // 刷新 keyframe list
    void refreshKeyFrameList(std::map<std::string, std::string> bindingMap);

private:
    QMap<QString, QSet<int>> keyFrameMap_;
    int selFrame_{-1};
    QString curNodeName_;
};
}

#endif // KeyFrameManagerEx_H
