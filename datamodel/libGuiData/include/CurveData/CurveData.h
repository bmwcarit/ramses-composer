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
    // 设置 left data
    void setLeftData(const std::any &value);
    // 获取 left data
    std::any getLeftData();
    // 设置 left keyframe
    void setLeftKeyFrame(const int keyFrame);
    // 获取 left keyFrame
    int getLeftKeyFrame();
    // 设置 right data
    void setRightData(const std::any &value);
    // 获取 right data
    std::any getRightData();
    // 设置 right keyframe
    void setRightKeyFrame(const int keyFrame);
    // 获取 right keyframe
    int getRightKeyFrame();

private:
    int keyFrame_;
    EInterPolationType interPolationType_{LINER};
    std::any data_{0.0}; //当前只考虑FLOAT type *
    std::any leftTagent_{0.0};
    std::any rightTagent_{0.0};
    std::any leftData_{0.0};
    int leftKeyFrame_{INT_MIN};
    std::any rightData_{0.0};
    int rightKeyFrame_{INT_MIN};
};

class Curve
{
public:
    Curve();
    ~Curve();

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
    //
    bool insertSamePoint(Point* point);
    // 删除Point
    bool delPoint(int keyFrame);
    bool takePoint(int keyFrame);
    bool delSamePoint(int keyFrame);
    // 获取Point list
    std::list<Point*> getPointList();
    // 获取Point
    Point* getPoint(int keyFrame);
    // 排序
    bool sortPoint();
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
    std::string curveName_; // 自定义的curve名称，作为主键索引
    EDataType dataType_{Type_FLOAT};
    std::list<Point*> pointList_;  //一定是有序的 ???用什么数据结构 还是决定用list
};
}

#endif // CURVEDATA_H
