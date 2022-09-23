#include "visual_curve/VisualCurveWidget.h"

#include <QRect>
#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QDebug>
#include "math.h"

#include "visual_curve/VisualCurvePosManager.h"
#include "time_axis/TimeAxisScrollArea.h"
#include "signal/SignalProxy.h"

namespace raco::visualCurve {

Q_GLOBAL_STATIC_WITH_ARGS(int, numHeight, (20))
#define PI (3.14159265358979323846)

double calculateTrigLen(double x, double y) {
    return sqrt(x * x + y * y);
}

VisualCurveWidget::VisualCurveWidget(QWidget *parent, raco::core::CommandInterface* commandInterface) :
    QWidget(parent),
    intervalLength_(INTERVAL_LENGTH_DEFAULT) {

    button_ = new DragPushButton(this);
    button_->resize(DEFAULT_BUTTON_WIDTH, this->height());
    button_->setText(curFrame_);

    viewportOffset_.setX(moveNumX_ * intervalLength_);
    viewportOffset_.setY(this->height() / 2);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);

    connect(button_, &DragPushButton::buttonMove, this, &VisualCurveWidget::slotUpdateSlider);
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
    switchHermiteAct_ = new QAction("Switch To Hermite", this);
    connect(linerAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Liner);
    connect(hermiteAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Hermite);
    connect(bezierAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Bezier);
    connect(stepAct_, &QAction::triggered, this, &VisualCurveWidget::slotSwitchPoint2Step);
    connect(switchHermiteAct_, &QAction::triggered, this, &VisualCurveWidget::slotBezier2Hermite);

    menu_->addAction(switchHermiteAct_);
    menu_->addAction(deleteKeyFrameAct_);
    interpolationMenu->addAction(linerAct_);
    interpolationMenu->addAction(hermiteAct_);
    interpolationMenu->addAction(bezierAct_);
    interpolationMenu->addAction(stepAct_);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualCurveWidget::customContextMenuRequested, this, &VisualCurveWidget::slotShowContextMenu);

    QObject::connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigInsertCurve_To_VisualCurve, this, &VisualCurveWidget::slotInsertCurve);
}

void VisualCurveWidget::refreshKeyFrameView() {
    bindingMap_ = getBindingMap();
    updateCurvePoint();
    update();
}

int VisualCurveWidget::getCurrentKeyFrame() {
    return curFrame_;
}

void VisualCurveWidget::insertKeyFrame() {
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
            QPair<QPointF, QPointF> workerPoint(leftPoint, rightPoint);

            QList<SKeyPoint> keyPoints;
            VisualCurvePosManager::GetInstance().getKeyPointList(curveName, keyPoints);
            int index{keyPoints.size()};
            for (int i{0}; i < keyPoints.size(); i++) {
                if (keyPoints.at(i).keyFrame > curFrame_) {
                    index = i;
                    break;
                }
            }

            if (index <= VisualCurvePosManager::GetInstance().getCurrentPointInfo().second) {
                VisualCurvePosManager::GetInstance().setCurrentPointInfo(VisualCurvePosManager::GetInstance().getCurrentPointInfo().second + 1);
            }

            VisualCurvePosManager::GetInstance().insertKeyPoint(index, curveName, keyPoint);
            VisualCurvePosManager::GetInstance().insertWorkerPoint(index, curveName, workerPoint);
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
    QFont font("宋体", 8, QFont::Bold, false);
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
        int letfMovePix = 5;
        painter.drawText(letfMovePix, curY - count*intervalLength_ + letfMovePix, QString::number(curNumber));
    }
    // paint text y down
    for (int count = 1; count <= numYDown; count++) {
        double curNumber = -count * numTextIntervalY_;
        int letfMovePix = 5;
        painter.drawText(letfMovePix, curY + count*intervalLength_ + letfMovePix, QString::number(curNumber));
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

    drawKeyFrame();
    painter.end();
    mouseAction_ = MOUSE_NO_ACTION;
    QWidget::paintEvent(event);
}

void VisualCurveWidget::resizeEvent(QResizeEvent *event) {
    button_->resize(button_->size().width(), event->size().height());
    QWidget::resizeEvent(event);
}

