#include "visual_curve/VisualCurveWidget.h"

#include <QRect>
#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QDebug>
#include "math.h"
#include "common_editors/Int64Editor.h"
#include "VisualCurveData/VisualCurvePosManager.h"
#include "time_axis/TimeAxisScrollArea.h"
#include "signal/SignalProxy.h"
#include "core/Undo.h"
#include "FolderData/FolderDataManager.h"

namespace raco::visualCurve {

Q_GLOBAL_STATIC_WITH_ARGS(int, numHeight, (20))
#define PI (3.14159265358979323846)

double calculateTrigLen(double x, double y) {
    return sqrt(x * x + y * y);
}

VisualCurveWidget::VisualCurveWidget(QWidget *parent, raco::core::CommandInterface* commandInterface) :
    QWidget(parent),
    intervalLength_(INTERVAL_LENGTH_DEFAULT),
    commandInterface_(commandInterface) {

    button_ = new DragPushButton(this);
    button_->resize(DEFAULT_BUTTON_WIDTH, this->height());
    button_->setText(0);

    viewportOffset_.setX(moveNumX_ * intervalLength_);
    viewportOffset_.setY(this->height() / 2);
    button_->move(-viewportOffset_.x() - button_->width()/2, 0);

    connect(button_, &DragPushButton::buttonMove, this, &VisualCurveWidget::slotUpdateSlider);
    connect(button_, &DragPushButton::buttonMoveRelease, this, &VisualCurveWidget::slotFinishSlider);
    setFocusPolicy(Qt::StrongFocus);

    menu_ = new QMenu(this);
    deleteKeyFrameAct_ = new QAction("Delete Keyframe",this);
    connect(deleteKeyFrameAct_, &QAction::triggered, this, &VisualCurveWidget::slotDeleteKeyFrame);

    interpolationMenu = new QMenu("Interpolation Mode", this);
    menu_->addMenu(interpolationMenu);
    linerAct_ = new QAction("Liner", this);
    hermiteAct_ = new QAction("Hermite", this);
    bezierAct_ = new QAction("Bezier", this);
    stepAct_ = new QAction("Step", this);
    connect(linerAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Liner);
    connect(hermiteAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Hermite);
    connect(bezierAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Bezier);
    connect(stepAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Step);

    menu_->addAction(deleteKeyFrameAct_);
    interpolationMenu->addAction(linerAct_);
    interpolationMenu->addAction(hermiteAct_);
    interpolationMenu->addAction(bezierAct_);
    interpolationMenu->addAction(stepAct_);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualCurveWidget::customContextMenuRequested, this, &VisualCurveWidget::slotShowContextMenu);
    QObject::connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigInsertCurve_To_VisualCurve, this, &VisualCurveWidget::slotInsertCurve);
    QObject::connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigRepaintAfterUndoOpreation, this, &VisualCurveWidget::slotRefreshVisualCurveAfterUndo);

    initVisualCurvePos();
}

void VisualCurveWidget::refreshKeyFrameView() {
    bindingMap_ = getBindingMap();
    updateCurvePoint();
    update();
}

int VisualCurveWidget::getCurrentKeyFrame() {
    return VisualCurvePosManager::GetInstance().getCurFrame();
}

void VisualCurveWidget::insertKeyFrame() {
    int curFrame_ = VisualCurvePosManager::GetInstance().getCurFrame();
    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    std::string curveName = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    if (CurveManager::GetInstance().getCurve(curveName)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curveName);
        if (!curve->getPoint(curFrame_)) {
            Point *point = new Point;
            point->setKeyFrame(curFrame_);
            double value{0.0};
            curve->getDataValue(curFrame_, value);
            point->setDataValue(value);
            curve->insertPoint(point);

            QPointF pointF;
            keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, curFrame_, value, pointF);
            SKeyPoint keyPoint(pointF.x(), pointF.y(), 0, curFrame_);

            double offsetWorkerLen = 10 * eachFrameWidth;
            QPointF leftPoint, rightPoint;
            leftPoint.setX(pointF.x() - offsetWorkerLen);
            leftPoint.setY(pointF.y());
            rightPoint.setX(pointF.x() + offsetWorkerLen);
            rightPoint.setY(pointF.y());
            keyPoint.setLeftPoint(leftPoint);
            keyPoint.setRightPoint(rightPoint);

            QList<SKeyPoint> keyPoints;
            VisualCurvePosManager::GetInstance().getKeyPointList(curveName, keyPoints);
            int index{keyPoints.size()};
            for (int i{0}; i < keyPoints.   size(); i++) {
                if (keyPoints.at(i).keyFrame > curFrame_) {
                    index = i;
                    break;
                }
            }

            if (index <= VisualCurvePosManager::GetInstance().getCurrentPointInfo().second) {
                VisualCurvePosManager::GetInstance().setCurrentPointInfo(VisualCurvePosManager::GetInstance().getCurrentPointInfo().second + 1);
            }

            VisualCurvePosManager::GetInstance().insertKeyPoint(index, curveName, keyPoint);
            pushState2UndoStack(fmt::format("insert point to '{}', '{}' keyframe", curveName, curFrame_));
            update();
        }
    }
}

std::map<std::string, std::string> VisualCurveWidget::getBindingMap() {
    std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
    NodeData* nodeData = NodeDataManager::GetInstance().getActiveNode();
    std::map<std::string, std::string> bindingMap;
    if (nodeData) {
        nodeData->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty, bindingMap);
    }
    return bindingMap;
}

void VisualCurveWidget::paintEvent(QPaintEvent *event) {
    int width = this->width();
    int height = this->height();

    int numX = width/intervalLength_;
    int curX = viewportOffset_.x();

    int curY = viewportOffset_.y();
    int numYUp = curY / intervalLength_;
    int numYDown = (height - curY) / intervalLength_;

    QPainter painter(this);
    painter.fillRect(QRect(QPoint(0, 0), QPoint(width, *numHeight)), QColor(43, 43, 43, 255));
    QFont font("Helvetica", 8, QFont::Bold, false);
    painter.setFont(font);

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();
    double animationFrameLength = (finishFrame_ - startFrame_) * eachFrameWidth;
    double animationLeft = startFrame_ * eachFrameWidth - curX;
    double animationRight = finishFrame_ * eachFrameWidth - curX;
    // paint frame rect
    if (curX < animationFrameLength) {
        painter.fillRect(QRect(QPoint(animationLeft, *numHeight), QPoint(animationRight, height)), QColor(66, 66, 66, 255));
        painter.setPen(QColor(0, 0, 0, 255));
        painter.drawLine(QPoint(animationLeft, *numHeight), QPoint(animationLeft, height));
        painter.drawLine(QPoint(animationRight, *numHeight), QPoint(animationRight, height));
    }

    painter.setPen(QColor(188, 188, 188, 255));
    double offsetNum =(moveNumX_ - (int)moveNumX_);
    offsetNum = (offsetNum < 0) ? (-1 - offsetNum) : (1 - offsetNum);

    for (int count = 0; count <= numX; count++) {
        int curNumber = qRound((count+moveNumX_+offsetNum)*numTextIntervalX_);
        int leftMovePix = 0;
        if (curNumber > 0) {
            leftMovePix = 5;
        } else if (curNumber > 100) {
            leftMovePix = 9;
        } else if (curNumber > 1000) {
            leftMovePix = 13;
        }
        painter.drawText((count+offsetNum)*intervalLength_ - leftMovePix, *numHeight - 5, QString::number(curNumber));
    }

    // paint text y up
    for (int count = 1; count <= numYUp; count++) {
        double curNumber = count * numTextIntervalY_;
        int leftMovePix = 5;
        painter.drawText(leftMovePix, curY - count*intervalLength_ + leftMovePix, QString::number(curNumber));
    }
    // paint text y down
    for (int count = 1; count <= numYDown; count++) {
        double curNumber = -count * numTextIntervalY_;
        int leftMovePix = 5;
        painter.drawText(leftMovePix, curY + count*intervalLength_ + leftMovePix, QString::number(curNumber));
    }
    // paint centre text value
    painter.drawText(5, curY + 5, QString::number(0));

    QPen pen(QColor(41, 41, 41, 255), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen centerPen(QColor(54,90,143,255), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    centerPen.setWidth(2);
    painter.setPen(pen);

    // paint x line
    for (int count = 0; count <= numX; count++) {
        painter.drawLine((count+offsetNum)*intervalLength_, 20, (count+offsetNum)*intervalLength_, height);
    }
    // paint line y up
    for (int count = 0; count <= numYUp; count++) {
        painter.drawLine(0, curY - count*intervalLength_, width, curY - count*intervalLength_);
    }
    // paint line y down
    for (int count = 1; count <= numYDown; count++) {
        painter.drawLine(0, curY + count*intervalLength_, width, curY + count*intervalLength_);
    }

    // paint centre line
    if (VisualCurvePosManager::GetInstance().getCursorShow()) {
        painter.setPen(centerPen);
    }
    double value = VisualCurvePosManager::GetInstance().getCenterLinePos();
    QPointF pointF;
    keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, 0, value, pointF);
    painter.drawLine(0, pointF.y(), width, pointF.y());
    painter.setPen(pen);

    drawKeyFrame(painter);
    painter.end();
    mouseAction_ = MOUSE_NO_ACTION;
    QWidget::paintEvent(event);
}

void VisualCurveWidget::resizeEvent(QResizeEvent *event) {
    button_->resize(button_->size().width(), event->size().height());
    QWidget::resizeEvent(event);
}

void VisualCurveWidget::timerEvent(QTimerEvent *event) {
    int curFrame_ = VisualCurvePosManager::GetInstance().getCurFrame();
    if (curLoop_ >= loopCount_) {
        stopAnimation();
        Q_EMIT sigAnimationStop();
        return;
    }

    if (curFrame_ > finishFrame_) {
        curLoop_++;
        curFrame_ = 0;
        button_->setText(curFrame_);
        button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
        update();
        VisualCurvePosManager::GetInstance().setCurFrame(curFrame_);
        Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
        Q_EMIT sigUpdateCursorX();
        return;
    }

    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);

    curFrame_++;
    update();
    VisualCurvePosManager::GetInstance().setCurFrame(curFrame_);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
    Q_EMIT sigUpdateCursorX();
}

