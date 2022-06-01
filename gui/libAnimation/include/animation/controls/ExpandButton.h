#ifndef EXPANDBUTTON_H
#define EXPANDBUTTON_H

#pragma once

#include <QPushButton>
#include <QWidget>

namespace raco::animation {

class ExpandButton final : public QPushButton {
    Q_OBJECT
public:
    explicit ExpandButton(QWidget* parent = nullptr);

Q_SIGNALS:
    void expandBtnClicked(bool bClicked);

private Q_SLOTS:
    void updateIcon();

private:
    bool bIsExpanded = false;
};
}

#endif // EXPANDBUTTON_H