void VisualCurveWidget::timerEvent(QTimerEvent *event) {
    if (curLoop_ >= loopCount_) {
        stopAnimation();
        Q_EMIT AnimationStop();
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

        QPair<QPointF, QPointF> pair;
        if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(pair)) {
            QPointF pointRight = pair.second;
            QPointF pointLeft = pair.first;
            if (abs(pointRight.x() - xPos) <= 3 && abs(pointRight.y() - yPos) <= 3) {
                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_RIGHT_WORKER_KEY);
                VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                Q_EMIT sigPressKey();
                update();
                return;
            } else if (abs(pointLeft.x() - xPos) <= 3 && abs(pointLeft.y() - yPos) <= 3) {
                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_LEFT_WORKER_KEY);
                VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                Q_EMIT sigPressKey();
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
                            if (pressAction_ == KEY_PRESS_ACT::KEY_PRESS_CTRL) {
                                if (VisualCurvePosManager::GetInstance().getKeyBoardType() == MULTI_POINT_MOVE) {
                                    if (i == VisualCurvePosManager::GetInstance().getCurrentPointInfo().second) {
                                        return;
                                    }
                                }
                                if (VisualCurvePosManager::GetInstance().getKeyBoardType() == POINT_MOVE) {
                                    VisualCurvePosManager::GetInstance().setCurrentPointInfo(curve->getCurveName(), i);
                                    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_KEY);
                                    VisualCurvePosManager::GetInstance().addMultiSelPoint(i);
                                    VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::MULTI_POINT_MOVE);
                                    Q_EMIT sigPressKey();
                                    update();
                                    return;
                                }
                                if (!VisualCurvePosManager::GetInstance().hasMultiSelPoint(i)) {
                                    VisualCurvePosManager::GetInstance().addMultiSelPoint(i);
                                    VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::MULTI_POINT_MOVE);
                                    update();
                                    return;
                                } else {
                                    VisualCurvePosManager::GetInstance().delMultiSelPoint(i);
                                    if (VisualCurvePosManager::GetInstance().getMultiSelPoints().empty()) {
                                        VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::POINT_MOVE);
                                    }
                                    update();
                                    return;
                                }
                            } else {
                                if (VisualCurvePosManager::GetInstance().getKeyBoardType() == KEY_BOARD_TYPE::POINT_MOVE) {
                                    VisualCurvePosManager::GetInstance().setCurrentPointInfo(curve->getCurveName(), i);
                                    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_KEY);
                                    VisualCurvePosManager::GetInstance().clearMultiSelPoints();
                                    Q_EMIT sigPressKey();
                                    update();
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
        VisualCurvePosManager::GetInstance().setKeyBoardType(KEY_BOARD_TYPE::POINT_MOVE);
        VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_NONE);
        VisualCurvePosManager::GetInstance().resetCurrentPointInfo();
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
    update();
    QWidget::mouseMoveEvent(event);
}

void VisualCurveWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->MouseButtonRelease) {
        if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY && VisualCurvePosManager::GetInstance().getSameKeyType() != SAME_NONE) {

            std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

            QList<SKeyPoint> keyPoints;
            QList<QPair<QPointF, QPointF> > workerPoints;
            int sameKeyPointIndex = VisualCurvePosManager::GetInstance().getSameKeyPointIndex();
            VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);
            VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);

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
            workerPoints.removeAt(sameKeyPointIndex);
            VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);
            VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPoints);
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

    //1.计算两条线间隔长度
    //2.计算最上面数字的间隔
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
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    reCaculateCurvePoint(oldCurX, oldCurY, oldEachFrameWidth, oldEachValueWidth);
    update();

    VisualCurvePosManager::GetInstance().setCurPos(viewportOffset_.x(), viewportOffset_.y());
    VisualCurvePosManager::GetInstance().setWidth(eachFrameWidth, eachValueWidth);
}