void VisualCurveWidget::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        double xPos = event->pos().x();
        double yPos = event->pos().y();

        SKeyPoint keyPoint;
        if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
            QPointF pointRight = keyPoint.rightPoint;
            QPointF pointLeft = keyPoint.leftPoint;
            if (abs(pointRight.x() - xPos) <= 3 && abs(pointRight.y() - yPos) <= 3) {
                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_RIGHT_WORKER_KEY);
                VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                Q_EMIT sigPressKey();
                pushState2UndoStack(fmt::format("select point from {}, '{}' right worker point",
                                                VisualCurvePosManager::GetInstance().getCurrentPointInfo().first,
                                                VisualCurvePosManager::GetInstance().getCurrentPointInfo().second));
                update();
                return;
            } else if (abs(pointLeft.x() - xPos) <= 3 && abs(pointLeft.y() - yPos) <= 3) {
                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_LEFT_WORKER_KEY);
                VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                Q_EMIT sigPressKey();
                pushState2UndoStack(fmt::format("select point from {}, '{}' left worker point",
                                                VisualCurvePosManager::GetInstance().getCurrentPointInfo().first,
                                                VisualCurvePosManager::GetInstance().getCurrentPointInfo().second));
                update();
                return;
            }
        }

        for (const auto &curve : CurveManager::GetInstance().getCurveList()) {
            if (curve) {
                if (!VisualCurvePosManager::GetInstance().hasHidenCurve(curve->getCurveName())) {
                    QList<SKeyPoint> keyFrameList;
                    VisualCurvePosManager::GetInstance().getKeyPointList(curve->getCurveName(), keyFrameList);
                    for (int i{0}; i < keyFrameList.size(); i++) {
                        QPointF keyPointF(keyFrameList[i].x, keyFrameList[i].y);
                        if (abs(keyPointF.x() - xPos) <= 3 && abs(keyPointF.y() - yPos) <= 3) {
                            if (VisualCurvePosManager::GetInstance().getKeyBoardType() == KEY_BOARD_TYPE::POINT_MOVE) {
                                VisualCurvePosManager::GetInstance().setCurrentPointInfo(curve->getCurveName(), i);
                                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_KEY);
                                VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                                Q_EMIT sigPressKey();
                                pushState2UndoStack(fmt::format("select point from '{}', {} point", curve->getCurveName(), i));
                                update();
                            }
                            return;
                        }
                    }
                }
            }
        }
        VisualCurvePosManager::GetInstance().resetCurrentPointInfo();
        if (VisualCurvePosManager::GetInstance().getPressAction() != MOUSE_PRESS_NONE) {
            pushState2UndoStack("select none");
        }
        VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::POINT_MOVE);
        VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_NONE);
        VisualCurvePosManager::GetInstance().clearMultiSelPoints();
        Q_EMIT sigPressKey();
        update();
    }
}

void VisualCurveWidget::mouseMoveEvent(QMouseEvent *event) {
    KEY_BOARD_TYPE keyType = VisualCurvePosManager::GetInstance().getKeyBoardType();
    switch (keyType) {
    case KEY_BOARD_TYPE::POINT_MOVE: {
        pointMove(event);
        break;
    }
    case KEY_BOARD_TYPE::MULTI_POINT_MOVE: {
        multiPointMove(event);
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE_X: {
        curveMoveX(event);
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE_Y: {
        curveMoveY(event);
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE: {
        curveMove(event);
        break;
    }
    default:
        break;
    }
    isMoveDrag_ = true;
    update();
    QWidget::mouseMoveEvent(event);
}

void VisualCurveWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->MouseButtonRelease) {
        if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY && VisualCurvePosManager::GetInstance().getSameKeyType() != SAME_NONE) {

            std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

            QList<SKeyPoint> keyPoints;
            int sameKeyPointIndex = VisualCurvePosManager::GetInstance().getSameKeyPointIndex();
            VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);

            SKeyPoint keyPoint = keyPoints.at(sameKeyPointIndex);
            if (CurveManager::GetInstance().getCurve(curCurve)) {
                Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
                curve->delSamePoint(keyPoint.keyFrame);
            }

            int oldCurX = viewportOffset_.x();
            int oldCurY = viewportOffset_.y();
            double oldEachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
            double oldEachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;
            keyPoints.removeAt(sameKeyPointIndex);
            VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);
            reCaculateCurvePoint(oldCurX, oldCurY, oldEachFrameWidth, oldEachValueWidth);

            switch (VisualCurvePosManager::GetInstance().getSameKeyType()) {
            case SAME_WITH_LAST_KEY: {
                VisualCurvePosManager::GetInstance().setCurrentPointInfo(sameKeyPointIndex);
                break;
            }
            case SAME_WITH_NEXT_KEY: {
                VisualCurvePosManager::GetInstance().setCurrentPointInfo(sameKeyPointIndex - 1);
                break;
            }
            default:
                break;
            }
            VisualCurvePosManager::GetInstance().setSameKeyType(SAME_NONE);
        }
        Q_EMIT raco::signal::signalProxy::GetInstance().sigUpdatePointTagent();
    }
    update();
    if (isMoveDrag_) {
        pushMovedState();
        isMoveDrag_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void VisualCurveWidget::keyPressEvent(QKeyEvent *event) {
    if(event->modifiers() == Qt::ControlModifier) {
        if(event->key() == Qt::Key_Tab) {
            Q_EMIT sigSwitchCurveType();
            return;
        }
        pressAction_ = KEY_PRESS_ACT::KEY_PRESS_CTRL;
    }
    if(event->modifiers() == Qt::AltModifier) {
        if (event->key() == Qt::Key_X) {
            VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::CURVE_MOVE_X);
        } else if (event->key() == Qt::Key_Y) {
            VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::CURVE_MOVE_Y);
        } else if (event->key() == Qt::Key_M) {
            VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::CURVE_MOVE);
        }
    }
    if(event->key() == Qt::Key_I) {
        insertKeyFrame();
    }
    QWidget::keyPressEvent(event);
}

void VisualCurveWidget::keyReleaseEvent(QKeyEvent *event) {
    pressAction_ = KEY_PRESS_ACT::KEY_PRESS_NONE;
    QWidget::keyReleaseEvent(event);
}

void VisualCurveWidget::setViewportRect(const QSize areaSize,
                                     const QPoint viewportOffset,
                                     const double scaleValue,
                                     const MOUSEACTION mouseAction) {
    resize(areaSize);

    mouseAction_ = mouseAction;
    if (mouseAction_ == MOUSE_LEFT_EXTEND || mouseAction_ == MOUSE_LEFT_MOVE) {
        offsetX_ -= 1;
    } else if (mouseAction_== MOUSE_RIGHT_MOVE || mouseAction_ == MOUSE_RIGHT_EXTEND) {
        offsetX_ += 1;
    }

    if (mouseAction_ == MOUSE_TOP_EXTEND || mouseAction_ == MOUSE_TOP_MOVE) {
        offsetY_ += 1;
    } else if (mouseAction_== MOUSE_BUTTOM_EXTEND || mouseAction_ == MOUSE_BUTTOM_MOVE) {
        offsetY_ -= 1;
    }

    int oldCurX = viewportOffset_.x();
    int oldCurY = viewportOffset_.y();
    double oldEachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double oldEachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    QString strNumX = QString::number(numTextIntervalX_);
    QString strNumY = QString::number(numTextIntervalY_);
    if (mouseAction_ == MOUSE_SCROLL_UP) {
        intervalLength_ += INTERVAL_STEP;
        if (intervalLength_ > INTERVAL_LENGTH_MAX && numTextIntervalX_ > 1) {
            intervalLength_ = INTERVAL_LENGTH_MIN;
            if (strNumX.at(0) == '5') {
                strNumX.remove(0, 1);
                strNumX.insert(0, "2");
                numTextIntervalX_ = strNumX.toInt();
            } else {
                numTextIntervalX_ /= 2;
            }

            if (numTextIntervalY_ > 0.1) {
                if (strNumY == "0.5") {
                    strNumY = "0.2";
                    numTextIntervalY_ = strNumY.toDouble();
                } else if (strNumY.at(0) == '5'){
                    strNumY.remove(0, 1);
                    strNumY.insert(0, "2");
                    numTextIntervalY_ = strNumY.toDouble();
                } else {
                    numTextIntervalY_ /= 2;
                }
            }
        }
    } else if (mouseAction_ == MOUSE_SCROLL_DOWN) {
        intervalLength_ -= INTERVAL_STEP;
        if (intervalLength_ < INTERVAL_LENGTH_MIN) {
            intervalLength_ = INTERVAL_LENGTH_MAX;
            if (strNumX.at(0) == '2') {
                strNumX.remove(0, 1);
                strNumX.insert(0, "5");
                numTextIntervalX_ = strNumX.toInt();
            } else {
                numTextIntervalX_ *= 2;
            }
            if (strNumY == "0.2") {
                strNumY = "0.5";
                numTextIntervalY_ = strNumY.toDouble();
            } else if (strNumY.at(0) == '2'){
                strNumY.remove(0, 1);
                strNumY.insert(0, "5");
                numTextIntervalY_ = strNumY.toDouble();
            } else {
                numTextIntervalY_ *= 2;
            }
        }
    }

    int width = this->width();
    int height = this->height();
    double frameLength = (finishFrame_ - startFrame_) * ((double)intervalLength_/(double)numTextIntervalX_);

    moveNumX_ = ((frameLength - width) / 2) / (double)intervalLength_ + offsetX_;
    moveNumY_ = (height / (double)intervalLength_) / 2.0 + offsetY_;

    viewportOffset_.setX(moveNumX_ * intervalLength_);
    viewportOffset_.setY(moveNumY_ * intervalLength_);
    button_->setText(VisualCurvePosManager::GetInstance().getCurFrame());
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)VisualCurvePosManager::GetInstance().getCurFrame() - viewportOffset_.x() - button_->width()/2, 0);

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    reCaculateCurvePoint(oldCurX, oldCurY, oldEachFrameWidth, oldEachValueWidth);
    update();

    VisualCurvePosManager::GetInstance().setCurPos(viewportOffset_.x(), viewportOffset_.y());
    VisualCurvePosManager::GetInstance().setWidth(eachFrameWidth, eachValueWidth);
}

void VisualCurveWidget::startAnimation() {
    if (!timerId_) {
        if (VisualCurvePosManager::GetInstance().getCurFrame() < startFrame_) {
            VisualCurvePosManager::GetInstance().setCurFrame(startFrame_);
        }

        loopCount_ = animationDataManager::GetInstance().getActiveAnimationData().GetLoopCount();
        if (0 == loopCount_)
            loopCount_ = INT32_MAX;
        curLoop_ = 0;
        int timer = animationDataManager::GetInstance().getActiveAnimationData().GetUpdateInterval() * animationDataManager::GetInstance().getActiveAnimationData().GetPlaySpeed();
        timerId_ = startTimer(timer);
        return;
    }
    stopAnimation();
}

void VisualCurveWidget::stopAnimation() {
    killTimer(timerId_);
    timerId_ = 0;
}

void VisualCurveWidget::createKeyFrame(QString curveName) {
    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    if (CurveManager::GetInstance().hasCurve(curveName.toStdString())) {
        Curve *curve = CurveManager::GetInstance().getCurve(curveName.toStdString());
        if (curve) {
            std::list<Point*> pointList = curve->getPointList();
            QList<SKeyPoint> keyPoints;
            VisualCurvePosManager::GetInstance().getKeyPointList(curveName.toStdString(), keyPoints);

            // get point index of pointlist;
            int index{keyPoints.size()};
            for (int i{0}; i < keyPoints.size(); i++) {
                SKeyPoint point = keyPoints.at(i);
                if (point.keyFrame > VisualCurvePosManager::GetInstance().getCurFrame()) {
                    index = i;
                    break;
                }
            }

            auto it = pointList.begin();
            while (it != pointList.end()) {
                if ((*it)->getKeyFrame() == VisualCurvePosManager::GetInstance().getCurFrame()) {
                    QPointF pointF;
                    double value{0};
                    if ((*it)->getDataValue().type() == typeid(double)) {
                        value = *(*it)->getDataValue()._Cast<double>();
                    }
                    keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);
                    SKeyPoint newPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame());

                    // get next point
                    int offsetLastKey{10};
                    int offsetNextKey{10};

                    int curKeyFrame = (*it)->getKeyFrame();
                    if (it != pointList.begin()) {
                        it--;
                        int lastKey = (*it)->getKeyFrame();
                        if (curKeyFrame - lastKey < 10) {
                            offsetLastKey = curKeyFrame - lastKey;
                            offsetLastKey = offsetLastKey == 0 ? 1 : offsetLastKey;
                        }
                        it++;
                    }
                    it++;
                    if (it != pointList.end()) {
                        int nextKey = (*it)->getKeyFrame();
                        if (nextKey - curKeyFrame < 10) {
                            offsetNextKey = nextKey - curKeyFrame;
                            offsetNextKey = offsetNextKey == 0 ? 1 : offsetNextKey;
                        }
                    }
                    newPoint.setLeftPoint(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()));
                    newPoint.setRightPoint(QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
                    if (index != -1) {
                        VisualCurvePosManager::GetInstance().insertKeyPoint(index, curveName.toStdString(), newPoint);
                    }

                    update();
                    pushState2UndoStack(fmt::format("insert point to '{}', '{}' keyframe", curveName.toStdString(), curKeyFrame));
                    return;
                }
                it++;
            }
        }
    }
}

