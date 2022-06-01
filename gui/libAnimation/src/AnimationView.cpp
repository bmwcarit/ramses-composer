#include "animation/AnimationView.h"

namespace raco::animation {
AnimationView::AnimationView(QWidget *parent) {
    QGridLayout *gLayout = new QGridLayout(this);
    gLayout->setContentsMargins(10, 10, 10, 0);

    startTimeEditor_ = new TagContainerEditor(this);
    startTimeEditor_->setText(QString("StartTime"));
    startTimeStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(startTimeEditor_, 0, 0);
    gLayout->addWidget(startTimeStrEditor_, 0, 1);

    endTimeEditor_ = new TagContainerEditor(this);
    endTimeEditor_->setText(QString("EndTime"));
    endTimeStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(endTimeEditor_, 1, 0);
    gLayout->addWidget(endTimeStrEditor_, 1, 1);

    loopCountEditor_ = new TagContainerEditor(this);
    loopCountEditor_->setText(QString("LoopCount"));
    loopCountStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(loopCountEditor_, 2, 0);
    gLayout->addWidget(loopCountStrEditor_, 2, 1);

    intervalEditor_ = new TagContainerEditor(this);
    intervalEditor_->setText(QString("UpdateInterval"));
    intervalStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(intervalEditor_, 3, 0);
    gLayout->addWidget(intervalStrEditor_, 3, 1);

    playSpeedEditor_ = new TagContainerEditor(this);
    playSpeedEditor_->setText(QString("PlaySpeed"));
    playSpeedStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(playSpeedEditor_, 4, 0);
    gLayout->addWidget(playSpeedStrEditor_, 4, 1);

    returnEditor_ = new TagContainerEditor(this);
    returnEditor_->setText(QString("SampleProperty"));
    returnStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(returnEditor_, 5, 0);
    gLayout->addWidget(returnStrEditor_, 5, 1);

    connect(startTimeStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::startTimeEditorTextChanged);
    connect(endTimeStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::endTimeEditorTextChanged);
    connect(loopCountStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::loopCountEditorTextChanged);
    connect(intervalStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::intervalEditorTextChanged);
    connect(returnStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::returnEditorTextChanged);

    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateActiveAnimation_From_AnimationLogic, this, &AnimationView::slotloadAnimation);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAnimationProperty_From_AnimationLogic, this, &AnimationView::slotUpdateAnimationProperty);
}

void AnimationView::startTimeEditorTextChanged() {
    double dStartTime = startTimeStrEditor_->text().toDouble() * 100.0;
    double dEndTime = endTimeStrEditor_->text().toDouble() * 100.0;
    if (dStartTime >= dEndTime) {
        startTimeStrEditor_->setText(QString::number(dStartTime_ / 100.0));
        return;
    }
    dStartTime_ = dStartTime;
    animationDataManager::GetInstance().getActiveAnimationData().SetStartTime(dStartTime_);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::endTimeEditorTextChanged() {
    double dEndTime = endTimeStrEditor_->text().toDouble() * 100.0;
    double dStartTime = startTimeStrEditor_->text().toDouble() * 100.0;
    if (dStartTime >= dEndTime) {
        endTimeStrEditor_->setText(QString::number(dEndTime_ / 100.0));
        return;
    }
    dEndTime_ = dEndTime;
    animationDataManager::GetInstance().getActiveAnimationData().SetEndTime(dEndTime_);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::loopCountEditorTextChanged() {
    int iLoopCount = loopCountStrEditor_->text().toInt();
    animationDataManager::GetInstance().getActiveAnimationData().SetLoopCount(iLoopCount);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::intervalEditorTextChanged() {
    int iInterval = intervalStrEditor_->text().toInt();
    animationDataManager::GetInstance().getActiveAnimationData().SetUpdateInterval(iInterval);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::returnEditorTextChanged() {
    QString newSampleProperty = returnStrEditor_->text();
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimationKey_From_AnimationUI(sampleProperty_, newSampleProperty);
    sampleProperty_ = newSampleProperty;
}

void AnimationView::slotloadAnimation(QString sampleProperty) {
    animationData tempData = animationDataManager::GetInstance().getAnimationData(sampleProperty.toStdString());
    startTimeStrEditor_->setText(QString::number(tempData.GetStartTime()));
    endTimeStrEditor_->setText(QString::number(tempData.GetEndTime()));
    loopCountStrEditor_->setText(QString::number(tempData.GetLoopCount()));
    intervalStrEditor_->setText(QString::number(tempData.GetUpdateInterval()));
    returnStrEditor_->setText(sampleProperty);
}

void AnimationView::slotUpdateAnimationProperty() {
    animationData tempData = animationDataManager::GetInstance().getActiveAnimationData();
    startTimeStrEditor_->setText(QString::number(tempData.GetStartTime()));
    endTimeStrEditor_->setText(QString::number(tempData.GetEndTime()));
    loopCountStrEditor_->setText(QString::number(tempData.GetLoopCount()));
    intervalStrEditor_->setText(QString::number(tempData.GetUpdateInterval()));
    returnStrEditor_->setText(QString::fromStdString(animationDataManager::GetInstance().GetActiveAnimation()));
}
}