void VisualCurveWidget::startAnimation() {
    if (!timerId_) {
        if (curFrame_ < startFrame_) {
            curFrame_ = startFrame_;
        }

        loopCount_ = animationDataManager::GetInstance().getActiveAnimationData().GetLoopCount();
        if (0 == loopCount_)
            loopCount_ = INT32_MAX;	// 如果loopCount_为0.改成循环int最大值次数
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
                if (point.keyFrame > curFrame_) {
                    index = i;
                    break;
                }
            }

            auto it = pointList.begin();
            while (it != pointList.end()) {
                if ((*it)->getKeyFrame() == curFrame_) {
                    QPointF pointF;
                    double value{0};
                    if ((*it)->getDataValue().type() == typeid(double)) {
                        value = *(*it)->getDataValue()._Cast<double>();
                    }
                    keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);
                    SKeyPoint newPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame());

                    if (index != -1) {
                        VisualCurvePosManager::GetInstance().insertKeyPoint(index, curveName.toStdString(), newPoint);
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
                    QPair<QPointF, QPointF> pair(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()), QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
                    if (index != -1) {
                        VisualCurvePosManager::GetInstance().insertWorkerPoint(index, curveName.toStdString(), pair);
                        int curIndex = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;
                        if (VisualCurvePosManager::GetInstance().getCurrentPointInfo().first.compare(curveName.toStdString()) == 0 && curIndex > index) {
                            VisualCurvePosManager::GetInstance().setCurrentPointInfo(curIndex + 1);
                        }
                    }

                    update();
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
    curFrame_ = n;
    button_->setText(curFrame_);

    VisualCurvePosManager::GetInstance().setCurFrame(curFrame_);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(n);
    Q_EMIT sigUpdateCursorX();
    update();
}

void VisualCurveWidget::slotSetStartFrame() {
    QObject* object = sender();
    QLineEdit* editor = static_cast<QLineEdit*>(object);

    bool enableToInt;
    int tempStart = editor->text().toInt(&enableToInt);

    if (tempStart >= finishFrame_) {
        editor->setText(QString::number(startFrame_));
        return;
    }

    if (enableToInt) {
       startFrame_ = tempStart;
    }
    animationDataManager::GetInstance().getActiveAnimationData().SetStartTime(startFrame_);
    // zz
    Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();

    update();
}

void VisualCurveWidget::slotSetFinishFrame() {
    QObject* object = sender();
    QLineEdit* editor = static_cast<QLineEdit*>(object);

    bool enableToInt;
    int tempFinish = editor->text().toInt(&enableToInt);

    if (tempFinish < startFrame_) {
        editor->setText(QString::number(finishFrame_));
        return;
    }

    if (enableToInt) {
        finishFrame_ = tempFinish;
    }
    animationDataManager::GetInstance().getActiveAnimationData().SetEndTime(finishFrame_);
    //
    Q_EMIT Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();

    update();
}

void VisualCurveWidget::slotSetCurFrameToBegin() {
    curFrame_ = startFrame_;
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void VisualCurveWidget::slotSetCurFrameToEnd() {
    curFrame_ = finishFrame_;
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void VisualCurveWidget::slotClearKeyFrames() {
    VisualCurvePosManager::GetInstance().clearKeyPointMap();
    VisualCurvePosManager::GetInstance().clearWorkerPointMap();
    update();
}

void VisualCurveWidget::slotShowContextMenu(const QPoint &p) {
    if (VisualCurvePosManager::GetInstance().getPressAction() == MOUSE_PRESS_KEY) {
        menu_->exec(mapToGlobal(p));
    }
}

void VisualCurveWidget::slotSwitchPoint2Liner() {
    switchPointInterpolationType(EInterPolationType::LINER);
    Q_EMIT sigUpdateSelKey();
}

void VisualCurveWidget::slotSwitchPoint2Hermite() {
    switchPointInterpolationType(EInterPolationType::HERMIT_SPLINE);
    Q_EMIT sigUpdateSelKey();
}

void VisualCurveWidget::slotSwitchPoint2Bezier() {
    switchPointInterpolationType(EInterPolationType::BESIER_SPLINE);
    Q_EMIT sigUpdateSelKey();
}

void VisualCurveWidget::slotSwitchPoint2Step() {
    switchPointInterpolationType(EInterPolationType::STEP);
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

    QList<QPair<QPointF, QPointF>> workerPoints;
    VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);
    workerPoints.removeAt(index);
    VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPoints);

    // delete curveData point interpolation type
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        curve->delPoint(keyPoint.keyFrame);
        if (curve->getPointList().empty()) {
            CurveManager::GetInstance().delCurve(curCurve);
            Q_EMIT sigDeleteCurve(curCurve);
        }
    }

    VisualCurvePosManager::GetInstance().setCurrentPointInfo(std::string());
    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_NONE);
    update();
    Q_EMIT sigPressKey();
}

void VisualCurveWidget::slotRefreshVisualCurve() {
    update();
}

void VisualCurveWidget::slotRefreshCursorX() {
    curFrame_ = VisualCurvePosManager::GetInstance().getCurFrame();
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextIntervalX_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    update();
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
        QList<QPair<QPointF, QPointF>> pairs;
        QList<SKeyPoint> srcPoints;
        auto it = pointList.begin();
        while (it != pointList.end()) {
            QPointF pointF;
            double value{0};
            if ((*it)->getDataValue().type() == typeid(double)) {
                value = *(*it)->getDataValue()._Cast<double>();
            }
            keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);
            srcPoints.append(SKeyPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame()));

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
            QPair<QPointF, QPointF> pair(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()), QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
            pairs.push_back(pair);
        }
        VisualCurvePosManager::GetInstance().addKeyPointList(curve->getCurveName(), srcPoints);
        VisualCurvePosManager::GetInstance().addWorkerPointList(curve->getCurveName(), pairs);
    }
    update();
}

