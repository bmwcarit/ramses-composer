#ifndef TIMEAXISCOMMON_H
#define TIMEAXISCOMMON_H

#include "qpoint.h"
#include <QMap>
#include <QList>
#include <any>
#include <string>
#include <vector>
#include <QMetaType>
#include <QVariant>

namespace raco::time_axis {

#define INTERVAL_LENGTH_MIN 40
#define INTERVAL_LENGTH_DEFAULT 50
#define INTERVAL_LENGTH_MAX 80
#define INTERVAL_STEP 5
#define BASE_NUM 5
#define DEFAULT_BUTTON_WIDTH 20

//滚动条操作的选项
enum MOUSEACTION {
    MOUSE_SCROLL_DOWN = 0, //选中时间轴区域时，滚轮向下滚动
    MOUSE_SCROLL_UP = 1, //选中时间轴区域时，滚轮向上滚动
    MOUSE_LEFT_EXTEND = 2, //鼠标点击滚动条，向左扩展
    MOUSE_RIGHT_EXTEND = 3, //鼠标点击滚动条，向右扩展
    MOUSE_LEFT_MOVE = 4, //鼠标点击滚动条，向左移动
    MOUSE_RIGHT_MOVE = 5, //鼠标点击滚动条，向右移动
    MOUSE_TOP_EXTEND = 6, //鼠标点击滚动条，向上扩展
    MOUSE_BUTTOM_EXTEND = 7, //鼠标点击滚动条，向下扩展
    MOUSE_TOP_MOVE = 8, //鼠标点击滚动条，向上移动
    MOUSE_BUTTOM_MOVE = 9, //鼠标点击滚动条，向下移动
    MOUSE_NO_ACTION = 10
};

enum ANIMATIONACTION {
    ANIMATION_START = 0,
    ANIMATION_STOP = 1,
    ANIMATION_PAUSE = 2,
    ANIMATION_END = 3
};

enum MOUSE_PRESS_ACTION {
    MOUSE_PRESS_NONE = 0,
    MOUSE_PRESS_KEY,
    MOUSE_PRESS_LEFT_WORKER_KEY,
    MOUSE_PRESS_RIGHT_WORKER_KEY
};

enum SAME_KEY_TYPE {
    SAME_NONE = -1,
    SAME_WITH_LAST_KEY,
    SAME_WITH_NEXT_KEY
};

enum KEY_BOARD_TYPE {
    POINT_MOVE,
    MULTI_POINT_MOVE,
    CURVE_MOVE,
    CURVE_MOVE_X,
    CURVE_MOVE_Y
};

enum KEY_PRESS_ACT {
    KEY_PRESS_NONE,
    KEY_PRESS_CTRL
};

struct SSelPoint {
    std::string curve;
    std::string property;
    std::string lastCurve;
    int index;
};

struct SKeyPoint {
    int keyFrame{0};
    double x{0};
    double y{0};
    int type{0};
    QPointF leftPoint;
    QPointF rightPoint;

    SKeyPoint(double v1 = 0, double v2 = 0, int t = 0, int key = 0) {
        x = v1;
        y = v2;
        type = t;
        keyFrame = key;
    }
    void setX(double v) {
        x = v;
    }
    void setY(double v) {
        y = v;
    }
    void setKeyFrame(int frame) {
        keyFrame = frame;
    }
    void setType(int t) {
        type = t;
    }
    void setLeftPoint(QPointF point) {
        leftPoint.setX(point.x());
        leftPoint.setY(point.y());
    }
    void setRightPoint(QPointF point) {
        rightPoint.setX(point.x());
        rightPoint.setY(point.y());
    }
};

static void createNBezierCurve(QList<QPointF> src, QList<QPointF> &dest, qreal precision) {
    if (src.size() <= 0) return;

    QList<QPointF>().swap(dest);

    for (qreal t = 0; t < 1.0000; t += precision) {
        int size = src.size();
        QVector<qreal> coefficient(size, 0);
        coefficient[0] = 1.000;
        qreal u1 = 1.0 - t;

        for (int j = 1; j <= size - 1; j++) {
            qreal saved = 0.0;
            for (int k = 0; k < j; k++){
                qreal temp = coefficient[k];
                coefficient[k] = saved + u1 * temp;
                saved = t * temp;
            }
            coefficient[j] = saved;
        }

        QPointF resultPoint;
        for (int i = 0; i < size; i++) {
            QPointF point = src.at(i);
            resultPoint = resultPoint + point * coefficient[i];
        }

        dest.append(resultPoint);
    }
}

static void createHermiteCurve(QList<QPointF> src, QList<QPointF> &dest, qreal precision) {
    if (src.size() <= 0) return;

    QList<QPointF>().swap(dest);

    QPointF pointF0 = src[0];
    QPointF pointF1 = src[1];
    QPointF pointF2 = src[2];
    QPointF pointF3 = src[3];

    double x = pointF0.x();
    double y = pointF0.y();

    for (double t = 0.0; t <= 1.0; t += precision) {
        double t2 = t * t;
        double t3 = t2 * t;
        double express = 3 * t2 - 2 * t3;

        x = ((1 - express) * pointF0.x() + express * pointF1.x() + (t - 2 * t2 + t3) * pointF2.x() + (t3 - t2) * pointF3.x());
        y = ((1 - express) * pointF0.y() + express * pointF1.y() + (t - 2 * t2 + t3) * pointF2.y() + (t3 - t2) * pointF3.y());

        dest.append(QPointF(x, y));
    }
}

static void keyFrame2PointF(int offsetWidth, int offsetHeight, double frameWidth, double valueWidth, int keyFrame, double value, QPointF &pointF) {
    double x = frameWidth * (double)keyFrame - offsetWidth;
    double y = offsetHeight - value * valueWidth;
    pointF.setX(x);
    pointF.setY(y);
}

static void pointF2Value(int offsetWidth, int offsetHeight, double frameWidth, double valueWidth, QPointF pointF, double &value) {
    value = (double)(offsetHeight - pointF.y()) / valueWidth;
}

static void pointF2KeyFrame(int offsetWidth, int offsetHeight, double frameWidth, double valueWidth, QPointF pointF, int &keyFrame) {
    keyFrame = qRound((double)(pointF.x() + offsetWidth) / frameWidth);
}
}

#endif // TIMEAXISCOMMON_H