void VisualCurveWidget::slotUpdateSlider(int pix) {
    QMutexLocker locker(&pressDragBtnMutex_);
    isPressDragBtn_ = true;
    int n = ((double)numTextIntervalX_ / (double)intervalLength_) * (pix + viewportOffset_.x() + button_->width()/2);
    VisualCurvePosManager::GetInstance().setCurFrame(n);
    button_->setText(n);

    Q_EMIT sigUpdateSlider(n);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(n);
    Q_EMIT sigUpdateCursorX();
    update();
}

void VisualCurveWidget::slotUpdateSliderPos(int keyFrame) {
    VisualCurvePosManager::GetInstance().setCurFrame(keyFrame);
    button_->setText(keyFrame);

    button_->blockSignals(true);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)keyFrame - viewportOffset_.x() - button_->width()/2, 0);
    button_->blockSignals(false);
}

void VisualCurveWidget::slotFinishSlider(int pix) {
    int n = ((double)numTextIntervalX_ / (double)intervalLength_) * (pix + viewportOffset_.x() + button_->width()/2);
    pushState2UndoStack(fmt::format("set current keyframe '{}'", n));
}

void VisualCurveWidget::slotSetStartFrame(int keyframe) {
    QObject* object = sender();
    common_editors::Int64Editor *editor = static_cast<common_editors::Int64Editor*>(object);

    startFrame_ = keyframe;
    if (keyframe >= finishFrame_) {
        startFrame_ = keyframe - 1;
        editor->setValue(startFrame_);
        return;
    }
    update();
}

void VisualCurveWidget::slotSetFinishFrame(int keyframe) {
    QObject* object = sender();
    common_editors::Int64Editor *editor = static_cast<common_editors::Int64Editor*>(object);

    finishFrame_ = keyframe;
    if (keyframe <= startFrame_) {
        finishFrame_ = keyframe + 1;
        editor->setValue(finishFrame_);
        return;
    }
    update();
}

void VisualCurveWidget::slotSetCurFrameToBegin() {
    VisualCurvePosManager::GetInstance().setCurFrame(startFrame_);
    button_->setText(startFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)startFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(startFrame_);

    pushState2UndoStack(fmt::format("set current keyframe '{}'", finishFrame_));
}

void VisualCurveWidget::slotSetCurFrameToEnd() {
    VisualCurvePosManager::GetInstance().setCurFrame(finishFrame_);
    button_->setText(finishFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)finishFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(finishFrame_);

    pushState2UndoStack(fmt::format("set current keyframe '{}'", finishFrame_));
}

void VisualCurveWidget::slotClearKeyFrames() {
    VisualCurvePosManager::GetInstance().clearKeyPointMap();
    update();
}

void VisualCurveWidget::slotShowContextMenu(const QPoint &p) {
    if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY) {
        menu_->exec(mapToGlobal(p));
    }
}

void VisualCurveWidget::slotSwitchPoint2Liner() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    switchPointInterpolationType(EInterPolationType::LINER);
    pushState2UndoStack(fmt::format("switch curve type to Liner from '{}', {} point", curve, index));
    Q_EMIT sigUpdateSelKey();
}

void VisualCurveWidget::slotSwitchPoint2Hermite() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    SKeyPoint keyPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        if (keyPoint.type == EInterPolationType::BESIER_SPLINE) {
            bezierSwitchToHermite();
        } else {
            switchPointInterpolationType(EInterPolationType::HERMIT_SPLINE);
        }
        pushState2UndoStack(fmt::format("switch curve type to Hermite from '{}', {} point", curve, index));
        Q_EMIT sigUpdateSelKey();
    }
}

void VisualCurveWidget::slotSwitchPoint2Bezier() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    SKeyPoint keyPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        if (keyPoint.type == EInterPolationType::HERMIT_SPLINE) {
            hermiteSwitchToBezier();
        } else {
            switchPointInterpolationType(EInterPolationType::BESIER_SPLINE);
        }
        pushState2UndoStack(fmt::format("switch curve type to Bezier from '{}', {} point", curve, index));
        Q_EMIT sigUpdateSelKey();
    }
}

void VisualCurveWidget::slotSwitchPoint2Step() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    switchPointInterpolationType(EInterPolationType::STEP);
    pushState2UndoStack(fmt::format("switch curve type to Step from '{}', {} point", curve, index));
    Q_EMIT sigUpdateSelKey();
}

void VisualCurveWidget::slotDeleteKeyFrame() {
    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    // delete ui point interpolation type
    QList<SKeyPoint> keyPoints;
    VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);

    SKeyPoint keyPoint = keyPoints.at(index);
    keyPoints.removeAt(index);
    VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

    // delete curveData point interpolation type
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        curve->takePoint(keyPoint.keyFrame);
        if (curve->getPointList().empty()) {
            CurveManager::GetInstance().takeCurve(curCurve);
            Q_EMIT sigDeleteCurve(curCurve);
        }
    }

    VisualCurvePosManager::GetInstance().setCurrentPointInfo(std::string());
    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_NONE);
    update();
    pushState2UndoStack(fmt::format("delete point from '{}' curve, '{}' point",
                                    curCurve, index));
    Q_EMIT sigPressKey();
}

void VisualCurveWidget::slotRefreshVisualCurve() {
    update();
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(VisualCurvePosManager::GetInstance().getCurFrame());
}

void VisualCurveWidget::slotRefreshVisualCurveAfterUndo() {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();
    reCaculateCurvePoint(curX, curY, eachFrameWidth, eachValueWidth);

    button_->setText(VisualCurvePosManager::GetInstance().getCurFrame());
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)VisualCurvePosManager::GetInstance().getCurFrame() - viewportOffset_.x() - button_->width()/2, 0);
    update();
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(VisualCurvePosManager::GetInstance().getCurFrame());
}

void VisualCurveWidget::slotRefreshCursorX() {
    int curFrame_ = VisualCurvePosManager::GetInstance().getCurFrame();
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    update();
    Q_EMIT sigUpdateSlider(curFrame_);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void VisualCurveWidget::slotInsertCurve(QString property, QString curveName, QVariant value) {
    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    Curve *curve = CurveManager::GetInstance().getCurve(curveName.toStdString());
    if (curve) {
        std::list<Point*> pointList = curve->getPointList();
        QList<SKeyPoint> srcPoints;
        auto it = pointList.begin();
        while (it != pointList.end()) {
            QPointF pointF;
            double value{0};
            if ((*it)->getDataValue().type() == typeid(double)) {
                value = *(*it)->getDataValue()._Cast<double>();
            }
            keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);

            SKeyPoint keyPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame());

            // get next point
            int offsetLastKey{10};
            int offsetNextKey{10};

            int curKeyFrame = (*it)->getKeyFrame();
            if (it != pointList.begin()) {
                it--;
                int lastKey = (*it)->getKeyFrame();
                if (curKeyFrame - lastKey < 10) {
                    offsetLastKey = curKeyFrame - lastKey;
                    offsetLastKey = offsetLastKey == 0 ? 1 : offsetLastKey;
                }
                it++;
            }
            it++;
            if (it != pointList.end()) {
                int nextKey = (*it)->getKeyFrame();
                if (nextKey - curKeyFrame < 10) {
                    offsetNextKey = nextKey - curKeyFrame;
                    offsetNextKey = offsetNextKey == 0 ? 1 : offsetNextKey;
                }
            }
            keyPoint.setLeftPoint(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()));
            keyPoint.setRightPoint(QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
            srcPoints.append(keyPoint);
        }
        VisualCurvePosManager::GetInstance().addKeyPointList(curve->getCurveName(), srcPoints);

        pushState2UndoStack(fmt::format("insert curve '{}'", curve->getCurveName()));
    }
    update();
}

void VisualCurveWidget::slotSwitchCurveType(int type) {
    switch (type) {
    case EInterPolationType::LINER: {
        slotSwitchPoint2Liner();
        break;
    }
    case EInterPolationType::HERMIT_SPLINE: {
        slotSwitchPoint2Hermite();
        break;
    }
    case EInterPolationType::BESIER_SPLINE: {
        slotSwitchPoint2Bezier();
        break;
    }
    case EInterPolationType::STEP: {
        slotSwitchPoint2Step();
        break;
    }
    }
}

void VisualCurveWidget::initVisualCurvePos() {
    updateCurvePoint();
    commandInterface_->undoStack().resetUndoState(VisualCurvePosManager::GetInstance().convertDataStruct());
}

