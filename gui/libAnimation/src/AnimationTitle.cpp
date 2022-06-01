#include "animation/AnimationTitle.h"

#include <QString>

namespace raco::animation {
AnimationTitle::AnimationTitle(QWidget *parent) :
    QWidget(parent) {
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0,0,10,0);

    foldBtn_ = new raco::animation::ExpandButton(this);
    connect(foldBtn_, &ExpandButton::expandBtnClicked, this, &AnimationTitle::animationViewExpand);

    label_ = new raco::animation::TagContainerEditor(this);
    label_->setText(QString("Name"));
    nameEdit_ = new QLineEdit(this);

    hLayout->addWidget(foldBtn_);
    hLayout->addWidget(label_);
    hLayout->addWidget(nameEdit_);

    connect(nameEdit_, &QLineEdit::editingFinished, this, &AnimationTitle::editingFinished);
}

void AnimationTitle::setAnimationTitle(QString title) {
    nameEdit_->setText(title);
    animationID_ = title;
}

void AnimationTitle::animationViewExpand(bool bIsExpanded) {
    if (bIsExpanded) {
        Q_EMIT animationViewExpanded(true);
    }
    else {
        Q_EMIT animationViewExpanded(false);
    }
}

void AnimationTitle::editingFinished() {
    QString newAnimationID = nameEdit_->text();
    Q_EMIT animationIDChanged(animationID_, newAnimationID);
    animationID_ = newAnimationID;
}
}
