#ifndef VISUALCURVESCROLLAREA_H
#define VISUALCURVESCROLLAREA_H

#include <QObject>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QMouseEvent>
#include <QDebug>
#include "log_system/log.h"
#include "time_axis/TimeAxisCommon.h"

using namespace raco::time_axis;
namespace raco::visualCurve{
class VisualCurveHScrollBar : public QScrollBar {
    Q_OBJECT
public:
    VisualCurveHScrollBar(QWidget *parent);

Q_SIGNALS:
    void barLeftExtend();
    void barRightExtend();
    void barLeftMove();
    void barRightMove();
    void barRestore();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint mousePivot_{0, 0};
    int leftNum_{0};
    int rightNum_{0};
};

class VisualCurveVScrollBar : public QScrollBar {
    Q_OBJECT
public:
    VisualCurveVScrollBar(QWidget *parent);

Q_SIGNALS:
    void barTopExtend();
    void barButtomExtend();
    void barTopMove();
    void barButtomMove();
    void barRestore();
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint mousePivot_{0, 0};
    int yNum{0};
};

class VisualCurveScrollArea : public QAbstractScrollArea {
    Q_OBJECT
public:
    VisualCurveScrollArea(QWidget *parent = nullptr);
    void setCenterWidget(QWidget *widget);

public Q_SLOTS:
    void barLeftExtend();
    void barLeftMove();
    void barRightExtend();
    void barRightMove();
    void barTopExtend();
    void barTopMove();
    void barButtomExtend();
    void barButtomMove();
    void barRestore();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

Q_SIGNALS:
    void getValueHandls();
    void viewportRectChanged(
            const QSize areaSize,
            const QPoint viewportOffset,
            const double scaleValue_,
            const MOUSEACTION mouseAction) const;

private:
    void updateViewport();
    void updateHScrollbar(int value) noexcept;
    void updateVScrollbar(int value) noexcept;
    QSize scaledSize() const noexcept;
private:
    QSize sceneSize_;
    QPoint mousePivot_{0, 0};
    double hScaleValue_{1.0};
    double vScaleValue_{1.0};
    double curVScaleValue_{1.0};
    int leftScaleNum_{1};
    int rightScaleNum_{1};
    int topScaleNum_{1};
    int buttomScaleNum_{1};
    double scaleValueBeforeExtend_{1.0};
    MOUSEACTION mouseAction_{MOUSE_NO_ACTION};
    VisualCurveHScrollBar *hScrollBar_;
    VisualCurveVScrollBar *vScrollBar_;
    QWidget *centerWidget;
};
}

#endif // VISUALCURVESCROLLAREA_H
