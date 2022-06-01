#ifndef TAGCONTAINEREDITOR_H
#define TAGCONTAINEREDITOR_H

#include <QString>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>

namespace raco::animation {

class TagContainerEditor : public QWidget {
    Q_OBJECT
public:
    explicit TagContainerEditor(QWidget* parent = nullptr);

public Q_SLOTS:
    void setText(const QString&);

private:
    QLabel* label_;
};
}

#endif // TAGCONTAINEREDITOR_H