void VisualCurveWidget::drawKeyFrame(QPainter &painter) {
    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;
    std::string animation = animationDataManager::GetInstance().GetActiveAnimation();
    std::map < std::string, std::map<std::string, std::string>> curveBindingMap;
    curveBindingMap = NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().bindingMap();

    auto getProperty = [=](std::string curve)->std::string {
        for (const auto &it : curveBindingMap) {
            for (const auto &bindingIt : it.second) {
                if (bindingIt.second == curve) {
                    return bindingIt.first;
                }
            }
        }
        return std::string();
    };

    auto painterPen = [=](QString curve, int width, int alpha)->QPen {
        QPen pen(QColor(), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QString property = curve.section(".", -1);
        if (property.compare("y") == 0) {
            pen.setColor(QColor(30, 125, 183, alpha));
        } else if (property.compare("z") == 0) {
            pen.setColor(QColor(172, 200, 13, alpha));
        } else {
            pen.setColor(QColor(223, 49, 71, alpha));
        }
        return pen;
    };

    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QBrush brush;
    brush.setColor(QColor(0, 0, 0, 255));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.save();

    for (const auto &it : VisualCurvePosManager::GetInstance().getKeyPointMap().toStdMap()) {
        if (VisualCurvePosManager::GetInstance().hasHidenCurve(it.first)) {
            continue;
        }

        QPen pen = painterPen(QString::fromStdString(getProperty(it.first)), 1, 125);
        painter.setPen(pen);
        for (int i{0}; i < it.second.size(); ++i) {
            SKeyPoint lastPoint = it.second.at(i);

            QPair<QPointF, QPointF> lastWorkerPoint(lastPoint.leftPoint, lastPoint.rightPoint);
            MOUSE_PRESS_ACTION pressAction = VisualCurvePosManager::GetInstance().getPressAction();
            if ((pressAction == MOUSE_PRESS_KEY || pressAction == MOUSE_PRESS_LEFT_WORKER_KEY || pressAction == MOUSE_PRESS_RIGHT_WORKER_KEY) && curCurve == it.first) {
                QPen tempPen = painterPen(QString::fromStdString(getProperty(it.first)), 2, 255);
                painter.setPen(tempPen);
            }

            if (i + 1 < it.second.size()) {
                SKeyPoint nextPoint = it.second.at(i + 1);
                QPair<QPointF, QPointF> nextWorkerPoint(nextPoint.leftPoint, nextPoint.rightPoint);
                switch (lastPoint.type) {
                case EInterPolationType::LINER: {
                    drawLiner(painter, it.first, lastPoint, nextPoint);
                    break;
                }
                case EInterPolationType::BESIER_SPLINE: {
                    drawBezier(painter, it.first, lastPoint, nextPoint, lastWorkerPoint, nextWorkerPoint);
                    break;
                }
                case EInterPolationType::HERMIT_SPLINE: {
                    drawHermite(painter, it.first, lastPoint, nextPoint, lastWorkerPoint, nextWorkerPoint);
                    break;
                }
                case EInterPolationType::STEP: {
                    drawStep(painter, it.first, lastPoint, nextPoint);
                    break;
                }
                }
            }
            if (i == 0) {
                SKeyPoint nextPoint = lastPoint;
                lastPoint.setX(0);
                if (lastPoint.y >= *numHeight) {
                    drawLiner(painter, it.first, lastPoint, nextPoint);
                }
            }
            if (i == it.second.size() - 1) {
                SKeyPoint nextPoint(this->width(), lastPoint.y);
                if (nextPoint.y >= *numHeight) {
                    drawLiner(painter, it.first, lastPoint, nextPoint);
                }
            }

            // paint worker point
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.save();
            drawWorkerPoint(painter, lastPoint, lastWorkerPoint, i, it.first);
        }

        painter.setBrush(brush);
        painter.setPen(pen);
        painter.save();
        // paint keyframe
        for (auto i = 0; i < it.second.size(); i++) {
            if (it.second.at(i).y < *numHeight) {
                continue;
            }
            if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY && (curCurve == it.first && index == i)) {
                auto brush = painter.brush();
                brush.setColor(QColor(255, 255, 255, 255));
                painter.setBrush(brush);
                painter.drawEllipse(QPointF(it.second.at(i).x, it.second.at(i).y) , 3, 3);
                painter.restore();
                continue;
            }
            if (VisualCurvePosManager::GetInstance().getKeyBoardType() == MULTI_POINT_MOVE && (curCurve == it.first && VisualCurvePosManager::GetInstance().hasMultiSelPoint(i))) {
                auto brush = painter.brush();
                brush.setColor(QColor(200, 200, 0, 255));
                painter.setBrush(brush);
                painter.drawEllipse(QPointF(it.second.at(i).x, it.second.at(i).y) , 3, 3);
                painter.restore();
                continue;
            }
            painter.drawEllipse(QPointF(it.second.at(i).x, it.second.at(i).y) , 3, 3);
        }
    }
    painter.end();
}

void VisualCurveWidget::drawLiner(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint) {
    // liner curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    int lastY = lastPoint.y;
    int nextY = nextPoint.y;
    if (lastY < *numHeight && nextY < *numHeight) {
        return;
    }
    lastY = lastY < *numHeight ? *numHeight : lastY;
    nextY = nextY < *numHeight ? *numHeight : nextY;

    painter.setBrush(QBrush());
    painter.drawLine(lastPoint.x, lastY, nextPoint.x, nextY);
}

void VisualCurveWidget::drawBezier(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint) {
    // beizer curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    int lastY = lastPoint.y;
    int nextY = nextPoint.y;
    if (lastY < *numHeight && nextY < *numHeight) {
        return;
    }

    // get worker point
    QPointF endWorkerPoint(lastWorkerPoint.second);
    QPointF startWorkerPoint(nextWorkerPoint.first);
    if (lastWorkerPoint.second.x() > nextPoint.x) {
        endWorkerPoint.setX(nextPoint.x);
        double y = lastPoint.y + (nextPoint.x - lastPoint.x) / (lastWorkerPoint.second.x() - lastPoint.x) * (lastWorkerPoint.second.y() - lastPoint.y);
        endWorkerPoint.setY(y);
    }
    if (nextWorkerPoint.first.x() < lastPoint.x) {
        startWorkerPoint.setX(lastPoint.x);
        double y = nextPoint.y + (nextPoint.x - lastPoint.x) / (nextPoint.x - nextWorkerPoint.first.x()) * (nextWorkerPoint.first.y() - nextPoint.y);
        startWorkerPoint.setY(y);
    }

    QList<QPointF> srcPoints, destPoints;
    srcPoints.push_back(lastKey);
    srcPoints.push_back(endWorkerPoint);
    srcPoints.push_back(startWorkerPoint);
    srcPoints.push_back(nextKey);

    // get bezier points
    createNBezierCurve(srcPoints, destPoints, 0.01);

    bool hasSecondCurve = false;
    int beginIndex{1}, endIndex{destPoints.size() - 1};
    for (auto i = 1; i < destPoints.size(); i++) {
        if (destPoints.at(i).y() >= *numHeight) {
            beginIndex = i;
            break;
        }
    }

    // paint bezier curve
    painter.setBrush(QBrush());
    QPainterPath painterPath;
    painterPath.moveTo(destPoints.at(beginIndex));
    for (auto i = beginIndex; i < destPoints.size(); i++) {
        if (destPoints.at(i).y() >= *numHeight) {
            painterPath.lineTo(destPoints.at(i));
        } else {
            endIndex = i;
            break;
        }
    }
    painter.drawPath(painterPath);

    // second curve
    if (endIndex < destPoints.size() - 1) {
        for (auto i = endIndex; i < destPoints.size(); i++) {
            if (destPoints.at(i).y() >= *numHeight) {
                beginIndex = i;
                hasSecondCurve = true;
                break;
            }
        }
        if (hasSecondCurve) {
            QPainterPath painterPath2;
            painterPath2.moveTo(destPoints.at(beginIndex));
            for (auto i = beginIndex; i < destPoints.size(); i++) {
                painterPath2.lineTo(destPoints.at(i));
            }
            painter.drawPath(painterPath2);
        }
    }
}

void VisualCurveWidget::drawHermite(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint) {
    // hermite curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    int lastY = lastPoint.y;
    int nextY = nextPoint.y;
    if (lastY < *numHeight && nextY < *numHeight) {
        return;
    }

    QPointF offsetKey(nextKey.x() - lastKey.x(), nextKey.y() - lastKey.y());

    QPointF endWorkerPoint(lastWorkerPoint.second);
    QPointF startWorkerPoint(nextWorkerPoint.first);
    double lastPointX = nextPoint.x - 3.0 * (nextPoint.x - lastPoint.x);
    double nextPointX = 3.0 * (nextPoint.x - lastPoint.x) + lastPoint.x;
    if (lastWorkerPoint.second.x() > nextPointX) {
        endWorkerPoint.setX(nextPointX);
        double y = lastPoint.y + (nextPointX - lastPoint.x) / (lastWorkerPoint.second.x() - lastPoint.x) * (lastWorkerPoint.second.y() - lastPoint.y);
        endWorkerPoint.setY(y);
    }
    if (nextWorkerPoint.first.x() < lastPointX) {
        startWorkerPoint.setX(lastPointX);
        double y = nextPoint.y + (nextPoint.x - lastPointX) / (nextPoint.x - nextWorkerPoint.first.x()) * (nextWorkerPoint.first.y() - nextPoint.y);
        startWorkerPoint.setY(y);
    }

    // tangency point
    QPointF tangPoint1, tangPoint2;
    tangPoint1.setX(endWorkerPoint.x() - lastKey.x());
    tangPoint1.setY(endWorkerPoint.y() - lastKey.y());
    tangPoint2.setX(nextKey.x() - startWorkerPoint.x());
    tangPoint2.setY(nextKey.y() - startWorkerPoint.y());

    QList<QPointF> srcPoints, destPoints;
    srcPoints.push_back(lastKey);
    srcPoints.push_back(nextKey);
    srcPoints.push_back(tangPoint1);
    srcPoints.push_back(tangPoint2);

    // get hermite points
    createHermiteCurve(srcPoints, destPoints, 0.01);

    bool hasSecondCurve = false;
    int beginIndex{1}, endIndex{destPoints.size() - 1};
    for (auto i = 1; i < destPoints.size(); i++) {
        if (destPoints.at(i).y() >= *numHeight) {
            beginIndex = i;
            break;
        }
    }

    // paint bezier curve
    painter.setBrush(QBrush());
    QPainterPath painterPath;
    painterPath.moveTo(destPoints.at(beginIndex));
    for (auto i = beginIndex; i < destPoints.size(); i++) {
        if (destPoints.at(i).y() >= *numHeight) {
            painterPath.lineTo(destPoints.at(i));
        } else {
            endIndex = i;
            break;
        }
    }
    painter.drawPath(painterPath);

    // second curve
    if (endIndex < destPoints.size() - 1) {
        for (auto i = endIndex; i < destPoints.size(); i++) {
            if (destPoints.at(i).y() >= *numHeight) {
                beginIndex = i;
                hasSecondCurve = true;
                break;
            }
        }
        if (hasSecondCurve) {
            QPainterPath painterPath2;
            painterPath2.moveTo(destPoints.at(beginIndex));
            for (auto i = beginIndex; i < destPoints.size(); i++) {
                painterPath2.lineTo(destPoints.at(i));
            }
            painter.drawPath(painterPath2);
        }
    }
}

void VisualCurveWidget::drawBezier2Hermite(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint) {
    // hermite curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    // tangency point
    QPointF tangPoint1, tangPoint2;
    tangPoint1.setX(lastWorkerPoint.second.x() - lastKey.x());
    tangPoint1.setY(lastWorkerPoint.second.y() - lastKey.y());
    tangPoint2.setX(nextKey.x() - nextWorkerPoint.first.x());
    tangPoint2.setY(nextKey.y() - nextWorkerPoint.first.y());

    // u1 u2 convert p1 p2
    QPointF bezierPoint1, bezierPoint2;
    bezierPoint1.setX(tangPoint1.x() / 3 + lastKey.x());
    bezierPoint1.setY(tangPoint1.y() / 3 + lastKey.y());
    bezierPoint2.setX(nextKey.x() - tangPoint2.x() / 3);
    bezierPoint2.setY(nextKey.y() - tangPoint2.y() / 3);

    QList<QPointF> srcPoints, destPoints;
    srcPoints.push_back(lastKey);
    srcPoints.push_back(bezierPoint1);
    srcPoints.push_back(bezierPoint2);
    srcPoints.push_back(nextKey);

    // get hermite points
    createNBezierCurve(srcPoints, destPoints, 0.01);

    // paint hermite curve
    QPainterPath painterPath;
    painterPath.moveTo(destPoints.at(0));
    for (auto i = 1; i < destPoints.size(); i++) {;
        painterPath.lineTo(destPoints.at(i));
    }
    painter.setBrush(QBrush());
    painter.drawPath(painterPath);
}

void VisualCurveWidget::drawStep(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint) {
    // step curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    int lastY = lastPoint.y;
    int nextY = nextPoint.y;
    if (lastY < *numHeight && nextY < *numHeight) {
        return;
    }
    lastY = lastY < *numHeight ? *numHeight : lastY;
    nextY = nextY < *numHeight ? *numHeight : nextY;

    painter.setBrush(QBrush());
    if (lastPoint.y >= *numHeight) {
        painter.drawLine(lastPoint.x, lastPoint.y, nextPoint.x, lastPoint.y);
    }
    painter.drawLine(nextPoint.x, lastY, nextPoint.x, nextY);
}

