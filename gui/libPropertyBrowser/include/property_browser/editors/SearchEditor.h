#ifndef SEARCHEDITOR_H
#define SEARCHEDITOR_H

#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QDialog>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include "property_browser/PropertyBrowserLayouts.h"

namespace raco::property_browser {
class SearchEditor : public QDialog {
    Q_OBJECT
public:
    explicit SearchEditor(QWidget *parent = nullptr);
    QString getSearchString();
    void setGeoMetry(QWidget* widget);

private:
    PropertyBrowserGridLayout layout_{this};
    QLineEdit search_{this};
    QPushButton acceptButton_{this};
    QPushButton closeButton_{this};
    QString strSearch_;
};
}

#endif // SEARCHEDITOR_H
