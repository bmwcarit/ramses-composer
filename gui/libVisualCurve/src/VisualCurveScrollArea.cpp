#include "visual_curve/VisualCurveScrollArea.h"

namespace raco::visualCurve {
VisualCurveHScrollBar::VisualCurveHScrollBar(QWidget *parent) {

}

void VisualCurveHScrollBar::mousePressEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mousePressEvent(event);
}

void VisualCurveHScrollBar::mouseReleaseEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mouseReleaseEvent(event);
}

void VisualCurveHScrollBar::mouseMoveEvent(QMouseEvent *event) {
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

VisualCurveVScrollBar::VisualCurveVScrollBar(QWidget *parent) {

}

void VisualCurveVScrollBar::mousePressEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mousePressEvent(event);
}

void VisualCurveVScrollBar::mouseReleaseEvent(QMouseEvent *event) {
    mousePivot_ = event->pos();
    QScrollBar::mouseReleaseEvent(event);
}

void VisualCurveVScrollBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->MouseButtonPress) {
        if (event->pos().y() < mousePivot_.y()) {
            if (this->value() == 0 && yNum >= 15) {
                yNum = 0;
                Q_EMIT barTopExtend();
            } else if (yNum >= 15){
                yNum = 0;
                Q_EMIT barTopMove();
            } else {
                yNum++;
            }
        }
        else if (event->pos().y() > mousePivot_.y()) {
            if (this->value() == this->maximum() && yNum <= -15) {
                yNum = 0;
                Q_EMIT barButtomExtend();
            } else if (yNum <= -15){
                yNum = 0;
                Q_EMIT barButtomMove();
            }
            else {
                yNum--;
            }
        }
    }
    QScrollBar::mouseMoveEvent(event);
}

VisualCurveScrollArea::VisualCurveScrollArea(QWidget *parent)
    : QAbstractScrollArea{parent}, sceneSize_{viewport()->size()} {
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    hScrollBar_ = new VisualCurveHScrollBar(this);
    vScrollBar_ = new VisualCurveVScrollBar(this);
    connect(hScrollBar_, &VisualCurveHScrollBar::barLeftExtend, this, &VisualCurveScrollArea::barLeftExtend);
    connect(hScrollBar_, &VisualCurveHScrollBar::barRightExtend, this, &VisualCurveScrollArea::barRightExtend);
    connect(hScrollBar_, &VisualCurveHScrollBar::barLeftMove, this, &VisualCurveScrollArea::barLeftMove);
    connect(hScrollBar_, &VisualCurveHScrollBar::barRightMove, this, &VisualCurveScrollArea::barRightMove);
    connect(hScrollBar_, &VisualCurveHScrollBar::barRestore, this, &VisualCurveScrollArea::barRestore);

    connect(vScrollBar_, &VisualCurveVScrollBar::barTopExtend, this, &VisualCurveScrollArea::barTopExtend);
    connect(vScrollBar_, &VisualCurveVScrollBar::barButtomExtend, this, &VisualCurveScrollArea::barButtomExtend);
    connect(vScrollBar_, &VisualCurveVScrollBar::barTopMove, this, &VisualCurveScrollArea::barTopMove);
    connect(vScrollBar_, &VisualCurveVScrollBar::barButtomMove, this, &VisualCurveScrollArea::barButtomMove);
    connect(vScrollBar_, &VisualCurveVScrollBar::barRestore, this, &VisualCurveScrollArea::barRestore);

    setHorizontalScrollBar(hScrollBar_);
    setVerticalScrollBar(vScrollBar_);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &VisualCurveScrollArea::updateViewport);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &VisualCurveScrollArea::updateViewport);
}

void VisualCurveScrollArea::setCenterWidget(QWidget *widget) {
    centerWidget = widget;
}

void VisualCurveScrollArea::barLeftExtend() {
    leftScaleNum_ ++;
    if (hScaleValue_ < 1) {
        hScaleValue_ = 1;
    }
    hScaleValue_ *= 1.005;
    updateHScrollbar(scaledSize().width());
    mouseAction_ = MOUSE_LEFT_EXTEND;
    updateViewport();
}

void VisualCurveScrollArea::barLeftMove() {
    if (rightScaleNum_ > 1) {
        rightScaleNum_--;
        hScaleValue_ /= 1.005;
        updateHScrollbar(scaledSize().width());
        mouseAction_ = MOUSE_LEFT_MOVE;
        updateViewport();
    }
}

