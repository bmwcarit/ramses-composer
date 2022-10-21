#include "animation/AnimationView.h"

namespace raco::animation {
AnimationView::AnimationView(QWidget *parent) {
    QGridLayout *gLayout = new QGridLayout(this);
    gLayout->setContentsMargins(10, 10, 10, 0);

    startTimeEditor_ = new TagContainerEditor(this);
    startTimeEditor_->setText(QString("StartTime"));
    startTimeStrEditor_ = new Int64Editor(this);
    gLayout->addWidget(startTimeEditor_, 0, 0);
    gLayout->addWidget(startTimeStrEditor_, 0, 1);

    endTimeEditor_ = new TagContainerEditor(this);
    endTimeEditor_->setText(QString("EndTime"));
    endTimeStrEditor_ = new Int64Editor(this);
    gLayout->addWidget(endTimeEditor_, 1, 0);
    gLayout->addWidget(endTimeStrEditor_, 1, 1);

    loopCountEditor_ = new TagContainerEditor(this);
    loopCountEditor_->setText(QString("LoopCount"));
    loopCountStrEditor_ = new Int64Editor(this);
    loopCountStrEditor_->setRange(1, 10);
    gLayout->addWidget(loopCountEditor_, 2, 0);
    gLayout->addWidget(loopCountStrEditor_, 2, 1);

    intervalEditor_ = new TagContainerEditor(this);
    intervalEditor_->setText(QString("UpdateInterval"));
    intervalStrEditor_ = new Int64Editor(this);
    intervalStrEditor_->setRange(1, 60);
    gLayout->addWidget(intervalEditor_, 3, 0);
    gLayout->addWidget(intervalStrEditor_, 3, 1);

    playSpeedEditor_ = new TagContainerEditor(this);
    playSpeedEditor_->setText(QString("PlaySpeed"));
    playSpeedStrEditor_ = new Int64Editor(this);
    gLayout->addWidget(playSpeedEditor_, 4, 0);
    gLayout->addWidget(playSpeedStrEditor_, 4, 1);

    returnEditor_ = new TagContainerEditor(this);
    returnEditor_->setText(QString("Animation"));
    returnStrEditor_ = new QLineEdit(this);
    gLayout->addWidget(returnEditor_, 5, 0);
    gLayout->addWidget(returnStrEditor_, 5, 1);

    initAnimationView();

    connect(startTimeStrEditor_, &Int64Editor::sigValueChanged, this, &AnimationView::startTimeEditorTextChanged);
    connect(endTimeStrEditor_, &Int64Editor::sigValueChanged, this, &AnimationView::endTimeEditorTextChanged);
    connect(loopCountStrEditor_, &Int64Editor::sigValueChanged, this, &AnimationView::loopCountEditorTextChanged);
    connect(intervalStrEditor_, &Int64Editor::sigValueChanged, this, &AnimationView::intervalEditorTextChanged);
    connect(returnStrEditor_, &QLineEdit::editingFinished, this, &AnimationView::returnEditorTextChanged);
    connect(playSpeedStrEditor_, &Int64Editor::sigValueChanged, this, &AnimationView::playSpeedTextChanged);

    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateActiveAnimation_From_AnimationLogic, this, &AnimationView::slotloadAnimation);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAnimationProperty_From_AnimationLogic, this, &AnimationView::slotUpdateAnimationProperty);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &AnimationView::slotResetAniamitonView);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitAnimationView, this, &AnimationView::initAnimationView);
}

void AnimationView::initAnimationView() {
    QString sampleProperty = QString::fromStdString(animationDataManager::GetInstance().GetActiveAnimation());
    if (!sampleProperty.isEmpty()) {
        slotloadAnimation(sampleProperty);
    }
}

void AnimationView::startTimeEditorTextChanged(int value) {
    double dStartTime = value;
    double dEndTime = endTimeStrEditor_->value();
    if (dStartTime >= dEndTime) {
        startTimeStrEditor_->setValue(dStartTime_);
        return;
    }
    dStartTime_ = dStartTime;
    animationDataManager::GetInstance().getActiveAnimationData().SetStartTime(dStartTime_);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::endTimeEditorTextChanged(int value) {
    double dEndTime = value;
    double dStartTime = startTimeStrEditor_->value();
    if (dStartTime >= dEndTime) {
        endTimeStrEditor_->setValue(dEndTime_);
        return;
    }
    dEndTime_ = dEndTime;
    animationDataManager::GetInstance().getActiveAnimationData().SetEndTime(dEndTime_);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::loopCountEditorTextChanged(int value) {
    animationDataManager::GetInstance().getActiveAnimationData().SetLoopCount(value);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::intervalEditorTextChanged(int value) {
    animationDataManager::GetInstance().getActiveAnimationData().SetUpdateInterval(value);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::returnEditorTextChanged() {
    QString newSampleProperty = returnStrEditor_->text();
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimationKey_From_AnimationUI(sampleProperty_, newSampleProperty);
    sampleProperty_ = newSampleProperty;
}

void AnimationView::playSpeedTextChanged(int value) {
    animationDataManager::GetInstance().getActiveAnimationData().SetPlaySpeed(value);
    Q_EMIT signalProxy::GetInstance().sigUpdateAnimation_From_AnimationUI();
}

void AnimationView::slotloadAnimation(QString sampleProperty) {
    animationData tempData = animationDataManager::GetInstance().getAnimationData(sampleProperty.toStdString());
    startTimeStrEditor_->setValue(tempData.GetStartTime());
    endTimeStrEditor_->setValue(tempData.GetEndTime());
    loopCountStrEditor_->setValue(tempData.GetLoopCount());
    intervalStrEditor_->setValue(tempData.GetUpdateInterval());
    playSpeedStrEditor_->setValue(tempData.GetPlaySpeed());
    returnStrEditor_->setText(sampleProperty);
}

void AnimationView::slotUpdateAnimationProperty() {
    animationData tempData = animationDataManager::GetInstance().getActiveAnimationData();
    startTimeStrEditor_->setValue(tempData.GetStartTime());
    endTimeStrEditor_->setValue(tempData.GetEndTime());
    loopCountStrEditor_->setValue(tempData.GetLoopCount());
    intervalStrEditor_->setValue(tempData.GetUpdateInterval());
    playSpeedStrEditor_->setValue(tempData.GetPlaySpeed());
    returnStrEditor_->setText(QString::fromStdString(animationDataManager::GetInstance().GetActiveAnimation()));
}

void AnimationView::slotResetAniamitonView() {
    startTimeStrEditor_->setValue(0);
    endTimeStrEditor_->setValue(0);
    loopCountStrEditor_->setValue(0);
    intervalStrEditor_->setValue(0);
    playSpeedStrEditor_->setValue(0);
    returnStrEditor_->clear();
}
}
