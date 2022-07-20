#include "animation/AnimationMainWindow.h"

namespace raco::animation {
AnimationMainWindow::AnimationMainWindow(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setMargin(0);

    animationView_ = new AnimationView(this);
    vLayout->addWidget(animationView_);
    vLayout->addSpacerItem(new QSpacerItem(0, 900));
    setLayout(vLayout);
}
}
