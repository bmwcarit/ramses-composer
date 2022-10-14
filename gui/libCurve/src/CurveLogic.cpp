#include "curve/CurveLogic.h"

CurveLogic::CurveLogic(QObject *parent) {
	connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateKeyFram_From_AnimationLogic, this, &CurveLogic::slotUpdateCurrentKey);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInsertCurve_From_NodeUI, this, &CurveLogic::slotInsertCurve);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &CurveLogic::slotResetCurve);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitCurveView, this, &CurveLogic::sigRefreshCurveView);
}

void CurveLogic::setCurrentKeyFrame(int keyFrame) {
    curFrame_ = keyFrame;
}

bool CurveLogic::insertCurve(QString property, QString curve, QVariant value) {
    if (CurveManager::GetInstance().getCurve(curve.toStdString())) {
        Curve* curveData = CurveManager::GetInstance().getCurve(curve.toStdString());
        Point* point = new Point();
        if (value.type() == QVariant::Type::Double) {
            point->setDataValue(value.toDouble());
        }
        point->setKeyFrame(curFrame_);
        curveData->insertPoint(point);
        Q_EMIT signalProxy::GetInstance().sigInsertKeyFrame_From_NodeUI(curve);
        return true;
    } else {
        Curve* curveData = new Curve();
        curveData->setCurveName(curve.toStdString());
        Point* point = new Point();
        if (value.type() == QVariant::Type::Double) {
            point->setDataValue(value.toDouble());
        }
        point->setKeyFrame(curFrame_);
        curveData->insertPoint(point);
        CurveManager::GetInstance().addCurve(curveData);
        Q_EMIT signalProxy::GetInstance().sigInsertCurve_To_VisualCurve(property, curve, value);
        Q_EMIT signalProxy::GetInstance().sigCheckCurveBindingValid_From_CurveUI();
        return true;
    }
    return false;
}

bool CurveLogic::delCurve(QString curve) {
    return CurveManager::GetInstance().takeCurve(curve.toStdString());
}

bool CurveLogic::copyCurve(std::string curve) {
    return CurveManager::GetInstance().copyCurve(curve);
}

std::list<Curve *> CurveLogic::getCurveList() {
    return CurveManager::GetInstance().getCurveList();
}

bool CurveLogic::modifyCurveName(QString curve, QString modifyCurve) {
    return CurveManager::GetInstance().modifyCurveName(curve.toStdString(), modifyCurve.toStdString());
}

void CurveLogic::slotInsertCurve(QString property, QString curve, QVariant value) {
    insertCurve(property, curve, value);
    Q_EMIT sigRefreshCurveView();
}

void CurveLogic::slotUpdateCurrentKey(int keyFrame) {
    setCurrentKeyFrame(keyFrame);
}

void CurveLogic::slotResetCurve() {
    CurveManager::GetInstance().clearCurve();
    Q_EMIT sigRefreshCurveView();
}
