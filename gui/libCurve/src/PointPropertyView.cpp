#include "curve/PointPropertyView.h"

PointPropertyView::PointPropertyView(Curve *curve, Point *point, QWidget *parent)
    : QWidget{parent}, point_(point) , curve_(curve) {
    initView();
    setLayout(gridLayout_);
}

Point *PointPropertyView::getPoint() {
    return point_;
}

void PointPropertyView::slotKeyFrameChanged() {
    if (curve_) {
        int keyFrame = keyFrameSpinBox_->value();
		if (point_->getKeyFrame() == keyFrame) {
			return;
        }
        if (curve_->modifyPointKeyFrame(point_->getKeyFrame(), keyFrame)) {
            Q_EMIT sigRefreshCurveView();
        } else {
            keyFrameSpinBox_->setValue(point_->getKeyFrame());
        }
    }
}

void PointPropertyView::slotValueChanged() {
    if (point_) {
        double value = valueEditor_->text().toDouble();
        point_->setDataValue(value);
    }
}

void PointPropertyView::slotLeftTangentChanged() {
    if (point_) {
        double value = leftTangEditor_->text().toDouble();
        point_->setLeftTagent(value);
    }
}

void PointPropertyView::slotRightTangentChanged() {
    if (point_) {
        double value = rightTangEditor_->text().toDouble();
        point_->setRightTagent(value);
    }
}

void PointPropertyView::initView() {
    gridLayout_ = new QGridLayout(this);

    QLabel* typeLabel_ = new QLabel(QString("InterPolation Type"), this);
    QLabel* keyFrameLabel_ = new QLabel(QString("KeyFrame"), this);
    QLabel* valueLabel_ = new QLabel(QString("Data"), this);
    QLabel* leftTagentLabel_ = new QLabel(QString("Left Tangent"), this);
    QLabel* rightTagentLabel_ = new QLabel(QString("Right Tangent"), this);

    typeComboBox_ = new QComboBox(this);
    typeComboBox_->addItem(QString("Liner"));
    typeComboBox_->addItem(QString("HERMIT_SPLINE"));
    typeComboBox_->addItem(QString("BESIER_SPLINE"));

    keyFrameSpinBox_ = new QSpinBox(this);
    keyFrameSpinBox_->setRange(INT_MIN, INT_MAX);
    valueEditor_ = new QLineEdit(this);
    leftTangEditor_ = new QLineEdit(this);
    rightTangEditor_ = new QLineEdit(this);

    gridLayout_->addWidget(typeLabel_, 0, 0);
    gridLayout_->addWidget(typeComboBox_, 0, 1);
    gridLayout_->addWidget(keyFrameLabel_, 1, 0);
    gridLayout_->addWidget(keyFrameSpinBox_, 1, 1);
    gridLayout_->addWidget(valueLabel_, 2, 0);
    gridLayout_->addWidget(valueEditor_, 2, 1);
    gridLayout_->addWidget(leftTagentLabel_, 3, 0);
    gridLayout_->addWidget(leftTangEditor_, 3, 1);
    gridLayout_->addWidget(rightTagentLabel_, 4, 0);
    gridLayout_->addWidget(rightTangEditor_, 4, 1);

    if (point_) {
        keyFrameSpinBox_->setValue(point_->getKeyFrame());
        valueEditor_->setText(QString::number(*(point_->getDataValue())._Cast<double>()));
        leftTangEditor_->setText(QString::number(*(point_->getLeftTagent())._Cast<double>()));
        rightTangEditor_->setText(QString::number(*(point_->getRightTagent())._Cast<double>()));

        EInterPolationType type = point_->getInterPolationType();
        switch(type) {
        case LINER: {
            typeComboBox_->setCurrentIndex(LINER);
            leftTangEditor_->setEnabled(false);
            rightTangEditor_->setEnabled(false);
            break;
        }
        case HERMIT_SPLINE: {
            typeComboBox_->setCurrentIndex(HERMIT_SPLINE);
            leftTangEditor_->setEnabled(true);
            rightTangEditor_->setEnabled(true);
            break;
        }
        case BESIER_SPLINE: {
            typeComboBox_->setCurrentIndex(BESIER_SPLINE);
            leftTangEditor_->setEnabled(true);
            rightTangEditor_->setEnabled(true);
            break;
        }
        }

        connect(typeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
            if (index == 0) {
                leftTangEditor_->setEnabled(false);
                rightTangEditor_->setEnabled(false);
            } else {
                leftTangEditor_->setEnabled(true);
                rightTangEditor_->setEnabled(true);
            }
        }
        );

        connect(keyFrameSpinBox_, &QSpinBox::editingFinished, this, &PointPropertyView::slotKeyFrameChanged);
        connect(valueEditor_, &QLineEdit::editingFinished, this, &PointPropertyView::slotValueChanged);
        connect(leftTangEditor_, &QLineEdit::editingFinished, this, &PointPropertyView::slotLeftTangentChanged);
        connect(rightTangEditor_, &QLineEdit::editingFinished, this, &PointPropertyView::slotRightTangentChanged);
    }
}
