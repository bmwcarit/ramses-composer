#include "time_axis/TimeAxisScrollArea.h"
#include <QScrollBar>
namespace raco::time_axis {

TimeAxisScrollBar::TimeAxisScrollBar(QWidget *parent) : QScrollBar(parent) {
}

void TimeAxisScrollBar::mousePressEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mousePressEvent(event);
}

void TimeAxisScrollBar::mouseReleaseEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mouseReleaseEvent(event);
}

void TimeAxisScrollBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->MouseButtonPress) {
        if (event->pos().x() < mousePivot_.x()) {
            if (this->value() == 0 && leftNum_ >= 3) {
                leftNum_ = 0;
                Q_EMIT barLeftExtend();
            } else if (leftNum_ >= 3){
                leftNum_ = 0;
                Q_EMIT barLeftMove();
            } else {
                leftNum_++;
            }
        }
        else if (event->pos().x() > mousePivot_.x()) {
            if (this->value() == this->maximum() && rightNum_ >= 3) {
                rightNum_ = 0;
                Q_EMIT barRightExtend();
            } else if (rightNum_ >= 3){
                rightNum_ = 0;
                Q_EMIT barRightMove();
            }
            else {
                rightNum_++;
            }

        }
    }

    QScrollBar::mouseMoveEvent(event);
}

TimeAxisScrollArea::TimeAxisScrollArea(QWidget* parent)
    : QAbstractScrollArea{parent}, sceneSize_{viewport()->size()} {
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    hScrollBar_ = new TimeAxisScrollBar(this);
    connect(hScrollBar_, &TimeAxisScrollBar::barLeftExtend, this, &TimeAxisScrollArea::barLeftExtend);
    connect(hScrollBar_, &TimeAxisScrollBar::barRightExtend, this, &TimeAxisScrollArea::barRightExtend);
    connect(hScrollBar_, &TimeAxisScrollBar::barLeftMove, this, &TimeAxisScrollArea::barLeftMove);
    connect(hScrollBar_, &TimeAxisScrollBar::barRightMove, this, &TimeAxisScrollArea::barRightMove);

    connect(hScrollBar_, &TimeAxisScrollBar::barRestore, this, &TimeAxisScrollArea::barRestore);

    setHorizontalScrollBar(hScrollBar_);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &TimeAxisScrollArea::updateViewport);
}

void TimeAxisScrollArea::setCenterWidget(TimeAxisWidget *widget)
{
    centerWidget = widget;
}

void TimeAxisScrollArea::barLeftExtend() {
    leftScaleNum_ ++;
    if (scaleValue_ < 1) {
        scaleValue_ = 1;
    }
    scaleValue_ *= 1.005;
    updateScrollbar(scaledSize().width());
    mouseAction_ = MOUSE_LEFT_EXTEND;
    updateViewport();
}

void TimeAxisScrollArea::barLeftMove() {
    if (rightScaleNum_ > 1) {
        rightScaleNum_--;
        scaleValue_ /= 1.005;
        updateScrollbar(scaledSize().width());
        mouseAction_ = MOUSE_LEFT_MOVE;
        updateViewport();
    }
}

void TimeAxisScrollArea::barRightExtend() {
    rightScaleNum_++;
    if (scaleValue_ < 1) {
        scaleValue_ = 1;
    }
    scaleValue_ *= 1.005;
    updateScrollbar(scaledSize().width());
    mouseAction_ = MOUSE_RIGHT_EXTEND;
    updateViewport();
}

void TimeAxisScrollArea::barRightMove() {
    if (leftScaleNum_ > 1) {
        leftScaleNum_--;
        scaleValue_ /= 1.005;
        updateScrollbar(scaledSize().width());
        mouseAction_ = MOUSE_RIGHT_MOVE;
        updateViewport();
    }
}

void TimeAxisScrollArea::barRestore(){
    scaleValue_ = scaleValueBeforeExtend_;
}

void TimeAxisScrollArea::resizeEvent(QResizeEvent *event) {
    updateViewport();
}

void TimeAxisScrollArea::wheelEvent(QWheelEvent *event) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    mousePivot_ = event->pos();
#else
    mousePivot_ = event->position().toPoint();
#endif
    if (event->angleDelta().y() > 0) {
        scaleValue_ *= 1.1;
        mouseAction_ = MOUSE_SCROLL_UP;
    } else {
        scaleValue_ *= 0.9;
        mouseAction_ = MOUSE_SCROLL_DOWN;
    }

    scaleValueBeforeExtend_ = scaleValue_;
    int virtWidth = scaledSize().width();
    double centreX = static_cast<double>(horizontalScrollBar()->value() + mousePivot_.x()) / virtWidth;

    updateScrollbar(virtWidth);
    horizontalScrollBar()->setValue(centreX * virtWidth - mousePivot_.x());

    updateViewport();
}

void TimeAxisScrollArea::mousePressEvent(QMouseEvent *event) {
    setCursor(Qt::DragMoveCursor);
    mousePivot_ = event->pos();
}

void TimeAxisScrollArea::mouseReleaseEvent(QMouseEvent *event) {
    unsetCursor();
    mousePivot_ = event->pos();
}

void TimeAxisScrollArea::updateViewport() {
    const QSize areaSize = viewport()->size();
    sceneSize_ = areaSize;

    QPoint viewportOffset{horizontalScrollBar()->value(), verticalScrollBar()->value()};

    Q_EMIT viewportRectChanged(sceneSize_, viewportOffset, scaleValue_, mouseAction_);
    mouseAction_ = MOUSE_NO_ACTION;
}

void TimeAxisScrollArea::updateScrollbar(int widgetWidth) noexcept {
    const QSize areaSize = viewport()->size();
    hScrollBar_->setPageStep(areaSize.width());
    hScrollBar_->setRange(0, widgetWidth - areaSize.width());
}


QSize TimeAxisScrollArea::scaledSize() const noexcept {
    return sceneSize_ * scaleValue_;
}




}
