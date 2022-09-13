#include "visual_curve/VisualCurveInfoWidget.h"
#include "visual_curve/VisualCurvePosManager.h"
#include "CurveData/CurveManager.h"
#include "visual_curve/VisualCurveWidget.h"

#define PI (3.14159265358979323846)

using namespace raco::guiData;
namespace raco::visualCurve {
VisualCurveInfoWidget::VisualCurveInfoWidget(QWidget *parent)
    : QWidget{parent} {
    initVisualCurveKeyWidget();
    initVisualCurveCursorWidget();

    centreWidget_ = new QStackedWidget(this);
    centreWidget_->addWidget(visualCurveKeyWidget_);
    centreWidget_->addWidget(visualCurveCursorWidget_);
    centreWidget_->setCurrentWidget(visualCurveCursorWidget_);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(centreWidget_);
    this->setLayout(vLayout);
}

void VisualCurveInfoWidget::initVisualCurveKeyWidget() {
    visualCurveKeyWidget_ = new QWidget{this};
    visualCurveKeyWidget_->setFixedSize(235, 330);

    QLabel *label = new QLabel("Active KeyFrame", visualCurveKeyWidget_);
    QLabel *interpolationLabel = new QLabel("Interpolation", visualCurveKeyWidget_);
    QLabel *keyFrameLabel = new QLabel("Key Frame", visualCurveKeyWidget_);
    QLabel *valueLabel1 = new QLabel("Value", visualCurveKeyWidget_);
    leftValueLabel_ = new QLabel("Value", visualCurveKeyWidget_);
    rightValueLabel_ = new QLabel("Value", visualCurveKeyWidget_);
    leftFrameLabel_ = new QLabel("Left Handle Frame", visualCurveKeyWidget_);
    rightFrameLabel_ = new QLabel("Right Handle Frame", visualCurveKeyWidget_);
    leftTangentLabel_ = new QLabel("Tangent", visualCurveKeyWidget_);
    rightTangentLabel_ = new QLabel("Tangent", visualCurveKeyWidget_);

    interpolationComboBox_ = new QComboBox(visualCurveKeyWidget_);
    interpolationComboBox_->addItem("Liner");
    interpolationComboBox_->addItem("Hermite");
    interpolationComboBox_->addItem("Bezier");
    interpolationComboBox_->addItem("Step");
    keyFrameSpinBox_ = new Int64Editor(visualCurveKeyWidget_);
    keyValueSpinBox_ = new DoubleEditor(visualCurveKeyWidget_);
    leftFrameSpinBox_ = new Int64Editor(visualCurveKeyWidget_);
    leftValueSpinBox_ = new DoubleEditor(visualCurveKeyWidget_);
    rightFrameSpinBox_ = new Int64Editor(visualCurveKeyWidget_);
    rightValueSpinBox_ = new DoubleEditor(visualCurveKeyWidget_);
    leftTangentSpinBox_ = new DoubleEditor(visualCurveKeyWidget_);
    rightTangentSpinBox_ = new DoubleEditor(visualCurveKeyWidget_);

    QGridLayout *keyGridLayout = new QGridLayout{visualCurveKeyWidget_};
    keyGridLayout->setVerticalSpacing(2);
    keyGridLayout->setHorizontalSpacing(10);
    keyGridLayout->addWidget(label, 0, 0, Qt::AlignTop);
    keyGridLayout->addItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    keyGridLayout->addWidget(interpolationLabel, 2, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(interpolationComboBox_, 2, 1);
    keyGridLayout->addItem(new QSpacerItem(0, 8, QSizePolicy::Expanding, QSizePolicy::Minimum), 3, 0);
    keyGridLayout->addWidget(keyFrameLabel, 4, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(keyFrameSpinBox_, 4, 1);
    keyGridLayout->addWidget(valueLabel1, 5, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(keyValueSpinBox_, 5, 1);

    keyGridLayout->addItem(new QSpacerItem(0, 8, QSizePolicy::Expanding, QSizePolicy::Minimum), 6, 0);
    keyGridLayout->addWidget(leftFrameLabel_, 7, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(leftFrameSpinBox_, 7, 1);
    keyGridLayout->addWidget(leftValueLabel_, 8, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(leftValueSpinBox_, 8, 1);
    keyGridLayout->addWidget(leftTangentLabel_, 9, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(leftTangentSpinBox_, 9, 1);

    keyGridLayout->addItem(new QSpacerItem(0, 8, QSizePolicy::Expanding, QSizePolicy::Minimum), 10, 0);
    keyGridLayout->addWidget(rightFrameLabel_, 11, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(rightFrameSpinBox_, 11, 1);
    keyGridLayout->addWidget(rightValueLabel_, 12, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(rightValueSpinBox_, 12, 1);
    keyGridLayout->addWidget(rightTangentLabel_, 13, 0, Qt::AlignLeft);
    keyGridLayout->addWidget(rightTangentSpinBox_, 13, 1);
    keyGridLayout->addItem(new QSpacerItem(0, 200, QSizePolicy::Expanding, QSizePolicy::Minimum), 14, 0);

    visualCurveKeyWidget_->setLayout(keyGridLayout);

    connect(interpolationComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VisualCurveInfoWidget::slotSwitchInterpolationMode);
    connect(keyFrameSpinBox_, &Int64Editor::sigValueChanged, this, &VisualCurveInfoWidget::slotKeyFrameChanged);
    connect(keyValueSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotKeyValueChanged);
    connect(leftFrameSpinBox_, &Int64Editor::sigValueChanged, this, &VisualCurveInfoWidget::slotLeftKeyFrameChanged);
    connect(leftValueSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotLeftKeyValueChanged);
    connect(rightFrameSpinBox_, &Int64Editor::sigValueChanged, this, &VisualCurveInfoWidget::slotRightKeyFrameChanged);
    connect(rightValueSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotRightKeyValueChanged);
    connect(leftTangentSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotLeftTangentChanged);
    connect(rightTangentSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotRightTangentChanged);
}

void VisualCurveInfoWidget::initVisualCurveCursorWidget() {
    visualCurveCursorWidget_ = new QWidget{this};
    visualCurveCursorWidget_->setFixedSize(235, 200);
    QLabel *cursorXLabel = new QLabel("Cursor X", visualCurveCursorWidget_);
    QLabel *cursorYLabel = new QLabel("Y", visualCurveCursorWidget_);

    showCursorCheckBox_ = new QCheckBox("Show Cursor", visualCurveCursorWidget_);
    showCursorCheckBox_->setChecked(true);

    cursorXSpinBox_ = new Int64Editor(visualCurveCursorWidget_);
    cursorYSpinBox_ = new DoubleEditor(visualCurveCursorWidget_);

    cursorLayout_ = new QGridLayout{visualCurveCursorWidget_};
    cursorLayout_->setVerticalSpacing(2);
    cursorLayout_->setHorizontalSpacing(10);
    cursorLayout_->addWidget(showCursorCheckBox_, 0, 0, Qt::AlignTop);
    cursorLayout_->addItem(new QSpacerItem(0, 30, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    cursorLayout_->addWidget(cursorXLabel, 2, 0, Qt::AlignRight);
    cursorLayout_->addWidget(cursorXSpinBox_, 2, 1);
    cursorLayout_->addWidget(cursorYLabel, 3, 0, Qt::AlignRight);
    cursorLayout_->addWidget(cursorYSpinBox_, 3, 1);
    cursorLayout_->addItem(new QSpacerItem(0, 100, QSizePolicy::Expanding, QSizePolicy::Minimum), 4, 0);

    visualCurveCursorWidget_->setLayout(cursorLayout_);

    connect(showCursorCheckBox_, &QCheckBox::clicked, this, &VisualCurveInfoWidget::slotCursorShow);
    connect(cursorXSpinBox_, &Int64Editor::sigValueChanged, this, &VisualCurveInfoWidget::slotCursorXChanged);
    connect(cursorYSpinBox_, &DoubleEditor::sigValueChanged, this, &VisualCurveInfoWidget::slotCursorYChanged);
}

void VisualCurveInfoWidget::setKeyWidgetVisible() {
    centreWidget_->setCurrentWidget(visualCurveKeyWidget_);
    updateSelKey();
}

void VisualCurveInfoWidget::setCursorWidgetVisible() {
    centreWidget_->setCurrentWidget(visualCurveCursorWidget_);
    updateCursorX();
}

void VisualCurveInfoWidget::slotSwitchInterpolationMode(int index) {
    SKeyPoint keyPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        // change keypoint type
        keyPoint.setType(index);
        VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        // change curve data
        if (curve) {
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setInterPolationType(static_cast<EInterPolationType>(index));
            }
        }
        switchCurveType(index);
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotKeyFrameChanged(int value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    QPair<QPointF, QPointF> workerPoint;
    QList<SKeyPoint> keyPoints;
    QList<QPair<QPointF, QPointF>> workerPointList;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {

        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPoints);
        VisualCurvePosManager::GetInstance().getCurWorkerPointList(workerPointList);
        // change curve data
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        if (curve) {
            if (curve->getPoint(value)) {
                return;
            }
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setKeyFrame(value);
            }
            curve->sortPoint();
        }
        // change keyframe pos
        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, value, 0, point);

        // judge current point keyframe beyond lastkey or nextkey
        int index = VisualCurvePosManager::GetInstance().getCurrentPointInfo().second;
        int actualIndex{keyPoints.size() - 1};
        // get actual index
        for (int i{0}; i < keyPoints.size(); ++i) {
            SKeyPoint tempPoint = keyPoints.at(i);
            if (value < tempPoint.keyFrame && index != i) {
                if (keyPoint.keyFrame < tempPoint.keyFrame) {
                    actualIndex = i - 1;
                    actualIndex = actualIndex < 0 ? 0 : actualIndex;
                } else if (keyPoint.keyFrame > tempPoint.keyFrame) {
                    actualIndex = i;
                }
                break;
            }
        }

        // sort keypoints
        double offsetX = point.x() - keyPoint.x;
        keyPoint.setX(point.x());
        keyPoint.setKeyFrame(value);
        keyPoints.replace(index, keyPoint);
        keyPoints.move(index, actualIndex);
        VisualCurvePosManager::GetInstance().swapCurKeyPointList(keyPoints);

        VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint);
        workerPoint.first.setX(workerPoint.first.x() + offsetX);
        workerPoint.second.setX(workerPoint.second.x() + offsetX);
        workerPointList.replace(index, workerPoint);
        workerPointList.move(index, actualIndex);
        VisualCurvePosManager::GetInstance().setCurrentPointInfo(actualIndex);

        VisualCurvePosManager::GetInstance().swapCurWorkerPointList(workerPointList);
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotKeyValueChanged(double value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, keyPoint.keyFrame, value, point);

        double offsetY = point.y() - keyPoint.y;
        keyPoint.setY(point.y());
        VisualCurvePosManager::GetInstance().replaceCurKeyPoint(keyPoint);

        VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint);
        workerPoint.first.setY(workerPoint.first.y() + offsetY);
        workerPoint.second.setY(workerPoint.second.y() + offsetY);
        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);

        // change curve data
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        if (curve) {
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setDataValue(value);
            }
        }
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotLeftKeyFrameChanged(int value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;
    QList<SKeyPoint> keyPointList;
    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        int leftWorkerFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftWorkerFrame);

        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPointList);
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);

        if (value > keyPoint.keyFrame) {
            if (workerPoint.first.y() != workerPoint.second.y()) {
                slotRightKeyFrameChanged(keyPoint.keyFrame);
            }
            return slotLeftKeyFrameChanged(keyPoint.keyFrame);
        }
        if (value == leftWorkerFrame) {
            return;
        }

        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, value, 0, point);
        workerPoint.first.setX(point.x());

        recaculateWorkerLeftPoint(workerPoint, keyPoint);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotLeftKeyValueChanged(double value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, 0, value, point);
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
        workerPoint.first.setY(point.y());

        recaculateWorkerLeftPoint(workerPoint, keyPoint);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotRightKeyFrameChanged(int value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;
    QList<SKeyPoint> keyPointList;
    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        int rightWorkerFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightWorkerFrame);

        VisualCurvePosManager::GetInstance().getCurKeyPointList(keyPointList);
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);

        if (value < keyPoint.keyFrame) {
            if (workerPoint.first.y() != workerPoint.second.y()) {
                slotLeftKeyFrameChanged(keyPoint.keyFrame);
            }
            return slotRightKeyFrameChanged(keyPoint.keyFrame);
        }
        if (value == rightWorkerFrame) {
            return;
        }

        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, value, 0, point);
        workerPoint.second.setX(point.x());

