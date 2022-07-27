#ifndef CURVEDATA_H
#define CURVEDATA_H

#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <regex>

namespace raco::guiData {

enum EDataType{
    Type_FLOAT,
    Type_VEC2,
    Type_VEC3,
    Type_VEC4
};

enum EInterPolationType{
    LINER,
    HERMIT_SPLINE,
    BESIER_SPLINE,
    STEP
};

class Point
{
public:
    Point(int keyFrame = 0);

    // 设置KeyFrame 需要设置keyframe时 必须经过Curve的modifyPointKeyframe 判断
    void setKeyFrame(const int& keyFrame);
    // 获取KeyFrame
    int getKeyFrame();
    // 设置曲线类型
    void setInterPolationType(const EInterPolationType& interPolationType);
    // 获取曲线类型
    EInterPolationType getInterPolationType();
    // 设置数据
    void setDataValue(const std::any& value);
    // 获取数据
    std::any getDataValue();
    // 设置left tagent
    void setLeftTagent(const std::any& value);
    // 获取 left tagent
    std::any getLeftTagent();
    // 设置 right tagent
    void setRightTagent(const std::any& value);
    // 获取 right tagent
    std::any getRightTagent();

private:
    int keyFrame_;
    EInterPolationType interPolationType_{LINER};
    std::any data_{0.0}; //当前只考虑FLOAT type *
    std::any leftTagent_{0.0};
    std::any rightTagent_{0.0};
};

class Curve
{
public:
    Curve();
    ~Curve();

    // Curve Node 设置
    void setCurvNodeInfo(const std::string& curveNodeInfo);
    // Curve Node 获取
    std::string getCurveNodeInfo();
    // Curve Name 设置
    void setCurveName(const std::string& curveName);
    // Curve Name 获取
    std::string getCurveName();
    // 设置数据类型
    void setDataType(const EDataType& dataType);
    // 获取数据类型
    EDataType getDataType();
    // 插入Point
    bool insertPoint(Point* point);
    // 删除Point
    bool delPoint(int keyFrame);
    // 获取Point list
    std::list<Point*> getPointList();
    // 获取Point
    Point* getPoint(int keyFrame);
    // 通过当前帧获取对应曲线数据参数
    bool getDataValue(int curFrame, double &value);
    //
    bool getStepValue(int curFrame, double &value);
    //
    bool getPointType(int curFrame, EInterPolationType &type);
    // 根据当前frame计算线的值
    double calculateLinerValue(Point* firstPoint, Point* secondPoint, double curFrame);
    // 修改Point keyframe
    bool modifyPointKeyFrame(const int& keyFrame, const int& modifyKeyFrame);

private:
    std::string curveNodeInfo_; // 仅代表当前curve对应的节点信息，例如 “FL_Glass.translation.x”
    std::string curveName_; // 自定义的curve名称，作为主键索引
    EDataType dataType_{Type_FLOAT};
    std::list<Point*> pointList_;  //一定是有序的 ???用什么数据结构 还是决定用list
};
}

#endif // CURVEDATA_H