void VisualCurveScrollArea::barRightExtend() {
    rightScaleNum_++;
    if (hScaleValue_ < 1) {
        hScaleValue_ = 1;
    }
    hScaleValue_ *= 1.005;
    updateHScrollbar(scaledSize().width());
    mouseAction_ = MOUSE_RIGHT_EXTEND;
    updateViewport();
}

void VisualCurveScrollArea::barRightMove() {
    if (leftScaleNum_ > 1) {
        leftScaleNum_--;
        hScaleValue_ /= 1.005;
        updateHScrollbar(scaledSize().width());
        mouseAction_ = MOUSE_RIGHT_MOVE;
        updateViewport();
    }
}

void VisualCurveScrollArea::barTopExtend() {
    topScaleNum_ ++;
    if (vScaleValue_ < 1) {
        vScaleValue_ = 1;
    }
    vScaleValue_ *= 1.005;
    updateVScrollbar(scaledSize().height());
    mouseAction_ = MOUSE_TOP_EXTEND;
    updateViewport();
}

void VisualCurveScrollArea::barTopMove() {
    if (topScaleNum_ > 1) {
        topScaleNum_--;
        vScaleValue_ /= 1.005;
        updateVScrollbar(scaledSize().height());
        mouseAction_ = MOUSE_TOP_MOVE;
        updateViewport();
    }
}

void VisualCurveScrollArea::barButtomExtend() {
    buttomScaleNum_++;
    if (vScaleValue_ < 1) {
        vScaleValue_ = 1;
    }
    vScaleValue_ *= 1.005;
    updateVScrollbar(scaledSize().height());
    mouseAction_ = MOUSE_BUTTOM_EXTEND;
    updateViewport();
}

void VisualCurveScrollArea::barButtomMove() {
    if (buttomScaleNum_ > 1) {
        buttomScaleNum_--;
        vScaleValue_ /= 1.005;
        updateVScrollbar(scaledSize().height());
        mouseAction_ = MOUSE_BUTTOM_MOVE;
        updateViewport();
    }
}

void VisualCurveScrollArea::barRestore() {
    hScaleValue_ = scaleValueBeforeExtend_;
}

void VisualCurveScrollArea::resizeEvent(QResizeEvent *event) {
    updateViewport();
}

void VisualCurveScrollArea::wheelEvent(QWheelEvent *event) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    mousePivot_ = event->pos();
#else
    mousePivot_ = event->position().toPoint();
#endif
    if (event->angleDelta().y() > 0) {
        mouseAction_ = MOUSE_SCROLL_UP;
    } else {
        mouseAction_ = MOUSE_SCROLL_DOWN;
    }

    scaleValueBeforeExtend_ = hScaleValue_;
    int virtWidth = scaledSize().width();
    double centreX = static_cast<double>(horizontalScrollBar()->value() + mousePivot_.x()) / virtWidth;

    updateHScrollbar(virtWidth);
    horizontalScrollBar()->setValue(centreX * virtWidth - mousePivot_.x());

    int virtHeight = scaledSize().height();
    double centreY = static_cast<double>(verticalScrollBar()->value() + mousePivot_.y()) / virtHeight;

    updateVScrollbar(virtHeight);
    verticalScrollBar()->setValue(centreY * virtHeight - mousePivot_.y());

    updateViewport();
}

void VisualCurveScrollArea::mousePressEvent(QMouseEvent *event) {
    setCursor(Qt::DragMoveCursor);
    mousePivot_ = event->pos();
}

void VisualCurveScrollArea::mouseReleaseEvent(QMouseEvent *event) {
    unsetCursor();
    mousePivot_ = event->pos();
}

void VisualCurveScrollArea::updateViewport() {
    const QSize areaSize = viewport()->size();
    sceneSize_ = areaSize;

    QPoint viewportOffset{horizontalScrollBar()->value(), verticalScrollBar()->value()};

    Q_EMIT viewportRectChanged(sceneSize_, viewportOffset, hScaleValue_, mouseAction_);
    mouseAction_ = MOUSE_NO_ACTION;
}

void VisualCurveScrollArea::updateHScrollbar(int value) noexcept {
    const QSize areaSize = viewport()->size();
    hScrollBar_->setPageStep(areaSize.width());
    hScrollBar_->setRange(0, value - areaSize.width());
}

void VisualCurveScrollArea::updateVScrollbar(int value) noexcept {
    const QSize areaSize = viewport()->size();
    vScrollBar_->setPageStep(areaSize.height());
    vScrollBar_->setRange(0, value - areaSize.height());
}

QSize VisualCurveScrollArea::scaledSize() const noexcept {
    return QSize(sceneSize_.width() * hScaleValue_, sceneSize_.height() * vScaleValue_);
}
}