        recaculateWorkerRightPoint(workerPoint, keyPoint);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotRightKeyValueChanged(double value) {
    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        QPointF point;
        keyFrame2PointF(curX, curY, eachFrameWidth, eachValueWidth, 0, value, point);
        SKeyPoint keyPoint;
        VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint);
        workerPoint.second.setY(point.y());

        recaculateWorkerRightPoint(workerPoint, keyPoint);

        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotLeftTangentChanged(double value) {
    if (abs(value) > 90) {
        value = value > 0 ? 90 : -90;
        slotLeftTangentChanged(value);
        return;
    }

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint);

        double leftLength = calculateTrigLen(abs(keyPoint.x - workerPoint.first.x()), abs(keyPoint.y - workerPoint.first.y()));
        double rightLength = calculateTrigLen(abs(keyPoint.x - workerPoint.second.x()), abs(keyPoint.y - workerPoint.second.y()));

        double sinValue = sin(value / (180 / PI));
        double leftY = leftLength * sinValue;
        double leftX = -sqrt(leftLength * leftLength - leftY * leftY);

        double rightY = -(rightLength * sinValue);
        double rightX = sqrt(rightLength * rightLength - rightY * rightY);

        QPointF leftPoint, rightPoint;
        leftPoint.setX(keyPoint.x + leftX);
        leftPoint.setY(keyPoint.y + leftY);
        rightPoint.setX(keyPoint.x + rightX);
        rightPoint.setY(keyPoint.y + rightY);