void VisualCurveWidget::drawWorkerPoint(QPainter &painter, SKeyPoint point, QPair<QPointF, QPointF> workerPoint, int index, std::string curve) {

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;
    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    int leftY = workerPoint.first.y();
    int rightY = workerPoint.second.y();
    int rightX = workerPoint.second.x();
    int leftX = workerPoint.first.x();

    if (leftY < *numHeight && rightY < *numHeight) {
        return;
    }

    if (point.type == EInterPolationType::HERMIT_SPLINE || point.type == EInterPolationType::BESIER_SPLINE) {
        QPainterPath path;
        MOUSE_PRESS_ACTION pressAction = VisualCurvePosManager::GetInstance().getPressAction();

        if (pressAction == MOUSE_PRESS_NONE) {
            return;
        }

        if (VisualCurvePosManager::GetInstance().getCurrentPointInfo().first == curve && VisualCurvePosManager::GetInstance().getCurrentPointInfo().second == index) {
            if (pressAction == MOUSE_PRESS_LEFT_WORKER_KEY) {
                if (rightY >= *numHeight) {
                    painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
                }
                if (leftY >= *numHeight) {
                    auto brush = painter.brush();
                    brush.setColor(QColor(255, 255, 255, 255));
                    painter.setBrush(brush);
                    painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                }
            } else if (pressAction == MOUSE_PRESS_RIGHT_WORKER_KEY) {
                if (leftY >= *numHeight) {
                    painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                }
                if (rightY >= *numHeight) {
                    auto brush = painter.brush();
                    brush.setColor(QColor(255, 255, 255, 255));
                    painter.setBrush(brush);
                    painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
                }
            } else if (pressAction == MOUSE_PRESS_KEY) {
                if (leftY >= *numHeight) {
                    painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                }
                if (rightY >= *numHeight) {
                    painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
                }
            }

            QPointF tempPoint;
            keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, point.keyFrame, 0, tempPoint);
            // caculate the right and left workerpoint beyond border
            if (leftY < *numHeight) {
                double ratio = (double)(point.y - *numHeight) / (double)(point.y - leftY);
                leftX += qRound((1 - ratio) * (tempPoint.x() - leftX));
                leftY = *numHeight;
            }
            if (rightY < *numHeight) {
                double ratio = (double)(point.y - *numHeight) / (double)(point.y - rightY);
                rightX -= qRound((1 - ratio) * (rightX - tempPoint.x()));
                rightY = *numHeight;
            }

            // first point worker path
            path.moveTo(QPointF(leftX, leftY));
            path.lineTo(QPointF(rightX, rightY));
            auto pen = painter.pen();
            pen.setColor(QColor(255, 255, 204, 255));
            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawPath(path);
            painter.restore();
        }
    }/* else {
        if (lastPoint.type == EInterPolationType::HERMIT_SPLINE || lastPoint.type == EInterPolationType::BESIER_SPLINE) {
            QPainterPath workerPath;
            MOUSE_PRESS_ACTION pressAction = VisualCurvePosManager::GetInstance().getPressAction();

            if (pressAction == MOUSE_PRESS_NONE) {
                return;
            }

            if (VisualCurvePosManager::GetInstance().getCurrentPointInfo().first == curve && VisualCurvePosManager::GetInstance().getCurrentPointInfo().second == index) {
                if (pressAction == MOUSE_PRESS_LEFT_WORKER_KEY) {
                    auto brush = painter.brush();
                    brush.setColor(QColor(255, 255, 255, 255));
                    painter.setBrush(brush);
                    painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                } else if (pressAction == MOUSE_PRESS_KEY) {
                    painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                }

                // first point worker path
                workerPath.moveTo(workerPoint.first);
                workerPath.lineTo(QPointF(point.x, point.y));
                auto pen = painter.pen();
                pen.setColor(QColor(255, 255, 204, 255));
                pen.setWidth(1);
                painter.setPen(pen);
                painter.drawPath(workerPath);
                painter.restore();
            }
        }
    }*/
}

void VisualCurveWidget::updateCurvePoint() {
    VisualCurvePosManager::GetInstance().clearKeyPointMap();

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    for (const auto &curve : CurveManager::GetInstance().getCurveList()) {
        if (curve) {
            std::list<Point*> pointList = curve->getPointList();
            QList<SKeyPoint> srcPoints;
            auto it = pointList.begin();
            while (it != pointList.end()) {
                QPointF pointF;
                double value{0};
                if ((*it)->getDataValue().type() == typeid(double)) {
                    value = *(*it)->getDataValue()._Cast<double>();
                }
                keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);
                SKeyPoint keyPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame());

                if ((*it)->getInterPolationType() == EInterPolationType::HERMIT_SPLINE || (*it)->getInterPolationType() == EInterPolationType::BESIER_SPLINE) {
                    int leftKeyFrame = (*it)->getLeftKeyFrame();
                    int rightKeyFrame = (*it)->getRightKeyFrame();
                    if (leftKeyFrame != 0 && rightKeyFrame != 0) {
                        double leftData{0.0};
                        double rightData{0.0};
                        if ((*it)->getLeftData().type() == typeid(double)) {
                            leftData = *(*it)->getLeftData()._Cast<double>();
                        }
                        if ((*it)->getRightData().type() == typeid(double)) {
                            rightData = *(*it)->getRightData()._Cast<double>();
                        }
                        QPointF leftPoint, rightPoint;
                        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, leftKeyFrame, leftData, leftPoint);
                        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, rightKeyFrame, rightData, rightPoint);

                        keyPoint.setLeftPoint(leftPoint);
                        keyPoint.setRightPoint(rightPoint);
                        srcPoints.append(keyPoint);
                        it++;
                        continue;
                    }
                }
                // get next point
                int offsetLastKey{10};
                int offsetNextKey{10};

                int curKeyFrame = (*it)->getKeyFrame();
                if (it != pointList.begin()) {
                    it--;
                    int lastKey = (*it)->getKeyFrame();
                    if (curKeyFrame - lastKey < 10) {
                        offsetLastKey = curKeyFrame - lastKey;
                        offsetLastKey = offsetLastKey == 0 ? 1 : offsetLastKey;
                    }
                    it++;
                }
                it++;
                if (it != pointList.end()) {
                    int nextKey = (*it)->getKeyFrame();
                    if (nextKey - curKeyFrame < 10) {
                        offsetNextKey = nextKey - curKeyFrame;
                        offsetNextKey = offsetNextKey == 0 ? 1 : offsetNextKey;
                    }
                }
                keyPoint.setLeftPoint(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()));
                keyPoint.setRightPoint(QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
                srcPoints.append(keyPoint);
            }
            VisualCurvePosManager::GetInstance().addKeyPointList(curve->getCurveName(), srcPoints);
        }
    }
}

void VisualCurveWidget::reCaculateCurvePoint(int x, int y, double frameWidth, double valueWidth) {
    QMap<std::string, QList<SKeyPoint>> tempKeyFrameMap;

    auto searchKeyPoint = [&](SKeyPoint &point, std::string curveName, int keyFrame)->bool {
        QList<SKeyPoint> keyFrameList;
        VisualCurvePosManager::GetInstance().getKeyPointList(curveName, keyFrameList);
        for (auto keyPoint : qAsConst(keyFrameList)) {
            if (keyPoint.keyFrame == keyFrame) {
                point = keyPoint;
                return true;
            }
        }
        return false;
    };

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    for (const auto &curve : CurveManager::GetInstance().getCurveList()) {
        if (curve) {
            std::list<Point*> pointList = curve->getPointList();
            QList<SKeyPoint> srcPoints;
            int index{0};
            for (auto point : pointList) {
                QPointF pointF;
                double value{0};
                if (point->getDataValue().type() == typeid(double)) {
                    value = *point->getDataValue()._Cast<double>();
                }
                keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, point->getKeyFrame(), value, pointF);
                SKeyPoint newKeyPoint(pointF.x(), pointF.y(), point->getInterPolationType(), point->getKeyFrame());

                SKeyPoint oldKeyPoint;
                if (searchKeyPoint(oldKeyPoint, curve->getCurveName(), point->getKeyFrame())) {
                    double offsetX = pointF.x() - oldKeyPoint.x;
                    double offsetY = pointF.y() - oldKeyPoint.y;

                    if (mouseAction_ == MOUSE_SCROLL_UP || mouseAction_ == MOUSE_SCROLL_DOWN) {
                        QPointF leftWorkerPoint = reCaculatePoint(oldKeyPoint.leftPoint, x, y, frameWidth, valueWidth);
                        QPointF rightWorkerPoint = reCaculatePoint(oldKeyPoint.rightPoint, x, y, frameWidth, valueWidth);
                        newKeyPoint.setLeftPoint(leftWorkerPoint);
                        newKeyPoint.setRightPoint(rightWorkerPoint);
                    } else {
                        newKeyPoint.setLeftPoint(QPointF(oldKeyPoint.leftPoint.x() + offsetX, oldKeyPoint.leftPoint.y() + offsetY));
                        newKeyPoint.setRightPoint(QPointF(oldKeyPoint.rightPoint.x() + offsetX, oldKeyPoint.rightPoint.y() + offsetY));
                    }
                }
                srcPoints.append(newKeyPoint);
                index++;
            }
            tempKeyFrameMap.insert(curve->getCurveName(), srcPoints);
        }
    }
    VisualCurvePosManager::GetInstance().setKeyPointMap(tempKeyFrameMap);
}

void VisualCurveWidget::switchPointInterpolationType(EInterPolationType type) {
    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

    // switch ui point interpolation type
    SKeyPoint keyPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
    keyPoint.type = type;
    VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyFrame);

    double leftKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyFrame);

    double rightKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyValue);

    // switch curveData point interpolation type
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            curve->takePoint(keyPoint.keyFrame);
            point->setInterPolationType(type);
            if (type == EInterPolationType::HERMIT_SPLINE || type == EInterPolationType::BESIER_SPLINE) {
                point->setLeftData(leftKeyValue);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setRightData(rightKeyValue);
                point->setRightKeyFrame(rightKeyFrame);
            }
            curve->insertPoint(point);
        }
    }
    update();
}

