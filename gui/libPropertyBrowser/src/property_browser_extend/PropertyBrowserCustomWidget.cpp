#include "property_browser_extend/PropertyBrowserCustomWidget.h"
#include <any>

namespace raco::property_browser {
PropertyBrowserCustomWidget::PropertyBrowserCustomWidget(QWidget *parent)
    : QWidget{parent} {
    QWidget* mainWidget = new QWidget(this);
    layout_ = new QGridLayout(this);
    layout_->setSizeConstraint(QVBoxLayout::SetMinAndMaxSize);
    mainWidget->setLayout(layout_);

    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
    scroll_ = new QScrollArea(this);
    scroll_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    scroll_->setWidgetResizable(true);
    scroll_->setWidget(mainWidget);
    vBoxLayout->addWidget(scroll_);
    vBoxLayout->setMargin(0);
    this->setLayout(vBoxLayout);
}

void PropertyBrowserCustomWidget::clearPropertyBrowserCustomWidget() {
    for (QWidget* widget : qAsConst(listWidget_)) {
        layout_->removeWidget(widget);
        if (widget != nullptr) {
            delete widget;
            widget = nullptr;
        }
    }
    listWidget_.clear();
}

void PropertyBrowserCustomWidget::initPropertyBrowserCustomWidget() {
	clearPropertyBrowserCustomWidget();
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	if (pNode == nullptr) {
		return;
    }

    auto customPropMap = pNode->NodeExtendRef().customPropRef().customTypeMapRef();
	for (auto& it : customPropMap) {
        PropertyBrowserCustomView* widget = new PropertyBrowserCustomView(this);
		widget->readCustomProperty(QString::fromStdString(it.first), it.second);
        layout_->addWidget(widget, listWidget_.size(), 0);
		listWidget_.append(widget);
    }
}


void PropertyBrowserCustomWidget::insertData() {
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (listWidget_.size() < 0) {
        return;
    }

    if (listWidget_.size() < PropertyDataManager::GetInstance().getCustomPropertyTypeMap().size()) {
        PropertyBrowserCustomView* widget = new PropertyBrowserCustomView(this);
        layout_->addWidget(widget, layout_->rowCount(), 0);
        listWidget_.append(widget);
        std::string name = widget->getName().toStdString();
        std::any data = std::any_cast<double>(0.0);
        if (pNode)
            pNode->NodeExtendRef().customPropRef().insertType(name, data);
    }
}

void PropertyBrowserCustomWidget::removeData() {
    std::string stdStrSampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
    if (stdStrSampleProperty == std::string()) {
        return;
    }
 	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (listWidget_.size() < 0) {
		return;
	}
    for (int i{0}; i < listWidget_.size(); ++i) {
		if (listWidget_.at(i)->isChecked()) {
			std::string name = listWidget_.at(i)->getName().toStdString();
			if (pNode)
				pNode->NodeExtendRef().customPropRef().deleteType(name);
			layout_->removeWidget(listWidget_.at(i));
			delete listWidget_.at(i);
			listWidget_.removeAt(i);
			i--;
        }
    }
}
}