void VisualCurveWidget::slotBezier2Hermite() {
    std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;

    SKeyPoint lastPoint, nextPoint;
    QPair<QPointF, QPointF> lastWorkerPoint, nextWorkerPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(lastPoint);
    VisualCurvePosManager::GetInstance().getCurWorkerPoint(lastWorkerPoint);
    if (VisualCurvePosManager::GetInstance().getKeyPoint(curve, index + 1, nextPoint)) {
        VisualCurvePosManager::GetInstance().getWorkerPoint(curve, index + 1, nextWorkerPoint);

        QList<QPair<QPointF, QPointF> > workerPoints;
        VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);

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

        double lastWorkerPointX = 3 * (endWorkerPoint.x() - lastPoint.x) + lastPoint.x;
        double lastWorkerPointY = 3 * (endWorkerPoint.y() - lastPoint.y) + lastPoint.y;
        endWorkerPoint.setX(lastWorkerPointX);
        endWorkerPoint.setY(lastWorkerPointY);
        qSwap(lastWorkerPoint.second, endWorkerPoint);
        QPointF leftPoint = reCaculateLeftWorkerPoint(lastPoint, lastWorkerPoint.first, lastWorkerPoint.second);
        qSwap(lastWorkerPoint.first, leftPoint);

        double nextWorkerPointX = nextPoint.x - 3 * (nextPoint.x - startWorkerPoint.x());
        double nextWorkerPointY = nextPoint.y - 3 * (nextPoint.y - startWorkerPoint.y());
        startWorkerPoint.setX(nextWorkerPointX);
        startWorkerPoint.setY(nextWorkerPointY);
        qSwap(nextWorkerPoint.first, startWorkerPoint);
        QPointF rightPoint = reCaculateRightWorkerPoint(nextPoint, nextWorkerPoint.first, nextWorkerPoint.second);
        qSwap(nextWorkerPoint.second, rightPoint);

        workerPoints.replace(index, lastWorkerPoint);
        workerPoints.replace(index + 1, nextWorkerPoint);
        VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPoints);

        switchPointInterpolationType(EInterPolationType::HERMIT_SPLINE);
        Q_EMIT sigUpdateSelKey();
    }
}