void VisualCurveWidget::bezierSwitchToHermite() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    SKeyPoint lastPoint, nextPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(lastPoint);
    QPair<QPointF, QPointF> lastWorkerPoint(lastPoint.leftPoint, lastPoint.rightPoint);
    if (VisualCurvePosManager::GetInstance().getKeyPoint(curve, index + 1, nextPoint)) {
        QPair<QPointF, QPointF> nextWorkerPoint(nextPoint.leftPoint, nextPoint.rightPoint);

        QList<SKeyPoint> keyPoints;
        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);

        QPointF endWorkerPoint(lastWorkerPoint.second);
        QPointF startWorkerPoint(nextWorkerPoint.first);
        if (lastWorkerPoint.second.x() > nextPoint.x) {
            endWorkerPoint.setX(nextPoint.x);
            double y = lastPoint.y + (nextPoint.x - lastPoint.x) / (lastWorkerPoint.second.x() - lastPoint.x) * (lastWorkerPoint.second.y() - lastPoint.y);
            endWorkerPoint.setY(y);
        }
        if (nextWorkerPoint.first.x() < lastPoint.x) {
            startWorkerPoint.setX(lastPoint.x);
            double y = nextPoint.y + (nextPoint.x - lastPoint.x) / (nextPoint.x - nextWorkerPoint.first.x()) * (nextWorkerPoint.first.y() - nextPoint.y);
            startWorkerPoint.setY(y);
        }

        double lastWorkerPointX = 3.0 * (endWorkerPoint.x() - lastPoint.x) + lastPoint.x;
        double lastWorkerPointY = 3.0 * (endWorkerPoint.y() - lastPoint.y) + lastPoint.y;
        endWorkerPoint.setX(lastWorkerPointX);
        endWorkerPoint.setY(lastWorkerPointY);
        qSwap(lastWorkerPoint.second, endWorkerPoint);
        QPointF leftPoint = reCaculateLeftWorkerPoint(lastPoint, lastWorkerPoint.first, lastWorkerPoint.second);
        qSwap(lastWorkerPoint.first, leftPoint);

        double nextWorkerPointX = nextPoint.x - 3.0 * (nextPoint.x - startWorkerPoint.x());
        double nextWorkerPointY = nextPoint.y - 3.0 * (nextPoint.y - startWorkerPoint.y());
        startWorkerPoint.setX(nextWorkerPointX);
        startWorkerPoint.setY(nextWorkerPointY);
        qSwap(nextWorkerPoint.first, startWorkerPoint);
        QPointF rightPoint = reCaculateRightWorkerPoint(nextPoint, nextWorkerPoint.first, nextWorkerPoint.second);
        qSwap(nextWorkerPoint.second, rightPoint);

        lastPoint.setLeftPoint(lastWorkerPoint.first);
        lastPoint.setRightPoint(lastWorkerPoint.second);
        nextPoint.setLeftPoint(nextWorkerPoint.first);
        nextPoint.setRightPoint(nextWorkerPoint.second);

        keyPoints.replace(index, lastPoint);
        keyPoints.replace(index + 1, nextPoint);
        VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

        switchPointInterpolationType(EInterPolationType::HERMIT_SPLINE);
    }
    update();
}

void VisualCurveWidget::hermiteSwitchToBezier() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    SKeyPoint lastPoint, nextPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(lastPoint);
    QPair<QPointF, QPointF> lastWorkerPoint(lastPoint.leftPoint, lastPoint.rightPoint);
    if (VisualCurvePosManager::GetInstance().getKeyPoint(curve, index + 1, nextPoint)) {
        QPair<QPointF, QPointF> nextWorkerPoint(nextPoint.leftPoint, nextPoint.rightPoint);

        QList<SKeyPoint> keyPoints;
        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);

        QPointF endWorkerPoint(lastWorkerPoint.second);
        QPointF startWorkerPoint(nextWorkerPoint.first);

        double lastWorkerPointX = (endWorkerPoint.x() + 2 * lastPoint.x) / 3.0;
        double lastWorkerPointY = (endWorkerPoint.y() + 2 * lastPoint.y) / 3.0;
        endWorkerPoint.setX(lastWorkerPointX);
        endWorkerPoint.setY(lastWorkerPointY);
        qSwap(lastWorkerPoint.second, endWorkerPoint);
        QPointF leftPoint = reCaculateLeftWorkerPoint(lastPoint, lastWorkerPoint.first, lastWorkerPoint.second);
        qSwap(lastWorkerPoint.first, leftPoint);

        double nextWorkerPointX = (2 * nextPoint.x + startWorkerPoint.x()) / 3.0;
        double nextWorkerPointY = (2 * nextPoint.y + startWorkerPoint.y()) / 3.0;
        startWorkerPoint.setX(nextWorkerPointX);
        startWorkerPoint.setY(nextWorkerPointY);
        qSwap(nextWorkerPoint.first, startWorkerPoint);
        QPointF rightPoint = reCaculateRightWorkerPoint(nextPoint, nextWorkerPoint.first, nextWorkerPoint.second);
        qSwap(nextWorkerPoint.second, rightPoint);

        lastPoint.setLeftPoint(lastWorkerPoint.first);
        lastPoint.setRightPoint(lastWorkerPoint.second);
        nextPoint.setLeftPoint(nextWorkerPoint.first);
        nextPoint.setRightPoint(nextWorkerPoint.second);

        keyPoints.replace(index, lastPoint);
        keyPoints.replace(index + 1, nextPoint);
        VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

        switchPointInterpolationType(EInterPolationType::BESIER_SPLINE);
    }
    update();
}

QPointF VisualCurveWidget::reCaculatePoint(QPointF point, int x, int y, double frameWidth, double valueWidth) {
    int keyFrame{0};
    double value{0};
    pointF2Value(x, y, frameWidth, valueWidth, point, value);
    pointF2KeyFrame(x, y, frameWidth, valueWidth, point, keyFrame);

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();
    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;
    QPointF tempPoint;
    keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, keyFrame, value, tempPoint);
    return tempPoint;
}

void VisualCurveWidget::pointMove(QMouseEvent *event) {
    pressDragBtnMutex_.lock();
    if (event->MouseButtonPress && !isPressDragBtn_) {
        pressDragBtnMutex_.unlock();
        QPair<QPointF, QPointF> workerPoint;

        switch (VisualCurvePosManager::GetInstance().getPressAction()) {
        case MOUSE_PRESS_LEFT_WORKER_KEY: {
            leftWorkerPointMove(event);
            break;
        }
        case MOUSE_PRESS_RIGHT_WORKER_KEY: {
            rightWorkerPointMove(event);
            break;
        }
        case MOUSE_PRESS_KEY: {
            keyPointMove(event);
            break;
        }
        default:
            break;
        }
        Q_EMIT sigUpdateSelKey();
        Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(VisualCurvePosManager::GetInstance().getCurFrame());
    } else {
        isPressDragBtn_ = false;
        pressDragBtnMutex_.unlock();
    }
}

void VisualCurveWidget::keyPointMove(QMouseEvent *event) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    // caculate keyframe pos
    QList<SKeyPoint> keyPoints;
    SKeyPoint pressPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);
    VisualCurvePosManager::GetInstance().getCurKeyPoint(pressPoint);

    // judge cur pos 2 keyframe is beyond cur keyframe(int)
    double offsetX = event->pos().x() - pressPoint.x;
    double offsetY = event->pos().y() - pressPoint.y;

    if ((offsetX != 0 && offsetX > eachFrameWidth / 2) && abs(offsetX) < eachFrameWidth) {
        return;
    }
    int keyFrame = pressPoint.keyFrame;
    QPointF movePoint(event->pos().x(), event->pos().y());
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, movePoint, keyFrame);

    int moveIndex{keyPoints.size() - 1};
    bool bHandle{false};
    // cur keyframe same as Point
    for (int i{0}; i < keyPoints.size(); ++i) {
        SKeyPoint tempPoint = keyPoints.at(i);
        if (keyFrame == tempPoint.keyFrame) {
            bHandle = true;
            if (i != index) {
                SAME_KEY_TYPE type = SAME_NONE;
                if (keyFrame > pressPoint.keyFrame) {
                    type = SAME_WITH_NEXT_KEY;
                    moveIndex = i - 1;
                } else if (keyFrame < pressPoint.keyFrame) {
                    type = SAME_WITH_LAST_KEY;
                    moveIndex = i + 1;
                } else if (keyFrame == pressPoint.keyFrame) {
                   if (VisualCurvePosManager::GetInstance().getSameKeyType() != SAME_NONE) {
                       return;
                   }
                }
                VisualCurvePosManager::GetInstance().setSameKeyType(type);
                VisualCurvePosManager::GetInstance().setSameKeyPointIndex(i);
            } else {
                moveIndex = i;
            }
            break;
        } else if (keyFrame < tempPoint.keyFrame) {
            bHandle = true;
            VisualCurvePosManager::GetInstance().setSameKeyType(SAME_NONE);
            if (index != i) {
                // cur keyframe beyond point
                if (pressPoint.keyFrame < tempPoint.keyFrame) {
                    moveIndex = i - 1;
                    moveIndex = moveIndex < 0 ? 0 : moveIndex;
                } else if (pressPoint.keyFrame >= tempPoint.keyFrame) {
                    moveIndex = i;
                }
            } else {
                // cur keyframe only move
                moveIndex = i;
            }
            break;
        }
    }
    if (!bHandle) {
        VisualCurvePosManager::GetInstance().setSameKeyType(SAME_NONE);
    }

    // caculate worker point pos
    QPointF leftPoint(pressPoint.leftPoint.x() + offsetX, pressPoint.leftPoint.y() + offsetY);
    QPointF rightPoint(pressPoint.rightPoint.x() + offsetX, pressPoint.rightPoint.y() + offsetY);

    // fill worker point list
    SKeyPoint newKeyPoint(event->pos().x(), event->pos().y(), pressPoint.type, keyFrame);
    newKeyPoint.setLeftPoint(leftPoint);
    newKeyPoint.setRightPoint(rightPoint);

    keyPoints.replace(index, newKeyPoint);
    keyPoints.move(index, moveIndex);
    VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);
    VisualCurvePosManager::GetInstance().setCurrentPointInfo(moveIndex);

    // caculate worker point keyframe and value
    int leftKeyFrame{0}, rightKeyFrame{0};
    double leftData{0.0}, rightData{0.0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftData);
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightData);

    // handle curve data
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);

        if (curve->getPoint(pressPoint.keyFrame)) {
            Point *point = curve->getPoint(pressPoint.keyFrame);
            if (VisualCurvePosManager::GetInstance().getSameKeyType() == SAME_WITH_LAST_KEY) {
                curve->takePoint(pressPoint.keyFrame);
                point->setKeyFrame(keyFrame);
                curve->insertSamePoint(point);
            } else {
                point->setKeyFrame(keyFrame);
            }
            double value{0};
            if (point->getDataValue().type() == typeid(double)) {
                value = *(point->getDataValue())._Cast<double>();
            }
            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, movePoint, value);
            point->setDataValue(value);
            point->setLeftData(leftData);
            point->setLeftKeyFrame(leftKeyFrame);
            point->setRightData(rightData);
            point->setRightKeyFrame(rightKeyFrame);
            curve->sortPoint();
        }
    }
}

