#ifndef CURVELOGIC_H
#define CURVELOGIC_H

#include <QObject>
#include <QVariant>
#include "CurveData/CurveManager.h"
#include "NodeData/nodeManager.h"
#include "signal/SignalProxy.h"

using namespace raco::signal;
using namespace raco::guiData;
class CurveLogic : public QObject {
    Q_OBJECT
public:
    explicit CurveLogic(QObject *parent = nullptr);

    void setCurrentKeyFrame(int keyFrame);
    bool insertCurve(QString property, QString curve, QVariant value = 0.0);
    bool delCurve(QString curve);
    std::list<Curve*> getCurveList();
    bool modifyCurveName(QString curve, QString modifyCurve);

Q_SIGNALS:
    void sigRefreshCurveView();

public Q_SLOTS:
    void slotInsertCurve(QString property, QString curve, QVariant value);
    void slotUpdateCurrentKey(int keyFrame);
private:
    int curFrame_{0};
};

#endif // CURVELOGIC_H