void VisualCurveWidget::drawKeyFrame() {
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

    QPainter painter(this);
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
            QPair<QPointF, QPointF> lastWorkerPoint;
            if (VisualCurvePosManager::GetInstance().getWorkerPoint(it.first, i, lastWorkerPoint)) {
                MOUSE_PRESS_ACTION pressAction = VisualCurvePosManager::GetInstance().getPressAction();
                if ((pressAction == MOUSE_PRESS_KEY || pressAction == MOUSE_PRESS_LEFT_WORKER_KEY || pressAction == MOUSE_PRESS_RIGHT_WORKER_KEY) && curCurve == it.first) {
                    QPen tempPen = painterPen(QString::fromStdString(getProperty(it.first)), 2, 255);
                    painter.setPen(tempPen);
                }

                if (i + 1 < it.second.size()) {
                    QPair<QPointF, QPointF> nextWorkerPoint;
                    if (VisualCurvePosManager::GetInstance().getWorkerPoint(it.first, i + 1, nextWorkerPoint)) {
                        SKeyPoint nextPoint = it.second.at(i + 1);
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
                }
                if (i == 0) {
                    SKeyPoint nextPoint = lastPoint;
                    lastPoint.setX(0);
                    drawLiner(painter, it.first, lastPoint, nextPoint);

                }
                if (i == it.second.size() - 1) {
                    SKeyPoint nextPoint(this->width(), lastPoint.y);
                    drawLiner(painter, it.first, lastPoint, nextPoint);
                }

                // paint worker point
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.save();
                drawWorkerPoint(painter, lastPoint, lastWorkerPoint, i, it.first);
            }
        }

        painter.setBrush(brush);
        painter.setPen(pen);
        painter.save();
        // paint keyframe
        for (auto i = 0; i < it.second.size(); i++) {
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

    painter.setBrush(QBrush());
    painter.drawLine(lastPoint.x, lastPoint.y, nextPoint.x, nextPoint.y);
}

void VisualCurveWidget::drawBezier(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint) {
    // beizer curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

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

    // paint bezier curve
    QPainterPath painterPath;
    painterPath.moveTo(destPoints.at(0));
    for (auto i = 1; i < destPoints.size(); i++) {;
        painterPath.lineTo(destPoints.at(i));
    }
    painter.setBrush(QBrush());
    painter.drawPath(painterPath);
}

void VisualCurveWidget::drawHermite(QPainter &painter, std::string curve, SKeyPoint lastPoint, SKeyPoint nextPoint, QPair<QPointF, QPointF> lastWorkerPoint, QPair<QPointF, QPointF> nextWorkerPoint) {
    // hermite curve
    QPointF lastKey(lastPoint.x, lastPoint.y);
    QPointF nextKey(nextPoint.x, nextPoint.y);

    QPointF offsetKey(nextKey.x() - lastKey.x(), nextKey.y() - lastKey.y());

    QPointF endWorkerPoint(lastWorkerPoint.second);
    QPointF startWorkerPoint(nextWorkerPoint.first);
//    if (lastWorkerPoint.second.x() > nextPoint.x) {
//        endWorkerPoint.setX(nextPoint.x);
//        double y = lastPoint.y + (nextPoint.x - lastPoint.x) / (lastWorkerPoint.second.x() - lastPoint.x) * (lastWorkerPoint.second.y() - lastPoint.y);
//        endWorkerPoint.setY(y);
//    }
//    if (nextWorkerPoint.first.x() < lastPoint.x) {
//        startWorkerPoint.setX(lastPoint.x);
//        double y = nextPoint.y + (nextPoint.x - lastPoint.x) / (nextPoint.x - nextWorkerPoint.first.x()) * (nextWorkerPoint.first.y() - nextPoint.y);
//        startWorkerPoint.setY(y);
//    }

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

    // paint hermite curve
    QPainterPath painterPath;
    painterPath.moveTo(destPoints.at(0));
    for (auto i = 1; i < destPoints.size(); i++) {;
        painterPath.lineTo(destPoints.at(i));
    }
    painter.setBrush(QBrush());
    painter.drawPath(painterPath);
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

    painter.setBrush(QBrush());
    painter.drawLine(lastPoint.x, lastPoint.y, nextPoint.x, lastPoint.y);
    painter.drawLine(nextPoint.x, lastPoint.y, nextPoint.x, nextPoint.y);
}

void VisualCurveWidget::drawWorkerPoint(QPainter &painter, SKeyPoint point, QPair<QPointF, QPointF> workerPoint, int index, std::string curve) {
    SKeyPoint lastPoint;
    VisualCurvePosManager::GetInstance().getKeyPoint(curve, index - 1, lastPoint);

    if (point.type == EInterPolationType::HERMIT_SPLINE || point.type == EInterPolationType::BESIER_SPLINE) {
        QPainterPath workerPath;
        MOUSE_PRESS_ACTION pressAction = VisualCurvePosManager::GetInstance().getPressAction();

        if (pressAction == MOUSE_PRESS_NONE) {
            return;
        }

        if (VisualCurvePosManager::GetInstance().getCurrentPointInfo().first == curve && VisualCurvePosManager::GetInstance().getCurrentPointInfo().second == index) {
            if (pressAction == MOUSE_PRESS_LEFT_WORKER_KEY) {
                painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
                auto brush = painter.brush();
                brush.setColor(QColor(255, 255, 255, 255));
                painter.setBrush(brush);
                painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
            } else if (pressAction == MOUSE_PRESS_RIGHT_WORKER_KEY) {
                painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                auto brush = painter.brush();
                brush.setColor(QColor(255, 255, 255, 255));
                painter.setBrush(brush);
                painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
            } else if (pressAction == MOUSE_PRESS_KEY) {
                painter.drawEllipse(QPointF(workerPoint.first.x(), workerPoint.first.y()) , 3, 3);
                painter.drawEllipse(QPointF(workerPoint.second.x(), workerPoint.second.y()) , 3, 3);
            }

            // first point worker path
            workerPath.moveTo(workerPoint.first);
            workerPath.lineTo(workerPoint.second);
            auto pen = painter.pen();
            pen.setColor(QColor(255, 255, 204, 255));
            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawPath(workerPath);
            painter.restore();
        }
    } else {
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

    }
}

void VisualCurveWidget::updateCurvePoint() {
    VisualCurvePosManager::GetInstance().clearKeyPointMap();
    VisualCurvePosManager::GetInstance().clearWorkerPointMap();

    double eachFrameWidth = (double)intervalLength_/(double)numTextIntervalX_;
    double eachValueWidth = (double)intervalLength_/(double)numTextIntervalY_;

    int curX = viewportOffset_.x();
    int curY = viewportOffset_.y();

    for (const auto &curve : CurveManager::GetInstance().getCurveList()) {
        if (curve) {
            std::list<Point*> pointList = curve->getPointList();
            QList<QPair<QPointF, QPointF>> pairs;
            QList<SKeyPoint> srcPoints;
            auto it = pointList.begin();
            while (it != pointList.end()) {
                QPointF pointF;
                double value{0};
                if ((*it)->getDataValue().type() == typeid(double)) {
                    value = *(*it)->getDataValue()._Cast<double>();
                }
                keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, (*it)->getKeyFrame(), value, pointF);
                srcPoints.append(SKeyPoint(pointF.x(), pointF.y(), (*it)->getInterPolationType(), (*it)->getKeyFrame()));

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

                        QPair<QPointF, QPointF> pair(leftPoint, rightPoint);
                        pairs.push_back(pair);
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

                QPair<QPointF, QPointF> pair(QPointF(pointF.x() - offsetLastKey * eachFrameWidth, pointF.y()), QPointF(pointF.x() + offsetNextKey * eachFrameWidth, pointF.y()));
                pairs.push_back(pair);
            }
            VisualCurvePosManager::GetInstance().addKeyPointList(curve->getCurveName(), srcPoints);
            VisualCurvePosManager::GetInstance().addWorkerPointList(curve->getCurveName(), pairs);
        }
    }
}

