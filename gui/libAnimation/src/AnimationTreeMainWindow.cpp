#include "animation/AnimationTreeMainWindow.h"

namespace raco::animation {
AnimationTreeMainWindow::AnimationTreeMainWindow(QWidget *parent)
    : QMainWindow{parent} {
    QWidget* mainWidget = new QWidget(this);
    vLayout_ = new QVBoxLayout(this);
    vLayout_->setMargin(0);
    mainWidget->setLayout(vLayout_);
    setCentralWidget(mainWidget);
}

void AnimationTreeMainWindow::addAnimationView(raco::time_axis::Animation *animation) {
    if (animation != nullptr && mapAnimationViews_.size() >= 0) {
        AnimationTreeMainWindow* animationWidget = new AnimationTreeMainWindow(this);
        vLayout_->addWidget(animationWidget);
        mapAnimationViews_.insert(animation->getAnimationName(), animationWidget);
    }
}

void AnimationTreeMainWindow::delAnimationView(QString name) {
    if (mapAnimationViews_.contains(name)) {
        AnimationTreeMainWindow* animationWidget = mapAnimationViews_.value(name);
        delete animationWidget;
        animationWidget = nullptr;
        mapAnimationViews_.remove(name);
    }
}
}
