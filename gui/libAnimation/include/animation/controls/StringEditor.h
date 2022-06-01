#ifndef STRINGEDITOR_H
#define STRINGEDITOR_H

#pragma once

#include <QString>
#include <QWidget>
#include <QLineEdit>
#include "core/ErrorItem.h"

namespace raco::animation {

class StringEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool updatedInBackground READ updatedInBackground);
    Q_PROPERTY(int errorLevel READ errorLevel);
public:
    explicit StringEditor(QWidget* parent = nullptr);
    bool updatedInBackground() const;
    int errorLevel() const noexcept;

public:
    void setText(const QString& t);

Q_SIGNALS:
    void textChanged(QString text);

protected:
    bool editingStartedByUser();

    bool updatedInBackground_ = false;
    core::ErrorLevel errorLevel_{core::ErrorLevel::NONE};
    QLineEdit* lineEdit_;
};
}

#endif // STRINGEDITOR_H
