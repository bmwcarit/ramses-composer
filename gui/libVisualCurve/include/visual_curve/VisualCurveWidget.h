#ifndef VisualCurveWidget_H
#define VisualCurveWidget_H

#include <QWidget>
#include <QPainterPath>
#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QMenu>
#include "time_axis/TimeAxisCommon.h"
#include "time_axis/TimeAxisWidget.h"

using namespace raco::time_axis;
namespace raco::visualCurve {

double calculateTrigLen(double x, double y);

class VisualCurveWidget : public QWidget {
    Q_OBJECT
public:
    VisualCurveWidget(QWidget * parent, raco::core::CommandInterface* commandInterface);
    void startAnimation();
    void stopAnimation();
    void createKeyFrame(QString curveName);
    void refreshKeyFrameView();
    int getCurrentKeyFrame();
    std::map<std::string, std::string> getBindingMap();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

public Q_SLOTS:
    void setViewportRect(
            const QSize areaSize,
            const QPoint viewportOffset,
            const double scaleValue,
            const MOUSEACTION mouseAction);

    void slotSetStartFrame();
    void slotSetFinishFrame();
    void slotUpdateSlider(int pix);
    void slotSetCurFrameToBegin();
    void slotSetCurFrameToEnd();
    void slotClearKeyFrames();
    void slotShowContextMenu(const QPoint &p);
    void slotSwitchPoint2Liner();
    void slotSwitchPoint2Hermite();
    void slotSwitchPoint2Bezier();
    void slotSwitchPoint2Step();
    void slotDeleteKeyFrame();
    void slotRefreshVisualCurve();
    void slotRefreshCursorX();
    void slotInsertCurve(QString property, QString curveName, QVariant value);

Q_SIGNALS:
    void AnimationStop();
    void sigSwitchCurveType();
    void sigPressKey();
    void sigUpdateSelKey();
    void sigUpdateCursorX();
    void sigDeleteCurve(std::string curve);
private:
    // paint keyframe
    void drawKeyFrame();
    // paint liner
    void drawLiner(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint);
    // paint bezier
    void drawBezier(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint);
    // paint hermite
    void drawHermite(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint);
    // paint bezier convert hermite
    void drawBezier2Hermite(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint);
    // paint step
    void drawStep(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint);
    // paint worker point
    void drawWorkerPoint(QPainter &painter, SKeyPoint point, QPair<QPointF, QPointF> workerPoint, int index, std::string curve);
    // update keyframe & bezier points
    void updateCurvePoint();
    // recaculate curve point
    void reCaculateCurvePoint(int x, int y, double frameWidth, double valueWidth);
    // switch point interpolation
    void switchPointInterpolationType(EInterPolationType type);
    // recaculate point pos
    QPointF reCaculatePoint(QPointF point, int x, int y, double frameWidth, double valueWidth);
    // point move
    void pointMove(QMouseEvent *event);
    // curve move
    void curveMove(QMouseEvent *event);
    // multi point move
    void multiPointMove(QMouseEvent *event);
private:
    QPoint viewportOffset_;
    int intervalLength_;
    int numTextIntervalX_{10};
    double numTextIntervalY_{2};
    MOUSEACTION mouseAction_{MOUSE_NO_ACTION};
    DragPushButton *button_{nullptr};
    int timerId_{0};
    double moveNumX_{-4};
    double moveNumY_{0};
    int offsetX_{0};
    int offsetY_{0};

    int curFrame_{0};
    int startFrame_{0};
    int finishFrame_{200};
    int loopCount_{1};
    int curLoop_{0};

    QMutex pressDragBtnMutex_;
    bool isPressDragBtn_{false};

    std::map<std::string, std::string> bindingMap_;
private:
    QMenu *menu_{nullptr};
    QMenu *interpolationMenu{nullptr};
    QAction *deleteKeyFrameAct_{nullptr};
    QAction *linerAct_{nullptr};
    QAction *hermiteAct_{nullptr};
    QAction *bezierAct_{nullptr};
    QAction *stepAct_{nullptr};
};
}
#endif // VisualCurveWidget_H
