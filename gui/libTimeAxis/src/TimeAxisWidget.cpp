#include "time_axis/TimeAxisWidget.h"

#include <QRect>
#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QDebug>
#include "math.h"
#include "common_editors/Int64Editor.h"
#include "time_axis/TimeAxisScrollArea.h"

namespace raco::time_axis {


Q_GLOBAL_STATIC_WITH_ARGS(int, numHeight, (20))
Q_GLOBAL_STATIC_WITH_ARGS(double, PI, (3.14159265))

double calLinerValue(glm::vec2 first, glm::vec2 second, int n) {
    return ((double)n - first.x)/(second.x - first.x)*(second.y - first.y) + first.y;
}

DragPushButton::DragPushButton(QWidget *parent) :
    QWidget(parent),
    curText_(0) {
    setAttribute(Qt::WA_NoSystemBackground, true);
}

void DragPushButton::setText(int num)
{
    curText_ = num;
    QPainter painter(this);
    QFont font("宋体", 8, QFont::Bold, false);
    painter.setFont(font);
    QFontMetrics fm = painter.fontMetrics();
    QString text = QString::number(curText_);
    int textWidth = fm.horizontalAdvance(text);

    if (textWidth + 10 > DEFAULT_BUTTON_WIDTH) {
        resize(textWidth + 10, this->height());
    } else {
        resize(DEFAULT_BUTTON_WIDTH, this->height());
    }
    update();
}

void DragPushButton::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton){
        this->raise(); //将此按钮移动到顶层显示
        this->pressPoint_ = event->pos();
    }
}

void DragPushButton::mouseMoveEvent(QMouseEvent *event) {
    if(event->buttons() == Qt::LeftButton) {
        QPoint tempPoint = this->mapToParent(event->pos() - this->pressPoint_);

        this->move(tempPoint.x(), 0);

        //防止按钮移出父窗口
        if(this->mapToParent(this->rect().topLeft()).x() <= 0) {
            this->move(0, this->pos().y());
            Q_EMIT buttonMove(0);
            QWidget::mouseMoveEvent(event);
            return;
        }
        if(this->mapToParent(this->rect().bottomRight()).x() >= this->parentWidget()->rect().width()) {
            this->move(this->parentWidget()->rect().width() - this->width(), this->pos().y());
            Q_EMIT buttonMove(this->parentWidget()->rect().width() - this->width());
            QWidget::mouseMoveEvent(event);
            return;
        }
        /*if(this->mapToParent(this->rect().topLeft()).y() <= 0) {
            this->move(this->pos().x(), 0);
        }
        if(this->mapToParent(this->rect().bottomRight()).y() >= this->parentWidget()->rect().height()) {
            this->move(this->pos().x(), this->parentWidget()->rect().height() - this->height());
        }
        */
        Q_EMIT buttonMove(tempPoint.x());
    }

    QWidget::mouseMoveEvent(event);
}

void DragPushButton::mouseReleaseEvent(QMouseEvent *event) {
    QPoint tempPoint = this->mapToParent(event->pos() - this->pressPoint_);
    Q_EMIT buttonMoveRelease(tempPoint.x());
    QWidget::mouseReleaseEvent(event);
}

void DragPushButton::paintEvent(QPaintEvent *event) {
    int width = this->width();
    int height = this->height();
    //int height = this->height();

    QPainter painter(this);

    QBrush brush;   //画刷。填充几何图形的调色板，由颜色和填充风格组成
    brush.setColor(QColor(81,123,189,255));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    QPen pen(QColor(81,123,189,255), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    painter.drawRoundedRect(QRect(0, 5, width, *numHeight - 7), 4, 4);
    painter.drawLine(QPoint(width/2, *numHeight), QPoint(width/2, height));

    QFont font("宋体", 8, QFont::Bold, false);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 255));

    QFontMetrics fm = painter.fontMetrics();
    QString text = QString::number(curText_);
    int textWidth = fm.horizontalAdvance(text);

    painter.drawText(QRect(7, 3, textWidth, *numHeight - 5), QString::number(curText_));

    painter.end();
    QWidget::paintEvent(event);
}

TimeAxisWidget::TimeAxisWidget(QWidget *parent, raco::core::CommandInterface* commandInterface, KeyFrameManager *keyFrameManager) :
    QWidget(parent),
    intervalLength_(INTERVAL_LENGTH_DEFAULT),
    keyFrameMgr_(keyFrameManager) {
    button_ = new DragPushButton(parent);
    button_->resize(DEFAULT_BUTTON_WIDTH, this->height());
    connect(button_, &DragPushButton::buttonMove, this, &TimeAxisWidget::updateSlider);

    viewportOffset_.setX(moveNumX_ * intervalLength_);

    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    setFocusPolicy(Qt::StrongFocus);
}

