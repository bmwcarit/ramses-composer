#ifndef ANIMATIONTREEMAINWINDOW_H
#define ANIMATIONTREEMAINWINDOW_H

#include <QMainWindow>
#include "AnimationMainWindow.h"
#include "time_axis/KeyFrame.h"

namespace raco::animation {
class AnimationTreeMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit AnimationTreeMainWindow(QWidget *parent = nullptr);

    void addAnimationView(raco::time_axis::Animation* animation);
    void delAnimationView(QString name);

private:
    QMap<QString, AnimationTreeMainWindow*> mapAnimationViews_;

    QVBoxLayout* vLayout_{nullptr};
};
}

#endif // ANIMATIONTREEMAINWINDOW_H