        // fill worker point list
        qSwap(workerPoint.first, leftPoint);
        qSwap(workerPoint.second, rightPoint);
        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);

        rightTangentSpinBox_->setValue(value);
        int leftKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);
        leftFrameSpinBox_->setValue(leftKeyFrame);

        double leftKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyValue);
        leftValueSpinBox_->setValue(leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);
        rightFrameSpinBox_->setValue(rightKeyFrame);

        double rightKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyValue);
        rightValueSpinBox_->setValue(rightKeyValue);

        if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
            Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setLeftTagent(value);
                point->setRightTagent(value);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setLeftData(leftKeyValue);
                point->setRightKeyFrame(rightKeyFrame);
                point->setRightData(rightKeyValue);
            }
        }
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotRightTangentChanged(double value) {
    if (abs(value) > 90) {
        value = value > 0 ? 90 : -90;
        slotRightTangentChanged(value);
        return;
    }

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    SKeyPoint keyPoint;
    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint)) {
        VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint);

        double leftLength = calculateTrigLen(abs(keyPoint.x - workerPoint.first.x()), abs(keyPoint.y - workerPoint.first.y()));
        double rightLength = calculateTrigLen(abs(keyPoint.x - workerPoint.second.x()), abs(keyPoint.y - workerPoint.second.y()));

        double sinValue = sin(value / (180 / PI));
        double leftY = leftLength * sinValue;
        double leftX = -sqrt(leftLength * leftLength - leftY * leftY);

        double rightY = -(rightLength * sinValue);
        double rightX = sqrt(rightLength * rightLength - rightY * rightY);

        QPointF leftPoint, rightPoint;
        leftPoint.setX(keyPoint.x + leftX);
        leftPoint.setY(keyPoint.y + leftY);
        rightPoint.setX(keyPoint.x + rightX);
        rightPoint.setY(keyPoint.y + rightY);

        // fill worker point list
        qSwap(workerPoint.first, leftPoint);
        qSwap(workerPoint.second, rightPoint);
        VisualCurvePosManager::GetInstance().swapCurWorkerPoint(workerPoint);

        rightTangentSpinBox_->setValue(value);
        int leftKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyFrame);
        leftFrameSpinBox_->setValue(leftKeyFrame);

        double leftKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, leftPoint, leftKeyValue);
        leftValueSpinBox_->setValue(leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyFrame);
        rightFrameSpinBox_->setValue(rightKeyFrame);

        double rightKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, rightPoint, rightKeyValue);
        rightValueSpinBox_->setValue(rightKeyValue);

        if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
            Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
            Point *point = curve->getPoint(keyPoint.keyFrame);
            if (point) {
                point->setLeftTagent(value);
                point->setRightTagent(value);
                point->setLeftKeyFrame(leftKeyFrame);
                point->setLeftData(leftKeyValue);
                point->setRightKeyFrame(rightKeyFrame);
                point->setRightData(rightKeyValue);
            }
        }
        updateSelKey();
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveInfoWidget::slotCursorShow(bool checked) {
    VisualCurvePosManager::GetInstance().setCursorShow(checked);
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveInfoWidget::slotCursorXChanged(int value) {
    VisualCurvePosManager::GetInstance().setCurFrame(value);
    Q_EMIT sigRefreshCursorX();
}

void VisualCurveInfoWidget::slotCursorYChanged(double value) {
    VisualCurvePosManager::GetInstance().setCenterLinePos(value);
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveInfoWidget::slotUpdateSelKey() {
    updateSelKey();
}

void VisualCurveInfoWidget::slotUpdateCursorX() {
    updateCursorX();
}

void VisualCurveInfoWidget::switchCurveType(int type) {
    switch (type) {
    case EInterPolationType::LINER:
    case EInterPolationType::STEP: {
        leftFrameSpinBox_->setVisible(false);
        leftValueSpinBox_->setVisible(false);
        rightFrameSpinBox_->setVisible(false);
        rightValueSpinBox_->setVisible(false);
        leftValueLabel_->setVisible(false);
        rightValueLabel_->setVisible(false);
        leftFrameLabel_->setVisible(false);
        rightFrameLabel_->setVisible(false);
        leftTangentLabel_->setVisible(false);
        rightTangentLabel_->setVisible(false);
        leftTangentSpinBox_->setVisible(false);
        rightTangentSpinBox_->setVisible(false);
        break;
    }
    case EInterPolationType::HERMIT_SPLINE: {
        leftFrameSpinBox_->setVisible(true);
        leftValueSpinBox_->setVisible(true);
        rightFrameSpinBox_->setVisible(true);
        rightValueSpinBox_->setVisible(true);
        leftValueLabel_->setVisible(true);
        rightValueLabel_->setVisible(true);
        leftFrameLabel_->setVisible(true);
        rightFrameLabel_->setVisible(true);
        leftTangentLabel_->setVisible(true);
        rightTangentLabel_->setVisible(true);
        leftTangentSpinBox_->setVisible(true);
        rightTangentSpinBox_->setVisible(true);
        break;
    }
    case EInterPolationType::BESIER_SPLINE: {
        leftFrameSpinBox_->setVisible(true);
        leftValueSpinBox_->setVisible(true);
        rightFrameSpinBox_->setVisible(true);
        rightValueSpinBox_->setVisible(true);
        leftValueLabel_->setVisible(true);
        rightValueLabel_->setVisible(true);
        leftFrameLabel_->setVisible(true);
        rightFrameLabel_->setVisible(true);
        leftTangentLabel_->setVisible(false);
        rightTangentLabel_->setVisible(false);
        leftTangentSpinBox_->setVisible(false);
        rightTangentSpinBox_->setVisible(false);
        break;
    }
    }
}

void VisualCurveInfoWidget::updateSelKey() {
    SKeyPoint keyPoint;
    QPair<QPointF, QPointF> workerPoint;
    if (VisualCurvePosManager::GetInstance().getCurKeyPoint(keyPoint) && VisualCurvePosManager::GetInstance().getCurWorkerPoint(workerPoint)) {
        std::string curve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;

        int curX = VisualCurvePosManager::GetInstance().getCurX();
        int curY = VisualCurvePosManager::GetInstance().getCurY();
        double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
        double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

        interpolationComboBox_->setCurrentIndex(keyPoint.type);
        keyFrameSpinBox_->setValue(keyPoint.keyFrame);

        double value{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, QPointF(keyPoint.x, keyPoint.y), value);
        keyValueSpinBox_->setValue(value);

        int leftKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);
        leftFrameSpinBox_->setValue(leftKeyFrame);

        double leftKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);
        leftValueSpinBox_->setValue(leftKeyValue);

        int rightKeyFrame{0};
        pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);
        rightFrameSpinBox_->setValue(rightKeyFrame);

        double rightKeyValue{0};
        pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);
        rightValueSpinBox_->setValue(rightKeyValue);

        if (CurveManager::GetInstance().getCurve(curve)) {
            if (CurveManager::GetInstance().getCurve(curve)->getPoint(keyPoint.keyFrame)) {
                Point *point = CurveManager::GetInstance().getCurve(curve)->getPoint(keyPoint.keyFrame);
                if (point->getLeftTagent().type() == typeid(double)) {
                    leftTangentSpinBox_->setValue(std::any_cast<double>(point->getLeftTagent()));
                } else if (point->getLeftTagent().type() == typeid(float)) {
                    leftTangentSpinBox_->setValue(std::any_cast<float>(point->getLeftTagent()));
                }
                if (point->getRightTagent().type() == typeid(double)) {
                    rightTangentSpinBox_->setValue(std::any_cast<double>(point->getRightTagent()));
                } else if (point->getRightTagent().type() == typeid(float)) {
                    rightTangentSpinBox_->setValue(std::any_cast<float>(point->getRightTagent()));
                }
            }
        }
        switchCurveType(keyPoint.type);
    }
}

