#ifndef TIMEAXISWIDGET_H
#define TIMEAXISWIDGET_H

#include <map>
#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QToolBar>
#include <QLineEdit>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/spline.hpp>

#include "TimeAxisCommon.h"
#include "core/CommandInterface.h"
#include "time_axis/KeyFrameManager.h"
#include "node_logic/NodeLogic.h"
#include "AnimationData/animationData.h"

namespace raco::time_axis {

Q_GLOBAL_STATIC_WITH_ARGS(int, numHeight, (20))
Q_GLOBAL_STATIC_WITH_ARGS(double, PI, (3.14159265))

class DragPushButton : public QWidget {
    Q_OBJECT
public:
    explicit DragPushButton(QWidget *parent = nullptr);
    void setText(int num);

Q_SIGNALS:
    void buttonMove(int pix);

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
private:
    QPoint pressPoint_;
    int curText_;
};

class TimeAxisWidget : public QWidget {
    Q_OBJECT
public:
    TimeAxisWidget(QWidget * parent, raco::core::CommandInterface* commandInterface, KeyFrameManager *keyFrameManager);
    void startAnimation();
    void stopAnimation();
    void createKeyFrame();
    void refreshKeyFrameView();
    int getCurrentKeyFrame();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

public Q_SLOTS:
    //选中时间轴区域时，滚动条操作的处理函数
    void setViewportRect(
            const QSize areaSize,
            const QPoint viewportOffset,
            const double scaleValue,
            const MOUSEACTION mouseAction);

    void setStartFrame();
    void setFinishFrame();
    void updateSlider(int pix);    //拖动时间线的处理函数
    void setCurFrameToBegin();
    void setCurFrameToEnd();
    void clearKeyFrames();

Q_SIGNALS:
    void AnimationStop();
    void switchCurveType();
private:
    // 绘制关键帧
    void drawKeyFrame(QPainter &painter);
private:
    QPoint viewportOffset_;
    double scaleValue_{1};
    int intervalLength_;
    int numTextInterval_{10};
    MOUSEACTION mouseAction_{MOUSE_NO_ACTION};
    DragPushButton *button_{nullptr}; //时间轴滑动条
    int timerId_{0};
    int moveNum_{-2}; //时间轴起始,向左扩展变成负值,向右扩展不变

    int curFrame_{0};
    int startFrame_{0};
    int finishFrame_{200};
    int loopCount_{1};
    int curLoop_{0};

    KeyFrameManager* keyFrameMgr_{nullptr};
};
}
#endif // TIMEAXISWIDGET_H
