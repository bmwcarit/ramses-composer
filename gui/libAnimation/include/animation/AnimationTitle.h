#ifndef ANIMATIONTITLE_H
#define ANIMATIONTITLE_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpacerItem>
#include "animation/controls/ExpandButton.h"
#include "animation/controls/StringEditor.h"
#include "animation/controls/TagContainerEditor.h"

namespace raco::animation {
class AnimationTitle : public QWidget {
    Q_OBJECT
public:
    AnimationTitle(QWidget *parent = nullptr);
    void setAnimationTitle(QString title);

Q_SIGNALS:
    void animationViewExpanded(bool bIsExpanded);
    void animationIDChanged(QString oldID, QString newID);

public Q_SLOTS:
    void animationViewExpand(bool bIsExpanded);
    void editingFinished();

private:
    raco::animation::ExpandButton *foldBtn_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    raco::animation::TagContainerEditor* label_{nullptr};
    QString animationID_;
};
}

#endif // ANIMATIONTITLE_H