void VisualCurveWidget::reCaculateCurvePoint(int x, int y, double frameWidth, double valueWidth) {
    QMap<std::string, QList<SKeyPoint>> tempKeyFrameMap;
    QMap<std::string, QList<QPair<QPointF, QPointF>>> tempWorkerPointMap;

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
            QList<QPair<QPointF, QPointF>> pairs;
            QList<SKeyPoint> srcPoints;
            int index{0};
            for (auto point : pointList) {
                QPointF pointF;
                double value{0};
                if (point->getDataValue().type() == typeid(double)) {
                    value = *point->getDataValue()._Cast<double>();
                }
                keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, point->getKeyFrame(), value, pointF);
                srcPoints.append(SKeyPoint(pointF.x(), pointF.y(), point->getInterPolationType(), point->getKeyFrame()));

                SKeyPoint oldKeyPoint;
                if (searchKeyPoint(oldKeyPoint, curve->getCurveName(), point->getKeyFrame())) {
                    double offsetX = pointF.x() - oldKeyPoint.x;
                    double offsetY = pointF.y() - oldKeyPoint.y;

                    QPair<QPointF, QPointF> oldKeyPair;
                    VisualCurvePosManager::GetInstance().getWorkerPoint(curve->getCurveName(), index, oldKeyPair);
                    if (mouseAction_ == MOUSE_SCROLL_UP || mouseAction_ == MOUSE_SCROLL_DOWN) {
                        QPointF leftWorkerPoint = reCaculatePoint(oldKeyPair.first, x, y, frameWidth, valueWidth);
                        QPointF rightWorkerPoint = reCaculatePoint(oldKeyPair.second, x, y, frameWidth, valueWidth);
                        QPair<QPointF, QPointF> pair(leftWorkerPoint, rightWorkerPoint);
                        pairs.push_back(pair);
                    } else {
                        QPair<QPointF, QPointF> pair(QPointF(oldKeyPair.first.x() + offsetX, oldKeyPair.first.y() + offsetY), QPointF(oldKeyPair.second.x() + offsetX, oldKeyPair.second.y() + offsetY));
                        pairs.push_back(pair);
                    }
                }
                index++;
                tempWorkerPointMap.insert(curve->getCurveName(), pairs);
                tempKeyFrameMap.insert(curve->getCurveName(), srcPoints);
            }
        }
    }

    VisualCurvePosManager::GetInstance().setKeyPointMap(tempKeyFrameMap);
    VisualCurvePosManager::GetInstance().setWorkerPointMap(tempWorkerPointMap);
}

