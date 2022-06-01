#ifndef ANIMATIONMAINWINDOW_H
#define ANIMATIONMAINWINDOW_H

#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QToolBar>
#include <QSpacerItem>
#include <QList>
#include "animation/AnimationView.h"
#include "animation/AnimationTitle.h"

namespace raco::animation {
class AnimationMainWindow : public QWidget {
    Q_OBJECT
public:
    AnimationMainWindow(QWidget* parent = nullptr);

private:
    AnimationView* animationView_{nullptr};
};
}

#endif // ANIMATIONMAINWINDOW_H
