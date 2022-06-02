#include "curve/CurveNameWidget.h"

CurveNameWidget::CurveNameWidget(QWidget *parent) {
    QLabel* label = new QLabel("curve", this);
    curveEdit_ = new QLineEdit(this);
    okBtn_ = new QPushButton("ok", this);
    this->setFixedSize(200, 100);

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(label);
    hLayout->addWidget(curveEdit_);
    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addLayout(hLayout);
    vLayout->addWidget(okBtn_);
	this->setLayout(vLayout);

    connect(okBtn_, &QPushButton::clicked, this, &CurveNameWidget::slotOkBtnClicked);
}

void CurveNameWidget::setBindingData(QString property, QString curve) {
    index_ = 1;
    std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
    if (sampleProperty == std::string()) {
        return;
    }
    while (!isValidCurveStr(sampleProperty, curve.toStdString())) {
        curve += QString::number(index_);
    }
    property_ = property;
    curve_ = curve;
    curveEdit_->setText(curve);
}

void CurveNameWidget::getBindingData(QString &property, QString &curve) {
    property = property_;
    curve = curve_;
}

void CurveNameWidget::slotOkBtnClicked() {
    QString curve = curveEdit_->text();
    std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
    if (sampleProperty == std::string()) {
        return;
    }
    if (!isValidCurveStr(sampleProperty, curve.toStdString())) {
        QMessageBox::critical(this, "Ramses Composer",
            tr("curve name repeat!\n"),
            QMessageBox::Yes);
    } else {
        this->accept();
    }
}

bool CurveNameWidget::isValidCurveStr(std::string sampleProperty, std::string curve) {
    std::map<std::string, std::map<std::string, std::string>> bindingMap;
    bindingMap = NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().bindingMap();
    for (const auto &it : bindingMap) {
        for (const auto &bindingIt : it.second) {
            if (bindingIt.second.compare(curve) == 0) {
                return false;
            }
        }
    }
    return true;
}