void VisualCurveWidget::switchPointInterpolationType(EInterPolationType type) {
    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

    // switch ui point interpolation type
    SKeyPoint keyPoint;
    VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
    keyPoint.type = type;
    VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);
    QPair<QPointF, QPointF> workerPoint;
    VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint);

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);

    double leftKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);

    double rightKeyValue{0.0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);

    // switch curveData point interpolation type
    if (CurveManager::GetInstance().getCurve(curCurve)) {
        Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            curve->delPoint(keyPoint.keyFrame);
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
        Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
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

    keyPoints.replace(index, SKeyPoint(event->pos().x(), event->pos().y(), pressPoint.type, keyFrame));
    keyPoints.move(index, moveIndex);
    VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

    // caculate worker point pos
    QList<QPair<QPointF, QPointF>> workerPointList;
    QPair<QPointF, QPointF> curWorkerPoint;
    VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPointList);
    VisualCurvePosManager::GetInstance().getCurWorkerPoint(curWorkerPoint);

    // fill worker point list
    QPointF leftPoint(curWorkerPoint.first.x() + offsetX, curWorkerPoint.first.y() + offsetY);
    QPointF rightPoint(curWorkerPoint.second.x() + offsetX, curWorkerPoint.second.y() + offsetY);
    qSwap(curWorkerPoint.first, leftPoint);
    qSwap(curWorkerPoint.second, rightPoint);

    workerPointList.replace(index, curWorkerPoint);
    workerPointList.move(index, moveIndex);
    VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPointList);
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
                curve->delPoint(pressPoint.keyFrame);
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

    QPair<QPointF, QPointF> workerPoint;

    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        QPointF pointFLeft;
        QPointF pointFRight = workerPoint.second;
        pointFLeft.setX(event->pos().x());
        pointFLeft.setY(event->pos().y());

        std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
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
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);

        double leftKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);

        double rightKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);

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
        qSwap(workerPoint.first, pointFLeft);
        qSwap(workerPoint.second, pointFRight);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
        update();
    }
}

