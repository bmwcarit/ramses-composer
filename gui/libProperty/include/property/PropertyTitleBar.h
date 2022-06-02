#ifndef PROPERTYTITILEBAR_H
#define PROPERTYTITILEBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace raco::property {
class PropertyTitileBar : public QWidget {
    Q_OBJECT
public:
    PropertyTitileBar(QString title, QWidget * parent);

Q_SIGNALS:
    void addProperty();
    void delProperty();

public Q_SLOTS:
    void propertySelectedChanged(bool bIsSelected);
    void addBtnClicked();
    void delBtnClicked();
private:
    QString title_;

    QPushButton *addBtn_;
    QPushButton *delBtn_;
};
}
#endif // PROPERTYTITILEBAR_H