void VisualCurveInfoWidget::updateCursorX() {
    cursorXSpinBox_->setValue(VisualCurvePosManager::GetInstance().getCurFrame());
}

void VisualCurveInfoWidget::recaculateWorkerLeftPoint(QPair<QPointF, QPointF> &workerPoint, time_axis::SKeyPoint keyPoint) {
    // caculate offset x,y by trigle
    double leftLength = calculateTrigLen(abs(keyPoint.x - workerPoint.first.x()), abs(keyPoint.y - workerPoint.first.y()));
    double rightLength = calculateTrigLen(abs(keyPoint.x - workerPoint.second.x()), abs(keyPoint.y - workerPoint.second.y()));
    double offsetY = workerPoint.first.y() - keyPoint.y;
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

    // fill point tangent
    if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setLeftTagent(tangAngle);
            point->setRightTagent(tangAngle);
        }
    }

    workerPoint.second.setX(keyPoint.x + rightX);
    if (keyPoint.y >= workerPoint.first.y()) {
        workerPoint.second.setY(keyPoint.y + rightY);
    } else {
        workerPoint.second.setY(keyPoint.y - rightY);
    }

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);

    double leftKeyValue{0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);

    double rightKeyValue{0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);

    if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setLeftKeyFrame(leftKeyFrame);
            point->setLeftData(leftKeyValue);
            point->setRightKeyFrame(rightKeyFrame);
            point->setRightData(rightKeyValue);
        }
    }
}