void TimeAxisWidget::refreshKeyFrameView() {
    if (keyFrameMgr_) {
		NodeData* nodeData = NodeDataManager::GetInstance().getActiveNode();
		if (nodeData) {
            std::map < std::string, std::map<std::string, std::string>> bindingMap = nodeData->NodeExtendRef().curveBindingRef().bindingMap();
            keyFrameMgr_->refreshKeyFrameList(bindingMap);
            update();
        }
    }
}

int TimeAxisWidget::getCurrentKeyFrame() {
    return curFrame_;
}

void TimeAxisWidget::paintEvent(QPaintEvent *event) {
    int width = this->width();
    int height = this->height();

    int num = width/intervalLength_;
    int curX = viewportOffset_.x();

    QPainter painter(this);
    painter.fillRect(QRect(QPoint(0, 0), QPoint(width, *numHeight)), QColor(43, 43, 43, 255));
    QFont font("宋体", 8, QFont::Bold, false);
    painter.setFont(font);
    painter.setPen(QColor(188, 188, 188, 255));

    double offsetNum =(moveNumX_ - (int)moveNumX_);
    offsetNum = (offsetNum < 0) ? (-1 - offsetNum) : (1 - offsetNum);

    for(int count = 0; count <= num; count++) {
        int curNumber = qRound((count+moveNumX_+offsetNum)*numTextInterval_);
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

    double eachFrameWidth = (double)intervalLength_/(double)numTextInterval_;
    double animationFrameLength = (finishFrame_ - startFrame_) * eachFrameWidth;
    double animationLeft = startFrame_ * eachFrameWidth - curX;
    double animationRight = finishFrame_ * eachFrameWidth - curX;
    //绘制动画活动区域
    if (curX < animationFrameLength) {
        painter.fillRect(QRect(QPoint(animationLeft, *numHeight), QPoint(animationRight, height)), QColor(66, 66, 66, 255));
        painter.setPen(QColor(0, 0, 0, 255));
        painter.drawLine(QPoint(animationLeft, *numHeight), QPoint(animationLeft, height));
        painter.drawLine(QPoint(animationRight, *numHeight), QPoint(animationRight, height));
    }

    QPen pen(QColor(41, 41, 41, 255), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    //绘制竖线
    for (int count = 0; count <= num; count++) {
        painter.drawLine((count+offsetNum)*intervalLength_, 20, (count+offsetNum)*intervalLength_, height);
    }

    drawKeyFrame(painter);
    painter.end();
    mouseAction_ = MOUSE_NO_ACTION;
    QWidget::paintEvent(event);
}

void TimeAxisWidget::resizeEvent(QResizeEvent *event) {
    button_->resize(button_->size().width(), event->size().height());
    QWidget::resizeEvent(event);
}

void TimeAxisWidget::timerEvent(QTimerEvent *event) {
    if (curLoop_ >= loopCount_) {
        stopAnimation();
        Q_EMIT AnimationStop();
        return;
    }

    if (curFrame_ > finishFrame_) {
        curLoop_++;
        curFrame_ = 0;
        button_->setText(curFrame_);
        button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
        update();
        Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
        return;
    }

    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);

    curFrame_++;
    update();
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void TimeAxisWidget::mousePressEvent(QMouseEvent *event) {
    if (keyFrameMgr_ == nullptr) {
        return;
    }
    if(event->button() == Qt::LeftButton){
        double xPos = event->pos().x();
        int frame = ((double)numTextInterval_ / (double)intervalLength_) * (xPos + viewportOffset_.x());
        if (keyFrameMgr_) {
            keyFrameMgr_->setClickedFrame(frame);
        }
        update();
    }
}

void TimeAxisWidget::keyPressEvent(QKeyEvent *event) {
    if(event->modifiers() == Qt::ControlModifier) {
        if(event->key() == Qt::Key_Tab) {
            Q_EMIT switchCurveType();
        }
    }
    QWidget::keyPressEvent(event);
}

void TimeAxisWidget::keyReleaseEvent(QKeyEvent *event) {

}

void TimeAxisWidget::setViewportRect(const QSize areaSize,
                                     const QPoint viewportOffset,
                                     const double scaleValue,
                                     const MOUSEACTION mouseAction) {
    resize(areaSize);

    scaleValue_ = scaleValue;
    mouseAction_ = mouseAction;

    if (mouseAction_ == MOUSE_LEFT_EXTEND || mouseAction_ == MOUSE_LEFT_MOVE) {
        offsetX_ -= 1;
    } else if (mouseAction_== MOUSE_RIGHT_MOVE || mouseAction_ == MOUSE_RIGHT_EXTEND) {
        offsetX_ += 1;
    }

    //1.计算两条线间隔长度
    //2.计算最上面数字的间隔
    QString strNum = QString::number(numTextInterval_);
    if (mouseAction_ == MOUSE_SCROLL_UP) {
        intervalLength_ += INTERVAL_STEP;
        if (intervalLength_ > INTERVAL_LENGTH_MAX && numTextInterval_ > 1) {
            intervalLength_ = INTERVAL_LENGTH_MIN;
            if (strNum.at(0) == '5') {
                strNum.remove(0, 1);
                strNum.insert(0, "2");
                numTextInterval_ = strNum.toInt();
            } else {
                numTextInterval_ /= 2;
            }
        }
    } else if (mouseAction_ == MOUSE_SCROLL_DOWN) {
        intervalLength_ -= INTERVAL_STEP;
        if (intervalLength_ < INTERVAL_LENGTH_MIN) {
            intervalLength_ = INTERVAL_LENGTH_MAX;
            if (strNum.at(0) == '2') {
                strNum.remove(0, 1);
                strNum.insert(0, "5");
                numTextInterval_ = strNum.toInt();
            } else {
                numTextInterval_ *= 2;
            }
        }
    }

    double frameLength = (finishFrame_ - startFrame_) * ((double)intervalLength_/(double)numTextInterval_);
    double width = this->width();
    moveNumX_ = ((frameLength - width) / 2) / (double)intervalLength_ + offsetX_;

    viewportOffset_.setX(moveNumX_ * intervalLength_);
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    update();
}

void TimeAxisWidget::startAnimation() {
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

void TimeAxisWidget::stopAnimation() {
    killTimer(timerId_);
    timerId_ = 0;
}

void TimeAxisWidget::createKeyFrame() {
    if (keyFrameMgr_) {
        keyFrameMgr_->createKeyFrame(curFrame_);
    }
}

void TimeAxisWidget::updateSlider(int pix) {
    int n = ((double)numTextInterval_ / (double)intervalLength_) * (pix + viewportOffset_.x() + button_->width()/2);
    curFrame_ = n;
    button_->setText(curFrame_);

    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(n);
    update();
}

void TimeAxisWidget::setStartFrame(int keyframe) {
    QObject* object = sender();
    common_editors::Int64Editor *editor = static_cast<common_editors::Int64Editor*>(object);

    startFrame_ = keyframe;
    if (keyframe >= finishFrame_) {
        startFrame_ = keyframe - 1;
        editor->setValue(startFrame_);
        return;
    }
    animationDataManager::GetInstance().getActiveAnimationData().SetStartTime(startFrame_);
    Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();
    update();
}

void TimeAxisWidget::setFinishFrame(int keyframe) {
    QObject* object = sender();
    common_editors::Int64Editor *editor = static_cast<common_editors::Int64Editor*>(object);

    finishFrame_ = keyframe;
    if (keyframe <= startFrame_) {
        finishFrame_ = keyframe + 1;
        editor->setValue(finishFrame_);
        return;
    }
    animationDataManager::GetInstance().getActiveAnimationData().SetEndTime(finishFrame_);
    Q_EMIT Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();
    update();
}

void TimeAxisWidget::setCurFrameToBegin() {
    curFrame_ = startFrame_;
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void TimeAxisWidget::setCurFrameToEnd() {
    curFrame_ = finishFrame_;
    button_->setText(curFrame_);
    button_->move((double)intervalLength_ / (double)numTextInterval_ * (double)curFrame_ - viewportOffset_.x() - button_->width()/2, 0);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(curFrame_);
}

void TimeAxisWidget::clearKeyFrames() {
    keyFrameMgr_->clearKeyFrameList();
    update();
}

//绘制关键帧的点
void TimeAxisWidget::drawKeyFrame(QPainter &painter) {
    if (keyFrameMgr_) {
       QSet<int> keyFrameList = keyFrameMgr_->getMeshNodeKeyFrameList();

       QPen pen(QColor(41, 41, 41, 255), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
       painter.setPen(pen);
       QBrush brush;
       brush.setColor(QColor(245, 250, 250, 255));
       brush.setStyle(Qt::SolidPattern);
       painter.setBrush(brush);

       double n = (double)intervalLength_ / (double)numTextInterval_;
       int curX = viewportOffset_.x();

       for (const int& keyFrame : keyFrameList) {
           int leftPos = n * keyFrame - curX;
           if (leftPos > 0) {
//               if (keyFrameMgr_->getClickedFrame() == keyFrame) {
//                   brush.setColor(QColor(217, 125, 13, 255));
//                   painter.setBrush(brush);
//               }
               painter.drawEllipse(leftPos - 5,20,10,10);
               brush.setColor(QColor(245, 250, 250, 255));
               painter.setBrush(brush);
           }
       }
    }
}
}
