#ifndef CURVEMANAGER_H
#define CURVEMANAGER_H

#include "CurveData/CurveData.h"
#include "core/ChangeBase.h"
#include "core/StructCommon.h"

namespace raco::guiData {

class CurveManager {
public:
    static CurveManager& GetInstance();
    ~CurveManager();
    CurveManager(const CurveManager&) = delete;
    CurveManager& operator=(const CurveManager&) = delete;

    std::list<STRUCT_CURVE> convertCurveData();
    void merge(QVariant data);

    // add Curve
    bool addCurve(Curve* curve);
    // take Curve
    bool takeCurve(const std::string& curveNmae);
    // del Curve
    bool delCurve(const std::string& curveNmae);
    // get Curve
    Curve *getCurve(const std::string& curveName);
    // modify Curve Name
    bool modifyCurveName(const std::string& curveName, const std::string& modifyName);
    // copy Curve
    bool copyCurve(const std::string& curveName);
    // has Curve
    bool hasCurve(const std::string &curveName);
    // clear Curve
    bool clearCurve();
    // search
    std::list<Curve*> search(const std::string& curveNmae);
    //
    bool getCurveValue(std::string curve, int keyFrame, EInterPolationType type, double &value);
    //
    bool getPointType(std::string curve, int keyFrame, EInterPolationType &type);
    // get Curve list
    std::list<Curve*> getCurveList();
private:
    CurveManager();

private:
    static CurveManager* curveManager_;
    static std::mutex mutex_;
    std::list<Curve*> curveList_;
};
}

#endif // CURVEMANAGER_H