void VisualCurveWidget::rightWorkerPointMove(QMouseEvent *event) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;

    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        QPointF pointFRight;
        QPointF pointFLeft = workerPoint.first;
        pointFRight.setX(event->pos().x());
        pointFRight.setY(event->pos().y());

        std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
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
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);

        double leftKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);

        double rightKeyValue{0.0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);

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
        qSwap(workerPoint.first, pointFLeft);
        qSwap(workerPoint.second, pointFRight);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
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
            QList<QPair<QPointF, QPointF> > workerPoints, newWorkerPoints;
            if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);

                std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
                if (CurveManager::GetInstance().getCurve(curCurve)) {
                    Curve *curve = CurveManager::GetInstance().getCurve(curCurve);
                    if (offsetX < 0) {
                        for (int i{0}; i < keyPoints.size(); i++) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint = workerPoints.at(i);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), tempWorkerPoint.first.y());
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), tempWorkerPoint.second.y());
                            qSwap(tempWorkerPoint.first, leftPoint);
                            qSwap(tempWorkerPoint.second, rightPoint);
                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                            newWorkerPoints.push_back(tempWorkerPoint);

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
                        VisualCurvePosManager::GetInstance().swapCurWorkerPointList(newWorkerPoints);
                    } else if (offsetX > 0) {
                        for (int i{keyPoints.size() - 1}; i >= 0; i--) {
                            SKeyPoint tempKeyPoint = keyPoints.at(i);
                            tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                            int keyFrame{tempKeyPoint.keyFrame};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                            QPair<QPointF, QPointF> tempWorkerPoint = workerPoints.at(i);
                            QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), tempWorkerPoint.first.y());
                            QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), tempWorkerPoint.second.y());
                            qSwap(tempWorkerPoint.first, leftPoint);
                            qSwap(tempWorkerPoint.second, rightPoint);
                            int leftKeyFrame{0}, rightKeyFrame{0};
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                            pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                            newWorkerPoints.push_front(tempWorkerPoint);

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
                        VisualCurvePosManager::GetInstance().swapCurWorkerPointList(newWorkerPoints);
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
            QList<QPair<QPointF, QPointF> > workerPoints, newWorkerPoints;
            if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);

                std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
                if (CurveManager::GetInstance().getCurve(curCurve)) {
                    Curve *curve = CurveManager::GetInstance().getCurve(curCurve);

                    for (int i{0}; i < keyPoints.size(); i++) {
                        SKeyPoint tempKeyPoint = keyPoints.at(i);
                        tempKeyPoint.setY(tempKeyPoint.y + offsetY);
                        double keyValue{0.0};
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyValue);

                        QPair<QPointF, QPointF> tempWorkerPoint = workerPoints.at(i);
                        QPointF leftPoint(tempWorkerPoint.first.x(), (tempWorkerPoint.first.y() + offsetY));
                        QPointF rightPoint(tempWorkerPoint.second.x(), (tempWorkerPoint.second.y() + offsetY));
                        qSwap(tempWorkerPoint.first, leftPoint);
                        qSwap(tempWorkerPoint.second, rightPoint);
                        double leftKeyValue{0.0}, rightKeyValue{0.0};
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);
                        newWorkerPoints.push_back(tempWorkerPoint);

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
                    VisualCurvePosManager::GetInstance().swapCurWorkerPointList(newWorkerPoints);
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
                QList<QPair<QPointF, QPointF> > workerPoints, newWorkerPoints;
                if (VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints)) {
                    VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPoints);

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

                                QPair<QPointF, QPointF> tempWorkerPoint = workerPoints.at(i);
                                QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), (tempWorkerPoint.first.y() + offsetY));
                                QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), (tempWorkerPoint.second.y() + offsetY));
                                qSwap(tempWorkerPoint.first, leftPoint);
                                qSwap(tempWorkerPoint.second, rightPoint);
                                int leftKeyFrame{0}, rightKeyFrame{0};
                                pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                                pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                                double leftKeyValue{0.0}, rightKeyValue{0.0};
                                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);
                                newWorkerPoints.push_back(tempWorkerPoint);

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
                            VisualCurvePosManager::GetInstance().swapCurWorkerPointList(newWorkerPoints);
                        } else if (offsetX >= 0) {
                            for (int i{keyPoints.size() - 1}; i >= 0; i--) {
                                SKeyPoint tempKeyPoint = keyPoints.at(i);
                                tempKeyPoint.setX(tempKeyPoint.x + offsetX);
                                int keyFrame{tempKeyPoint.keyFrame};
                                tempKeyPoint.setY(tempKeyPoint.y + offsetY);
                                double keyValue{0.0};
                                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyValue);
                                pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, QPointF(tempKeyPoint.x, tempKeyPoint.y), keyFrame);

                                QPair<QPointF, QPointF> tempWorkerPoint = workerPoints.at(i);
                                QPointF leftPoint((tempWorkerPoint.first.x() + offsetX), (tempWorkerPoint.first.y() + offsetY));
                                QPointF rightPoint((tempWorkerPoint.second.x() + offsetX), (tempWorkerPoint.second.y() + offsetY));
                                qSwap(tempWorkerPoint.first, leftPoint);
                                qSwap(tempWorkerPoint.second, rightPoint);
                                int leftKeyFrame{0}, rightKeyFrame{0};
                                pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyFrame);
                                pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyFrame);
                                double leftKeyValue{0.0}, rightKeyValue{0.0};
                                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.first, leftKeyValue);
                                pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, tempWorkerPoint.second, rightKeyValue);
                                newWorkerPoints.push_front(tempWorkerPoint);

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
                            VisualCurvePosManager::GetInstance().swapCurWorkerPointList(newWorkerPoints);
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

        keyPoints.replace(index, SKeyPoint(event->pos().x(), event->pos().y(), pressPoint.type, keyFrame));
        keyPoints.move(index, moveIndex);
        VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

        // caculate worker point pos
        QList<QPair<QPointF, QPointF>> workerPointList;
        QPair<QPointF, QPointF> curWorkerPoint;
        VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPointList);
        VisualCurvePosManager::GetInstance().getCurWorkerPoint(curWorkerPoint);

        // fill worker point list
        QPointF leftPoint(curWorkerPoint.first.x() + offsetX, curWorkerPoint.first.y() + offsetY);
        QPointF rightPoint(curWorkerPoint.second.x() + offsetX, curWorkerPoint.second.y() + offsetY);
        qSwap(curWorkerPoint.first, leftPoint);
        qSwap(curWorkerPoint.second, rightPoint);

        workerPointList.replace(index, curWorkerPoint);
        workerPointList.move(index, moveIndex);
        VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPointList);
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
                    curve->delPoint(pressPoint.keyFrame);
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
    for (int index : VisualCurvePosManager::GetInstance().getMultiSelPoints()) {
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
    qDebug() << "22222:" << keyPoint.x << keyPoint.y << rightPoint.x() << rightPoint.y() << leftPoint.x() << leftPoint.y();

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

    qDebug() << "11111:" << keyPoint.x << keyPoint.y << rightPoint.x() << rightPoint.y() << leftPoint.x() << leftPoint.y();

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
}
