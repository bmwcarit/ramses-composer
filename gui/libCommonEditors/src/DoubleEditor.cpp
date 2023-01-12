#include "common_editors/DoubleEditor.h"
#include "qdebug.h"

namespace raco::common_editors {
DoubleEditor::DoubleEditor(QWidget *parent)
    : QWidget{parent} {
    auto* layout = new PropertyBrowserGridLayout{this};
    stack_ = new QStackedWidget{this};

//    stack_->setFixedHeight(20);
    spinBox_ = new DoubleSpinBox{stack_};
    slider_ = new DoubleSlider{stack_};

    slider_->setValue(0);
    spinBox_->setValue(0);

    // connect everything to our item values
    {
        QObject::connect(spinBox_, &DoubleSpinBox::valueEdited, this, [this](double value) {
            if (value < min_) {
                spinBox_->setValue(min_);
                slider_->setValue(min_);
                return;
            }
            if (value > max_) {
                spinBox_->setValue(max_);
                slider_->setValue(max_);
                return;
            }
            spinBox_->setValue(value);
            slider_->setValue(value);
            Q_EMIT sigValueChanged(value);
        });
        QObject::connect(slider_, &DoubleSlider::valueEdited, this, [this](double value) {
            if (value < min_) {
                spinBox_->setValue(min_);
                slider_->setValue(min_);
                return;
            }
            if (value > max_) {
                spinBox_->setValue(max_);
                slider_->setValue(max_);
                return;
            }
            spinBox_->setValue(value);
            slider_->setValue(value);
            Q_EMIT sigValueChanged(value);
        });
    }

    // State change: Show spinbox or slider
    QObject::connect(slider_, &DoubleSlider::singleClicked, this, [this]() { stack_->setCurrentWidget(spinBox_); });
    QObject::connect(slider_, &DoubleSlider::finished, this, [this]() {
        double value = spinBox_->value();
        if (value < min_) {
            spinBox_->setValue(min_);
            slider_->setValue(min_);
            return;
        }
        if (value > max_) {
            spinBox_->setValue(max_);
            slider_->setValue(max_);
            return;
        }
        Q_EMIT sigEditingFinished(); });
    QObject::connect(spinBox_, &DoubleSpinBox::editingFinished, this, [this]() {
        stack_->setCurrentWidget(slider_);
        slider_->clearFocus();
        Q_EMIT sigEditingFinished();
    });

    stack_->addWidget(slider_);
    stack_->addWidget(spinBox_);

    stack_->setCurrentWidget(slider_);
    layout->addWidget(stack_);
    this->setFixedHeight(20);
}

void DoubleEditor::setRange(double min, double max) {
    min_ = min;
    max_ = max;
    spinBox_->setSoftRange(min, max);
    slider_->setSoftRange(min, max);
}

void DoubleEditor::setValue(double value) {
    slider_->setValue(value);
    spinBox_->setValue(value);
}

void DoubleEditor::setWidth(int width) {
    spinBox_->setFixedWidth(width);
    stack_->setFixedWidth(width);
}

void DoubleEditor::setHeight(int height) {
    spinBox_->setFixedHeight(height);
    stack_->setFixedHeight(height);
}

double DoubleEditor::value() {
    return spinBox_->value();
}
}