void VisualCurveWidget::leftWorkerPointMove(QMouseEvent *event) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        QPointF pointFLeft;
        QPointF pointFRight = keyPoint.rightPoint;
        pointFLeft.setX(event->pos().x());
        pointFLeft.setY(event->pos().y());

        std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
        QPointF keyPointF(keyPoint.x, keyPoint.y);

        // handle border
        if (keyPointF.x() < pointFLeft.x()) {
            pointFLeft.setX(keyPointF.x());
        }

        // caculate offset x,y by trigle
        double leftLength = calculateTrigLen(abs(keyPointF.x() - pointFLeft.x()), abs(keyPointF.y() - pointFLeft.y()));
        double rightLength = calculateTrigLen(abs(keyPointF.x() - pointFRight.x()), abs(keyPointF.y() - pointFRight.y()));
        double offsetY = pointFLeft.y() - keyPointF.y();
        double sinValue = abs(offsetY) / leftLength;
        sinValue = (offsetY == 0) ? 0 : sinValue;

        double rightY = rightLength * sinValue;
        double rightX = sqrt(rightLength * rightLength - rightY * rightY);

        // get tangent
        double tangAngle{0};
        if (rightX != 0) {
            double tangValue = rightY / rightX;
            tangAngle = atan(tangValue) * 180 / PI;
            tangAngle = (offsetY >= 0) ? tangAngle : (-tangAngle);
        } else {
            tangAngle = (offsetY >= 0) ? 90 : (-90);
        }

        pointFRight.setX(keyPointF.x() + rightX);
        if (keyPointF.y() >= pointFLeft.y()) {
            pointFRight.setY(keyPointF.y() + rightY);
        } else {
            pointFRight.setY(keyPointF.y() - rightY);
        }

        int leftKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyFrame);

        double leftKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyFrame);

        double rightKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyValue);

        // fill point tangent
        if (CurveManager::GetInstance().getCurve(curCurve)) {
            Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setLeftTagent(tangAngle);
                point->setRightTagent(tangAngle);
                point->setLeftData(leftKeyValue);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setRightData(rightKeyValue);
                point->setRightKeyFrame(rightKeyFrame);
            }
        }

        // fill worker point list
        keyPoint.setLeftPoint(pointFLeft);
        keyPoint.setRightPoint(pointFRight);
        VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);
        update();
    }
}

void VisualCurveWidget::rightWorkerPointMove(QMouseEvent *event) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        QPointF pointFRight;
        QPointF pointFLeft = keyPoint.leftPoint;
        pointFRight.setX(event->pos().x());
        pointFRight.setY(event->pos().y());

        std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
        QPointF keyPointF(keyPoint.x, keyPoint.y);

        // handle border
        if (keyPointF.x() > pointFRight.x()) {
            pointFRight.setX(keyPointF.x());
        }

        // caculate offset x,y by trigle
        double leftLength = calculateTrigLen(abs(keyPointF.x() - pointFLeft.x()), abs(keyPointF.y() - pointFLeft.y()));
        double rightLength = calculateTrigLen(abs(keyPointF.x() - pointFRight.x()), abs(keyPointF.y() - pointFRight.y()));
        double offsetY = keyPointF.y() - pointFRight.y();
        double sinValue = abs(offsetY) / rightLength;
        sinValue = (offsetY == 0) ? 0 : sinValue;

        double letfY = leftLength * sinValue;
        double letfX = sqrt(leftLength * leftLength - letfY * letfY);

        // get tangent
        double tangAngle{0};
        if (letfX != 0) {
            double tangValue = letfY / letfX;
            tangAngle = atan(tangValue) * 180 / PI;
            tangAngle = (offsetY >= 0) ? tangAngle : (-tangAngle);
        } else {
            tangAngle = (offsetY >= 0) ? 90 : (-90);
        }

        int leftKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyFrame);

        double leftKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.leftPoint, leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyFrame);

        double rightKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.rightPoint, rightKeyValue);

        pointFLeft.setX(keyPointF.x() - letfX);
        if (keyPointF.y() >= pointFRight.y()) {
            pointFLeft.setY(keyPointF.y() + letfY);
        } else {
            pointFLeft.setY(keyPointF.y() - letfY);
        }

        // fill point tangent
        if (CurveManager::GetInstance().getCurve(curCurve)) {
            Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setRightTagent(tangAngle);
                point->setLeftTagent(tangAngle);
                point->setLeftData(leftKeyValue);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setRightData(rightKeyValue);
                point->setRightKeyFrame(rightKeyFrame);
            }
        }

        // fill worker point list
        keyPoint.setLeftPoint(pointFLeft);
        keyPoint.setRightPoint(pointFRight);
        VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);
        update();
    }
}

void VisualCurveWidget::curveMoveX(QMouseEvent *event) {
    pressDragBtnMutex_.lock();
    if (event->MouseButtonPress && !isPressDragBtn_) {
        pressDragBtnMutex_.unlock();
        if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY) {
            double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
            double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

            int curX = viewportOffset_.x();
            int curY = viewportOffset_.y();

            SKeyPoint curKeyPoint;
            double offsetX{0};
            if (VisualCurvePosManager::GetInstance().getCurKeyPoint(curKeyPoint)) {
                offsetX = event->x() - curKeyPoint.x;
            }

            QList<SKeyPoint> keyPoints, newKeyPoints;
            if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
                if (CurveManager::GetInstance().getCurve(curCurve)) {
                    Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
                    if (offsetX < 0) {
                        for (int i{0}; i < keyPoints.size(); i++) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint(tempKeyPoint.leftPoint, tempKeyPoint.rightPoint);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), tempWorkerPoint.first.y());
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), tempWorkerPoint.second.y());
                            tempKeyPoint.setLeftPoint(leftPoint);
                            tempKeyPoint.setRightPoint(rightPoint);

                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);

                            if (curve->getPoint(tempKeyPoint.keyFrame)) {
                                Point *point = curve->getPoint(tempKeyPoint.keyFrame);
                                if (point) {
                                    point->setKeyFrame(keyFrame);
                                    point->setLeftKeyFrame(leftKeyFrame);
                                    point->setRightKeyFrame(rightKeyFrame);
                                }
                            }
                            tempKeyPoint.setKeyFrame(keyFrame);
                            newKeyPoints.push_back(tempKeyPoint);
                        }
                        VisualCurvePosManager::GetInstance().swapCurKeyPointList(newKeyPoints);
                    } else if (offsetX > 0) {
                        for (int i{keyPoints.size() - 1}; i >= 0; i--) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint(tempKeyPoint.leftPoint, tempKeyPoint.rightPoint);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), tempWorkerPoint.first.y());
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), tempWorkerPoint.second.y());
                            tempKeyPoint.setLeftPoint(leftPoint);
                            tempKeyPoint.setRightPoint(rightPoint);

                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);

                            if (curve->getPoint(tempKeyPoint.keyFrame)) {
                                Point *point = curve->getPoint(tempKeyPoint.keyFrame);
                                if (point) {
                                    point->setKeyFrame(keyFrame);
                                    point->setLeftKeyFrame(leftKeyFrame);
                                    point->setRightKeyFrame(rightKeyFrame);
                                }
                            }
                            tempKeyPoint.setKeyFrame(keyFrame);
                            newKeyPoints.push_front(tempKeyPoint);
                        }
                        VisualCurvePosManager::GetInstance().swapCurKeyPointList(newKeyPoints);
                    }
                }
            }
        }
    }
}

void VisualCurveWidget::curveMoveY(QMouseEvent *event) {
    pressDragBtnMutex_.lock();
    if (event->MouseButtonPress && !isPressDragBtn_) {
        pressDragBtnMutex_.unlock();
        if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY) {
            double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
            double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

            int curX = viewportOffset_.x();
            int curY = viewportOffset_.y();

            SKeyPoint curKeyPoint;
            double offsetY{0};
            if (VisualCurvePosManager::GetInstance().getCurKeyPoint(curKeyPoint)) {
                offsetY = event->y() - curKeyPoint.y;
            }

            QList<SKeyPoint> keyPoints, newKeyPoints;
            if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
                if (CurveManager::GetInstance().getCurve(curCurve)) {
                    Curve *curve = CurveManager::GetInstance().getCurve(curCurve);

                    for (int i{0}; i < keyPoints.size(); i++) {
                        SKeyPoint tempKeyPoint = keyPoints.at(i);
                        tempKeyPoint.setY(tempKeyPoint.y + offsetY);
                        double keyValue{0.0};
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyValue);

                        QPair<QPointF, QPointF> tempWorkerPoint(tempKeyPoint.leftPoint, tempKeyPoint.rightPoint);
                        QPointF leftPoint(tempWorkerPoint.first.x(), (tempWorkerPoint.first.y() + offsetY));
                        QPointF rightPoint(tempWorkerPoint.second.x(), (tempWorkerPoint.second.y() + offsetY));
                        tempKeyPoint.setLeftPoint(leftPoint);
                        tempKeyPoint.setRightPoint(rightPoint);

                        double leftKeyValue{0.0}, rightKeyValue{0.0};
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);

                        if (curve->getPoint(tempKeyPoint.keyFrame)) {
                            Point *point = curve->getPoint(tempKeyPoint.keyFrame);
                            if (point) {
                                point->setDataValue(keyValue);
                                point->setLeftData(leftKeyValue);
                                point->setRightData(rightKeyValue);
                            }
                        }
                        newKeyPoints.push_back(tempKeyPoint);
                    }
                    VisualCurvePosManager::GetInstance().swapCurKeyPointList(newKeyPoints);
                }
            }
        }
    }
}

void VisualCurveWidget::curveMove(QMouseEvent *event) {
    pressDragBtnMutex_.lock();
    if (event->MouseButtonPress && !isPressDragBtn_) {
        pressDragBtnMutex_.unlock();
        if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY) {
            double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
            double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

            int curX = viewportOffset_.x();
            int curY = viewportOffset_.y();

            SKeyPoint curKeyPoint;
            double offsetX{0};
            if (VisualCurvePosManager::GetInstance().getCurKeyPoint(curKeyPoint)) {
                offsetX = event->x() - curKeyPoint.x;
            }
            double offsetY{0};
            if (VisualCurvePosManager::GetInstance().getCurKeyPoint(curKeyPoint)) {
                offsetY = event->y() - curKeyPoint.y;
            }

            QList<SKeyPoint> keyPoints, newKeyPoints;
            if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
                if (CurveManager::GetInstance().getCurve(curCurve)) {
                    Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
                    if (offsetX < 0) {
                        for (int i{0}; i < keyPoints.size(); i++) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            tempKeyPoint.setY(tempKeyPoint.y + offsetY);
                            double keyValue{0.0};
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyValue);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint(tempKeyPoint.leftPoint, tempKeyPoint.rightPoint);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), (tempWorkerPoint.first.y() + offsetY));
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), (tempWorkerPoint.second.y() + offsetY));
                            tempKeyPoint.setLeftPoint(leftPoint);
                            tempKeyPoint.setRightPoint(rightPoint);

                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                            double leftKeyValue{0.0}, rightKeyValue{0.0};
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);

                            if (curve->getPoint(tempKeyPoint.keyFrame)) {
                                Point *point = curve->getPoint(tempKeyPoint.keyFrame);
                                if (point) {
                                    point->setKeyFrame(keyFrame);
                                    point->setLeftKeyFrame(leftKeyFrame);
                                    point->setRightKeyFrame(rightKeyFrame);
                                    point->setDataValue(keyValue);
                                    point->setLeftData(leftKeyValue);
                                    point->setRightData(rightKeyValue);
                                }
                            }
                            tempKeyPoint.setKeyFrame(keyFrame);
                            newKeyPoints.push_back(tempKeyPoint);
                        }
                        VisualCurvePosManager::GetInstance().swapCurKeyPointList(newKeyPoints);
                    } else if (offsetX >= 0) {
                        for (int i{keyPoints.size() - 1}; i >= 0; i--) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            tempKeyPoint.setY(tempKeyPoint.y + offsetY);
                            double keyValue{0.0};
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyValue);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint(tempKeyPoint.leftPoint, tempKeyPoint.rightPoint);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), (tempWorkerPoint.first.y() + offsetY));
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), (tempWorkerPoint.second.y() + offsetY));
                            tempKeyPoint.setLeftPoint(leftPoint);
                            tempKeyPoint.setRightPoint(rightPoint);

                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                            double leftKeyValue{0.0}, rightKeyValue{0.0};
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                            pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);

                            if (curve->getPoint(tempKeyPoint.keyFrame)) {
                                Point *point = curve->getPoint(tempKeyPoint.keyFrame);
                                if (point) {
                                    point->setKeyFrame(keyFrame);
                                    point->setLeftKeyFrame(leftKeyFrame);
                                    point->setRightKeyFrame(rightKeyFrame);
                                    point->setDataValue(keyValue);
                                    point->setLeftData(leftKeyValue);
                                    point->setRightData(rightKeyValue);
                                }
                            }
                            tempKeyPoint.setKeyFrame(keyFrame);
                            newKeyPoints.push_front(tempKeyPoint);
                        }
                        VisualCurvePosManager::GetInstance().swapCurKeyPointList(newKeyPoints);
                    }
                }
            }
        }
    }
}

