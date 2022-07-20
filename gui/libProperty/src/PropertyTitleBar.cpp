#include "property/PropertyTitleBar.h"
#include <QScrollArea>

namespace raco::property {

Q_GLOBAL_STATIC_WITH_ARGS(int, titleHeight, (20))

PropertyTitileBar::PropertyTitileBar(QString title, QWidget * parent) :
    QWidget(parent),title_(title){

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0,0,10,0);
    QLabel *titleLabel = new QLabel(title_, this);

    addBtn_ = new QPushButton("add",this);
    addBtn_->setFixedWidth(30);
    connect(addBtn_, &QPushButton::clicked, this, &PropertyTitileBar::addBtnClicked);
    delBtn_ = new QPushButton("del",this);
    delBtn_->setFixedWidth(30);
    delBtn_->setEnabled(false);
    connect(delBtn_, &QPushButton::clicked, this, &PropertyTitileBar::delBtnClicked);

    hLayout->addWidget(titleLabel);
    hLayout->addSpacerItem(new QSpacerItem(5, 0));
    hLayout->addWidget(addBtn_);
    hLayout->addWidget(delBtn_);

    setStyleSheet("QWidget{background-color:#5d5d5d;}"
                  "QLabel{padding:3px, 2px, 2px, 2px}"
                  "QPushButton{padding:3px, 2px, 2px, 2px;}");
}

void PropertyTitileBar::propertySelectedChanged(bool bIsSelected) {
    if (bIsSelected) {
        delBtn_->setEnabled(true);
    }
    else {
        delBtn_->setEnabled(false);
    }
}

void PropertyTitileBar::addBtnClicked() {
    Q_EMIT addProperty();
}

void PropertyTitileBar::delBtnClicked() {
    Q_EMIT delProperty();
}
}