void VisualCurveInfoWidget::recaculateWorkerRightPoint(QPair<QPointF, QPointF> &workerPoint, time_axis::SKeyPoint keyPoint) {
    // caculate offset x,y by trigle
    double leftLength = calculateTrigLen(abs(keyPoint.x - workerPoint.first.x()), abs(keyPoint.y - workerPoint.first.y()));
    double rightLength = calculateTrigLen(abs(keyPoint.x - workerPoint.second.x()), abs(keyPoint.y - workerPoint.second.y()));
    double offsetY = keyPoint.y - workerPoint.second.y();
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

    // fill point tangent
    if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setRightTagent(tangAngle);
            point->setLeftTagent(tangAngle);
        }
    }

    workerPoint.first.setX(keyPoint.x - letfX);
    if (keyPoint.y >= workerPoint.second.y()) {
        workerPoint.first.setY(keyPoint.y + letfY);
    } else {
        workerPoint.first.setY(keyPoint.y - letfY);
    }

    int curX = VisualCurvePosManager::GetInstance().getCurX();
    int curY = VisualCurvePosManager::GetInstance().getCurY();
    double eachFrameWidth = VisualCurvePosManager::GetInstance().getEachFrameWidth();
    double eachValueWidth = VisualCurvePosManager::GetInstance().getEachValueWidth();

    int leftKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyFrame);

    double leftKeyValue{0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.first, leftKeyValue);

    int rightKeyFrame{0};
    pointF2KeyFrame(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyFrame);

    double rightKeyValue{0};
    pointF2Value(curX, curY, eachFrameWidth, eachValueWidth, workerPoint.second, rightKeyValue);

    if (CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first)) {
        Curve *curve = CurveManager::GetInstance().getCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        Point *point = curve->getPoint(keyPoint.keyFrame);
        if (point) {
            point->setLeftKeyFrame(leftKeyFrame);
            point->setLeftData(leftKeyValue);
            point->setRightKeyFrame(rightKeyFrame);
            point->setRightData(rightKeyValue);
        }
    }
}
}