void VisualCurveWidget::multiPointMove(QMouseEvent *event) {
    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    auto selPointMove = [&, this](int index, double offsetX, double offsetY, SAME_KEY_TYPE type) {
        std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

        // caculate keyframe pos
        QList<SKeyPoint> keyPoints;
        SKeyPoint pressPoint;
        VisualCurvePosManager::GetInstance().getKeyPoint(curCurve, index, pressPoint);
        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);

        // judge cur pos 2 keyframe is beyond cur keyframe(int)
        if ((offsetX != 0 && offsetX > eachFrameWidth / 2) && abs(offsetX) < eachFrameWidth) {
            return;
        }
        int keyFrame = pressPoint.keyFrame;
        QPointF movePoint(pressPoint.x + offsetX, pressPoint.y + offsetY);
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, movePoint, keyFrame);

        int moveIndex{keyPoints.size() - 1};
        bool bHandle{false};
        // cur keyframe same as Point
        for (int i{0}; i < keyPoints.size(); ++i) {
            SKeyPoint tempPoint = keyPoints.at(i);
            if (keyFrame == tempPoint.keyFrame) {
                bHandle = true;
                if (i != index) {
                    SAME_KEY_TYPE type = SAME_NONE;
                    if (type == SAME_WITH_NEXT_KEY) {
                        moveIndex = i - 1;
                    } else if (type == SAME_WITH_LAST_KEY) {
                        moveIndex = i + 1;
                    }
                    VisualCurvePosManager::GetInstance().setSameKeyType(type);
                    VisualCurvePosManager::GetInstance().setSameKeyPointIndex(moveIndex);
                } else {
                    moveIndex = i;
                }
                break;
            } else if (keyFrame < tempPoint.keyFrame) {
                bHandle = true;
                VisualCurvePosManager::GetInstance().setSameKeyType(SAME_NONE);
                if (index != i) {
                    // cur keyframe beyond point
                    if (type == SAME_WITH_NEXT_KEY) {
                        moveIndex = i - 1;
                        moveIndex = moveIndex < 0 ? 0 : moveIndex;
                    } else if (type == SAME_WITH_LAST_KEY) {
                        moveIndex = i;
                    }
                } else {
                    // cur keyframe only move
                    moveIndex = i;
                }
                break;
            }
        }
        if (!bHandle) {
            VisualCurvePosManager::GetInstance().setSameKeyType(SAME_NONE);
        }

        // caculate worker point pos
        QPointF leftPoint(pressPoint.leftPoint.x() + offsetX, pressPoint.leftPoint.y() + offsetY);
        QPointF rightPoint(pressPoint.rightPoint.x() + offsetX, pressPoint.rightPoint.y() + offsetY);
        pressPoint.setLeftPoint(leftPoint);
        pressPoint.setRightPoint(rightPoint);

        // fill worker point list
        SKeyPoint newKeyPoint(event->pos().x(), event->pos().y(), pressPoint.type, keyFrame);
        newKeyPoint.setLeftPoint(leftPoint);
        newKeyPoint.setRightPoint(rightPoint);

        keyPoints.replace(index, newKeyPoint);
        keyPoints.move(index, moveIndex);
        VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);
        VisualCurvePosManager::GetInstance().setCurrentPointInfo(moveIndex);

        // caculate worker point keyframe and value
        int leftKeyFrame{0}, rightKeyFrame{0};
        double leftData{0.0}, rightData{0.0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftData);
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightData);

        // handle curve data
        if (CurveManager::GetInstance().getCurve(curCurve)) {
            Curve *curve = CurveManager::GetInstance().getCurve(curCurve);

            if (curve->getPoint(pressPoint.keyFrame)) {
                Point *point = curve->getPoint(pressPoint.keyFrame);
                if (VisualCurvePosManager::GetInstance().getSameKeyType() == SAME_WITH_LAST_KEY) {
                    curve->takePoint(pressPoint.keyFrame);
                    point->setKeyFrame(keyFrame);
                    curve->insertSamePoint(point);
                } else {
                    point->setKeyFrame(keyFrame);
                }
                double value{0};
                if (point->getDataValue().type() == typeid(double)) {
                    value = *(point->getDataValue())._Cast<double>();
                }
                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, movePoint, value);
                point->setDataValue(value);
                point->setLeftData(leftData);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setRightData(rightData);
                point->setRightKeyFrame(rightKeyFrame);
                curve->sortPoint();
            }
        }
    };
    SKeyPoint keyPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
    double offsetX = event->x() - keyPoint.x;
    double offsetY = event->y() - keyPoint.y;
    SAME_KEY_TYPE type;
    if (offsetX >= 0) {
        type = SAME_KEY_TYPE::SAME_WITH_NEXT_KEY;
    } else {
        type = SAME_KEY_TYPE::SAME_WITH_LAST_KEY;
    }
    for (const int &index : VisualCurvePosManager::GetInstance().getMultiSelPoints()) {
        selPointMove(index, offsetX, offsetY, type);
    }
    update();
}

QPointF VisualCurveWidget::reCaculateRightWorkerPoint(SKeyPoint keyPoint, QPointF leftPoint, QPointF rightPoint) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    // caculate offset x,y by trigle
    double leftLength = calculateTrigLen(abs(keyPoint.x - leftPoint.x()), abs(keyPoint.y - leftPoint.y()));
    double rightLength = calculateTrigLen(abs(keyPoint.x - rightPoint.x()), abs(keyPoint.y - rightPoint.y()));
    double offsetY = leftPoint.y() - keyPoint.y;
    double sinValue = abs(offsetY) / leftLength;
    sinValue = (offsetY == 0) ? 0 : sinValue;

    double rightY = rightLength * sinValue;
    double rightX = sqrt(rightLength * rightLength - rightY * rightY);

    // get tangent
    double tangAngle{0};
    if (rightX != 0) {
        double tangValue = rightY / rightX;
        tangAngle = atan(tangValue) * 180 / PI;
        tangAngle = (offsetY >= 0) ? tangAngle : (-tangAngle);
    } else {
        tangAngle = (offsetY >= 0) ? 90 : (-90);
    }

    rightPoint.setX(keyPoint.x + rightX);
    if (keyPoint.y >= leftPoint.y()) {
        rightPoint.setY(keyPoint.y + rightY);
    } else {
        rightPoint.setY(keyPoint.y - rightY);
    }

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);

    double leftKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);

    double rightKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyValue);

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

    // fill point tangent
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setLeftTagent(tangAngle);
            point->setRightTagent(tangAngle);
            point->setLeftData(leftKeyValue);
            point->setLeftKeyFrame(leftKeyFrame);
            point->setRightData(rightKeyValue);
            point->setRightKeyFrame(rightKeyFrame);
        }
    }
    return rightPoint;
}

QPointF VisualCurveWidget::reCaculateLeftWorkerPoint(SKeyPoint keyPoint, QPointF leftPoint, QPointF rightPoint) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    // caculate offset x,y by trigle
    double leftLength = calculateTrigLen(abs(keyPoint.x - leftPoint.x()), abs(keyPoint.y - leftPoint.y()));
    double rightLength = calculateTrigLen(abs(keyPoint.x - rightPoint.x()), abs(keyPoint.y - rightPoint.y()));
    double offsetY = keyPoint.y - rightPoint.y();
    double sinValue = abs(offsetY) / rightLength;
    sinValue = (offsetY == 0) ? 0 : sinValue;

    double letfY = leftLength * sinValue;
    double letfX = sqrt(leftLength * leftLength - letfY * letfY);

    // get tangent
    double tangAngle{0};
    if (letfX != 0) {
        double tangValue = letfY / letfX;
        tangAngle = atan(tangValue) * 180 / PI;
        tangAngle = (offsetY >= 0) ? tangAngle : (-tangAngle);
    } else {
        tangAngle = (offsetY >= 0) ? 90 : (-90);
    }

    leftPoint.setX(keyPoint.x - letfX);
    if (keyPoint.y >= rightPoint.y()) {
        leftPoint.setY(keyPoint.y + letfY);
    } else {
        leftPoint.setY(keyPoint.y - letfY);
    }

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);

    double leftKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);

    double rightKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyValue);

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    // fill point tangent
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setRightTagent(tangAngle);
            point->setLeftTagent(tangAngle);
            point->setLeftData(leftKeyValue);
            point->setLeftKeyFrame(leftKeyFrame);
            point->setRightData(rightKeyValue);
            point->setRightKeyFrame(rightKeyFrame);
        }
    }
    return leftPoint;
}

void VisualCurveWidget::pushState2UndoStack(std::string description) {
    raco::core::UndoState undoState;
    undoState.saveCurrentUndoState();
    commandInterface_->undoStack().push(description, undoState);
}

void VisualCurveWidget::pushMovedState() {
    KEY_BOARD_TYPE keyType = VisualCurvePosManager::GetInstance().getKeyBoardType();
    switch (keyType) {
    case KEY_BOARD_TYPE::POINT_MOVE: {
        MOUSE_PRESS_ACTION pressAct = VisualCurvePosManager::GetInstance().getPressAction();
        if (pressAct == MOUSE_PRESS_LEFT_WORKER_KEY) {
            pushState2UndoStack(fmt::format("move left worker point from'{}', '{}' point",
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().first,
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().second));
        } else if (pressAct == MOUSE_PRESS_RIGHT_WORKER_KEY) {
            pushState2UndoStack(fmt::format("move right worker point from'{}', '{}' point",
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().first,
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().second));
        } else if (pressAct == MOUSE_PRESS_KEY) {
            pushState2UndoStack(fmt::format("move point from '{}', '{}' point",
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().first,
                                            VisualCurvePosManager::GetInstance().getCurrentPointInfo().second));
        }
        break;
    }
    case KEY_BOARD_TYPE::MULTI_POINT_MOVE: {
        pushState2UndoStack(fmt::format("move multi point from '{}'",
                                        VisualCurvePosManager::GetInstance().getCurrentPointInfo().first));
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE_X: {
        pushState2UndoStack(fmt::format("move curve X from '{}'",
                                        VisualCurvePosManager::GetInstance().getCurrentPointInfo().first));
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE_Y: {
        pushState2UndoStack(fmt::format("move curve Y from '{}'",
                                        VisualCurvePosManager::GetInstance().getCurrentPointInfo().first));
        break;
    }
    case KEY_BOARD_TYPE::CURVE_MOVE: {
        pushState2UndoStack(fmt::format("move curve from '{}'",
                                        VisualCurvePosManager::GetInstance().getCurrentPointInfo().first));
        break;
    }
    default:
        break;
    }
}
}
