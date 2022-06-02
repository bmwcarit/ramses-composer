#include "property/PropertyMainWindow.h"

namespace raco::property {

PropertyMainWindows::PropertyMainWindows(QWidget *parent) :
    QMainWindow(parent){

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    sysTitle_ = new PropertyTitileBar("SystemProperty", this);
    sysTitle_->setFixedHeight(30);
    sysTableView_ = new PropertyTableView(PROPERTY_SYSTEM, this);
    connect(sysTitle_, &PropertyTitileBar::addProperty, sysTableView_, &PropertyTableView::addProperty);
    connect(sysTitle_, &PropertyTitileBar::delProperty, sysTableView_, &PropertyTableView::delProperty);
    connect(sysTableView_, &PropertyTableView::propertySelectedChanged, sysTitle_, &PropertyTitileBar::propertySelectedChanged);

    customTitle_ = new PropertyTitileBar("CustomProperty", this);
    customTitle_->setFixedHeight(30);
    customTableView_ = new PropertyTableView(PROPERTY_CUSTOM, this);
    connect(customTitle_, &PropertyTitileBar::addProperty, customTableView_, &PropertyTableView::addProperty);
    connect(customTitle_, &PropertyTitileBar::delProperty, customTableView_, &PropertyTableView::delProperty);
    connect(customTableView_, &PropertyTableView::propertySelectedChanged, customTitle_, &PropertyTitileBar::propertySelectedChanged);

    animationTitle_ = new PropertyTitileBar("AnimationProperty", this);
    animationTitle_->setFixedHeight(30);
    animationTableView_ = new PropertyTableView(PROPERTY_ANIMATION, this);
    connect(animationTitle_, &PropertyTitileBar::addProperty, animationTableView_, &PropertyTableView::addProperty);
    connect(animationTitle_, &PropertyTitileBar::delProperty, animationTableView_, &PropertyTableView::delProperty);
    connect(animationTableView_, &PropertyTableView::propertySelectedChanged, animationTitle_, &PropertyTitileBar::propertySelectedChanged);

    vLayout->addWidget(sysTitle_);
    vLayout->addWidget(sysTableView_);
    vLayout->addWidget(customTitle_);
    vLayout->addWidget(customTableView_);
    vLayout->addWidget(animationTitle_);
    vLayout->addWidget(animationTableView_);

    mainWidget->setLayout(vLayout);
    setCentralWidget(mainWidget);

}


}
