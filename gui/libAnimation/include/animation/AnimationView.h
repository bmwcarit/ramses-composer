#ifndef ANIMATIONVIEW_H
#define ANIMATIONVIEW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpacerItem>
#include "animation/controls/ExpandButton.h"
#include "animation/controls/StringEditor.h"
#include "animation/controls/TagContainerEditor.h"
#include "signal/SignalProxy.h"
#include "AnimationData/animationData.h"

using namespace raco::signal;
using namespace raco::guiData;
namespace raco::animation {
class AnimationView : public QWidget {
    Q_OBJECT
public:
    AnimationView(QWidget* parent = nullptr);

public Q_SLOTS:
    void initAnimationView();
    void startTimeEditorTextChanged();
    void endTimeEditorTextChanged();
    void loopCountEditorTextChanged();
    void intervalEditorTextChanged();
    void returnEditorTextChanged();
    void playSpeedTextChanged();
    void slotloadAnimation(QString sampleProperty);
    void slotUpdateAnimationProperty();
    void slotResetAniamitonView();
private:
    TagContainerEditor* startTimeEditor_{nullptr};
    QLineEdit* startTimeStrEditor_{nullptr};
    TagContainerEditor* endTimeEditor_{nullptr};
    QLineEdit* endTimeStrEditor_{nullptr};
    TagContainerEditor* loopCountEditor_{nullptr};
    QLineEdit* loopCountStrEditor_{nullptr};
    TagContainerEditor* intervalEditor_{nullptr};
    QLineEdit* intervalStrEditor_{nullptr};
    TagContainerEditor* playSpeedEditor_{nullptr};
    QLineEdit* playSpeedStrEditor_{nullptr};
    TagContainerEditor* returnEditor_{nullptr};
    QLineEdit* returnStrEditor_{nullptr};

    int fixedWidth_ = 100;
    double dStartTime_{0};
    double dEndTime_{20000};
    QString sampleProperty_;
};
}

#endif // ANIMATIONVIEW_H
