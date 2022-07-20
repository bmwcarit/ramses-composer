#ifndef CURVEMANAGER_H
#define CURVEMANAGER_H

#include "CurveData/CurveData.h"

namespace raco::guiData {

class CurveManager {
public:
    static CurveManager& GetInstance();
    ~CurveManager();
    CurveManager(const CurveManager&) = delete;
    CurveManager& operator=(const CurveManager&) = delete;

    // 添加 Curve
    bool addCurve(Curve* curve);
    // 删除 Curve
    bool delCurve(const std::string& curveNmae);
    // 获取 Curve
    Curve *getCurve(const std::string& curveName);
    // 修改 Curve Name
    bool modifyCurveName(const std::string& curveName, const std::string& modifyName);
    // 复制 Curve
    bool copyCurve(const std::string& curveName);
    // 是否有Curve
    bool hasCurve(const std::string &curveName);
    // 清空Curve
    bool clearCurve();
    // 模糊搜索
    std::list<Curve*> search(const std::string& curveNmae);
    //
    bool getCurveValue(std::string curve, int keyFrame, double &value);
    // 获取 Curve list
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
