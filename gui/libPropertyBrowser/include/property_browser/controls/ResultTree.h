#ifndef RESULTTREE_H
#define RESULTTREE_H

#include "property_browser/PropertyBrowserLayouts.h"
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QDialog>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QListWidget>
#include <QAbstractItemView>

namespace raco::property_browser {
class ResultTree : public QDialog {
    Q_OBJECT
public:
    explicit ResultTree(QWidget *parent = nullptr);
    QString getSelectedCurve();
    void addCurve(QString curve);
    void setGeoMetry(QWidget* widget);
    void clear();

private:
    PropertyBrowserGridLayout layout_{this};
    QPushButton acceptButton_{this};
    QPushButton closeButton_{this};
    QListWidget treeWidget_{this};
    QString selectedCurve_;
};
}

#endif // RESULTTREE_H
