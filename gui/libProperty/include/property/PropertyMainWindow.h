#pragma once

#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>

#include "PropertyTableView.h"
#include "PropertyTitleBar.h"

namespace raco::property {

class PropertyMainWindows : public QMainWindow {
    Q_OBJECT
public:
    PropertyMainWindows(QWidget *parent);

private:
    PropertyTitileBar* sysTitle_;
    PropertyTableView* sysTableView_;

    PropertyTitileBar* customTitle_;
    PropertyTableView* customTableView_;

    PropertyTitileBar* animationTitle_;
    PropertyTableView* animationTableView_;
};
}
