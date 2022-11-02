#ifndef SIGNAL_H
#define SIGNAL_H

#include "core/CommandInterface.h"
#include "time_axis/TimeAxisCommon.h"
#include <QObject>
#include <QDebug>
#include <set>
namespace raco::signal {
class signalProxy : public QObject
{
    Q_OBJECT
public:
    static signalProxy& GetInstance() {
        static signalProxy Instance;
		qDebug() << "Signal& GetInstance() ";
        return Instance;
    }
    ~signalProxy() {}
    
    signalProxy& operator=(const signalProxy&) = delete;

private:
    explicit signalProxy(QObject *parent = nullptr)
    : QObject{parent}
    {}

Q_SIGNALS:
    // 信号命名方案: sig信号名_from_src
    void sigUpdateKeyFram_From_AnimationLogic(int keyFrame);
    //
    void sigUpdateActiveAnimation_From_AnimationLogic(QString animation);
    //
    void sigResetAnimationProperty_From_AnimationLogic();
    //
    void sigRepaintTimeAxis_From_NodeUI();
    //
    void sigRepaintTimeAixs_From_CurveUI();
    //
    void sigCheckCurveBindingValid_From_CurveUI();
    //
    void sigInsertCurve_From_NodeUI(QString property, QString curve, QVariant value);
    //
    void sigInsertCurve_To_VisualCurve(QString property, QString curve, QVariant value);
    //
    void sigInsertCurveBinding_From_NodeUI(QString property, QString curve);
    //
    void sigInsertKeyFrame_From_NodeUI(QString curve);
    //
    void sigUpdateAnimation_From_AnimationUI();
    //
    void sigUpdateAnimationKey_From_AnimationUI(QString oldKey, QString newKey);
    //
    void sigUpdateCustomProperty_From_PropertyUI();
    //
    void sigSwitchCurrentNode(raco::core::ValueHandle &handle);
    //
    void sigResetAllData_From_MainWindow();
    //
    void sigInitPropertyView();
    //
    void sigInitAnimationView();
    //
    void sigInitCurveView();
    //
    void sigInitPropertyBrowserView();
    //
    void sigValueHandleChanged_From_NodeUI(const raco::core::ValueHandle &handle);
    //
    void sigUpdateGltfAnimation(const std::set<raco::core::ValueHandle> &handles, QString file);
    //
    void sigDeleteAniamtionNode(std::set<std::string> ids);
    //
    void sigSwitchVisualCurve(std::string sampleProp, std::string property, std::string curve);
    //
    void sigUpdatePointTagent();
    //
    void sigRepaintAfterUndoOpreation();
    //
    void sigRepaintPropertyBrowserAfterUndo(raco::core::ValueHandle valueHandle);
    //
    void sigReLoadNodeData();
};

}

#endif // SIGNAL_H
