#ifndef TIMEAXISSCROLLAREA_H
#define TIMEAXISSCROLLAREA_H

#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QMouseEvent>
#include "log_system/log.h"

#include "TimeAxisCommon.h"
#include "time_axis/TimeAxisWidget.h"

namespace raco::time_axis{

class TimeAxisScrollBar : public QScrollBar {
    Q_OBJECT
public:
    TimeAxisScrollBar(QWidget *parent);

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

class TimeAxisScrollArea : public QAbstractScrollArea {
    Q_OBJECT
public:
    TimeAxisScrollArea(QWidget* parent = nullptr);
    void setCenterWidget(QWidget *widget);

public Q_SLOTS:
    void barLeftExtend();
    void barLeftMove();
    void barRightExtend();
    void barRightMove();
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
    void updateScrollbar(int value) noexcept;
    QSize scaledSize() const noexcept;

private:
    QSize sceneSize_;
    QPoint mousePivot_{0, 0};
    double scaleValue_{1.0};
    int leftScaleNum_{1};
    int rightScaleNum_{1};
    double scaleValueBeforeExtend_{1.0};
    MOUSEACTION mouseAction_{MOUSE_NO_ACTION};
    TimeAxisScrollBar *hScrollBar_;
    QWidget *centerWidget;
};
}
#endif // TIMEAXISSCROLLAREA_H
