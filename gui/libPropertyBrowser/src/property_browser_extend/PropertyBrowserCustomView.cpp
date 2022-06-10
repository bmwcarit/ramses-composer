#include "property_browser_extend/PropertyBrowserCustomView.h"
#include "style/Colors.h"
#include "style/Icons.h"

using namespace raco::style;
namespace raco::property_browser {
PropertyBrowserCustomView::PropertyBrowserCustomView(QWidget *parent)
    : QWidget{parent} {
    title_ = new QWidget{this};
    initCustomView();

    QObject::connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateCustomProperty_From_PropertyUI, this, &PropertyBrowserCustomView::slotUpdateCustomView);
}

void PropertyBrowserCustomView::initCustomView() {
    QHBoxLayout* hLayout = new QHBoxLayout{title_};
    button_  = new ExpandControlNoItemButton{title_, nullptr, this};
    button_->setFixedWidth(28);
    label_ = new QLabel(QString("Name"), title_);

    searchBtn_ = new QPushButton(title_);
    searchBtn_->setFlat(true);
    searchBtn_->setIcon(Icons::instance().typeZoom);

    searchEditor_ = new SearchEditor{this};

    // 下拉框
    nameCombox_ = new QComboBox(title_);
    for (const auto &it : PropertyDataManager::GetInstance().getCustomPropertyTypeMap()) {
        nameCombox_->addItem(QString::fromStdString(it.first));
    }
    // init combobox current index
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    for (int i{nameCombox_->count() - 1}; i >= 0 ; i--) {
        if (pNode) {
            if (!pNode->NodeExtendRef().customPropRef().hasType(nameCombox_->itemText(i).toStdString())) {
                nameCombox_->setCurrentIndex(i);
                lastName_ = nameCombox_->currentText();
            }
        }
    }

    hLayout->addWidget(button_, 0);
    hLayout->addWidget(label_, 0);
    hLayout->addSpacerItem(new QSpacerItem(20, 20));
    hLayout->addWidget(searchBtn_, 0, Qt::AlignLeft);
    hLayout->addWidget(nameCombox_, 1);
    hLayout->setMargin(0);
    title_->setLayout(hLayout);
    QObject::connect(nameCombox_, &QComboBox::currentTextChanged, this, &PropertyBrowserCustomView::slotNameChanged);
    QObject::connect(searchBtn_, &QPushButton::clicked, this, &PropertyBrowserCustomView::slotSearchBtnClicked);

    palette_ = title_->palette();
    view_ = new QStackedWidget{this};

    auto addwidget2HLayout = [this](QWidget* parent, QHBoxLayout* layout, int type)->void {
        layout->setMargin(0);
        layout->setSpacing(0);
        QString title = QString("Value");

        switch (type) {
        case PROPERTY_TYPE_FLOAT: {
            QLabel *label = new QLabel(title, parent);
            QDoubleSpinBox *spinBox = new QDoubleSpinBox(parent);
            spinBox->setStyleSheet("QDoubleSpinBox{up-button:width:0px;down-button:width:0px;}");
            layout->addWidget(label, 1);
            layout->addWidget(spinBox, 2);
            QObject::connect(spinBox, &QDoubleSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotDoubleSpinBoxChanged);
            break;
        }
        case PROPERTY_TYPE_INT:{
            QLabel *label = new QLabel(title, parent);
            QSpinBox *spinBox = new QSpinBox(parent);
            layout->addWidget(label, 1);
            layout->addWidget(spinBox, 2);
            QObject::connect(spinBox, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotSpinBoxChanged);
            break;
        }
        case PROPERTY_TYPE_BOOL:{
            QLabel *label = new QLabel(title, parent);
            QCheckBox *checkBox = new QCheckBox(parent);
            layout->addWidget(label, 1);
            layout->addWidget(checkBox, 2);
            QObject::connect(checkBox, &QCheckBox::clicked, this, &PropertyBrowserCustomView::slotCheckBoxChanged);
            break;
        }
        case PROPERTY_TYPE_STRING:{
            QLabel *label = new QLabel(title, parent);
            QLineEdit *edit = new QLineEdit(parent);
            layout->addWidget(label, 1);
            layout->addWidget(edit, 2);
            QObject::connect(edit, &QLineEdit::editingFinished, this, &PropertyBrowserCustomView::slotStringChanged);
            break;
        }
        default:
            break;
        }
    };

    auto addwidget2GridLayout = [this](QWidget* parent, QGridLayout* layout, int type, int row)->void {
        layout->setMargin(0);
        layout->setSpacing(3);
        QString title = QString("Value") + QString::number(row + 1);

        switch (type) {
        case PROPERTY_TYPE_FLOAT: {
            QLabel *label = new QLabel(title, parent);
            QDoubleSpinBox *spinBox = new QDoubleSpinBox(parent);
            layout->addWidget(label, row, 0);
            layout->addWidget(spinBox, row, 1);
            layout->setColumnStretch(0, 1);
            layout->setColumnStretch(1, 2);
            QObject::connect(spinBox, &QDoubleSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotDoubleSpinBoxChanged);
            break;
        }
        case PROPERTY_TYPE_INT:{
            QLabel *label = new QLabel(title, parent);
            QSpinBox *spinBox = new QSpinBox(parent);
            layout->addWidget(label, row, 0);
            layout->addWidget(spinBox, row, 1);
            layout->setColumnStretch(0, 1);
            layout->setColumnStretch(1, 2);
            QObject::connect(spinBox, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotSpinBoxChanged);
            break;
        }
        default:
            break;
        }
    };

    auto createMtxLayout = [this](QWidget* parent, QGridLayout* layout, int type, int row)->void {
        layout->setMargin(0);
        layout->setSpacing(3);
        switch (type) {
        case PROPERTY_TYPE_MAT2: {
            QDoubleSpinBox* spinBox1 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox2 = new QDoubleSpinBox(parent);
            layout->addWidget(spinBox1, row, 1);
            layout->addWidget(spinBox2, row, 1);
            QObject::connect(spinBox1, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox2, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            break;
        }
        case PROPERTY_TYPE_MAT3: {
            QDoubleSpinBox* spinBox1 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox2 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox3 = new QDoubleSpinBox(parent);
            layout->addWidget(spinBox1, row, 1);
            layout->addWidget(spinBox2, row, 1);
            layout->addWidget(spinBox3, row, 1);
            QObject::connect(spinBox1, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox2, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox3, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            break;
        }
        case PROPERTY_TYPE_MAT4: {
            QDoubleSpinBox* spinBox1 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox2 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox3 = new QDoubleSpinBox(parent);
            QDoubleSpinBox* spinBox4 = new QDoubleSpinBox(parent);
            layout->addWidget(spinBox1, row, 1);
            layout->addWidget(spinBox2, row, 1);
            layout->addWidget(spinBox3, row, 1);
            layout->addWidget(spinBox4, row, 1);
            QObject::connect(spinBox1, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox2, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox3, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            QObject::connect(spinBox4, &QSpinBox::editingFinished, this, &PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged);
            break;
        }
        default:
            break;
        }
    };

    // FLOAT Widget
    QWidget* floatWidget = new QWidget(view_);
    QHBoxLayout* hFloatLayout = new QHBoxLayout(floatWidget);
    addwidget2HLayout(floatWidget, hFloatLayout, PROPERTY_TYPE_FLOAT);
    floatWidget->setLayout(hFloatLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_FLOAT, floatWidget);
    view_->addWidget(floatWidget);

    // Int Widget
    QWidget * intWidget = new QWidget(view_);
    QHBoxLayout* hIntLayout = new QHBoxLayout(intWidget);
    addwidget2HLayout(intWidget, hIntLayout, PROPERTY_TYPE_INT);
    intWidget->setLayout(hIntLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_INT, intWidget);
    view_->addWidget(intWidget);

    // Bool Widget
    QWidget * boolWidget = new QWidget(view_);
    QHBoxLayout* hBoolLayout = new QHBoxLayout(boolWidget);
    addwidget2HLayout(boolWidget, hBoolLayout, PROPERTY_TYPE_BOOL);
    boolWidget->setLayout(hBoolLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_BOOL, boolWidget);
    view_->addWidget(boolWidget);

    // String Widget
    QWidget * stringWidget = new QWidget(view_);
    QHBoxLayout* hStringLayout = new QHBoxLayout(stringWidget);
    addwidget2HLayout(stringWidget, hStringLayout, PROPERTY_TYPE_STRING);
    stringWidget->setLayout(hStringLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_STRING, stringWidget);
    view_->addWidget(stringWidget);

     // SampleTextrue Widget TODO

    // VEC2i Widget
    QWidget * vec2iWidget = new QWidget(view_);
    QGridLayout* gVec2iLayout = new QGridLayout(vec2iWidget);
    addwidget2GridLayout(vec2iWidget, gVec2iLayout, PROPERTY_TYPE_INT, 0);
    addwidget2GridLayout(vec2iWidget, gVec2iLayout, PROPERTY_TYPE_INT, 1);
    vec2iWidget->setLayout(gVec2iLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC2i, vec2iWidget);
    view_->addWidget(vec2iWidget);

    // VEC3i Widget
    QWidget * vec3iWidget = new QWidget(view_);
    QGridLayout* gVec3iLayout = new QGridLayout(vec3iWidget);
    addwidget2GridLayout(vec3iWidget, gVec3iLayout, PROPERTY_TYPE_INT, 0);
    addwidget2GridLayout(vec3iWidget, gVec3iLayout, PROPERTY_TYPE_INT, 1);
    addwidget2GridLayout(vec3iWidget, gVec3iLayout, PROPERTY_TYPE_INT, 2);
    vec3iWidget->setLayout(gVec3iLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC3i, vec3iWidget);
    view_->addWidget(vec3iWidget);

    // VEC4i Widget
    QWidget * vec4iWidget = new QWidget(view_);
    QGridLayout* gVec4iLayout = new QGridLayout(vec4iWidget);
    addwidget2GridLayout(vec4iWidget, gVec4iLayout, PROPERTY_TYPE_INT, 0);
    addwidget2GridLayout(vec4iWidget, gVec4iLayout, PROPERTY_TYPE_INT, 1);
    addwidget2GridLayout(vec4iWidget, gVec4iLayout, PROPERTY_TYPE_INT, 2);
    addwidget2GridLayout(vec4iWidget, gVec4iLayout, PROPERTY_TYPE_INT, 3);
    vec4iWidget->setLayout(gVec4iLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC4i, vec4iWidget);
    view_->addWidget(vec4iWidget);

    // VEC2f Widget
    QWidget * vec2fWidget = new QWidget(view_);
    QGridLayout* gVec2fLayout = new QGridLayout(vec2fWidget);
    addwidget2GridLayout(vec2fWidget, gVec2fLayout, PROPERTY_TYPE_FLOAT, 0);
    addwidget2GridLayout(vec2fWidget, gVec2fLayout, PROPERTY_TYPE_FLOAT, 1);
    vec2fWidget->setLayout(gVec2fLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC2f, vec2fWidget);
    view_->addWidget(vec2fWidget);

    // VEC3f Widget
    QWidget * vec3fWidget = new QWidget(view_);
    QGridLayout* gVec3fLayout = new QGridLayout(vec3fWidget);
    addwidget2GridLayout(vec3fWidget, gVec3fLayout, PROPERTY_TYPE_FLOAT, 0);
    addwidget2GridLayout(vec3fWidget, gVec3fLayout, PROPERTY_TYPE_FLOAT, 1);
    addwidget2GridLayout(vec3fWidget, gVec3fLayout, PROPERTY_TYPE_FLOAT, 2);
    vec3fWidget->setLayout(gVec3fLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC3f, vec3fWidget);
    view_->addWidget(vec3fWidget);

    // VEC4f Widget
    QWidget * vec4fWidget = new QWidget(view_);
    QGridLayout* gVec4fLayout = new QGridLayout(vec4fWidget);
    addwidget2GridLayout(vec4fWidget, gVec4fLayout, PROPERTY_TYPE_FLOAT, 0);
    addwidget2GridLayout(vec4fWidget, gVec4fLayout, PROPERTY_TYPE_FLOAT, 1);
    addwidget2GridLayout(vec4fWidget, gVec4fLayout, PROPERTY_TYPE_FLOAT, 2);
    addwidget2GridLayout(vec4fWidget, gVec4fLayout, PROPERTY_TYPE_FLOAT, 3);
    vec4fWidget->setLayout(gVec4iLayout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_VEC4f, vec4fWidget);
    view_->addWidget(vec4fWidget);

    // MTX2 Widget
    QWidget * mtx2Widget = new QWidget(view_);
    QGridLayout* gMtx2Layout = new QGridLayout(mtx2Widget);
    createMtxLayout(mtx2Widget, gMtx2Layout, EPropertyType::PROPERTY_TYPE_MAT2, 0);
    createMtxLayout(mtx2Widget, gMtx2Layout, EPropertyType::PROPERTY_TYPE_MAT2, 1);
    mtx2Widget->setLayout(gMtx2Layout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_MAT2, mtx2Widget);
    view_->addWidget(mtx2Widget);

    // MTX3 Widget
    QWidget * mtx3Widget = new QWidget(view_);
    QGridLayout* gMtx3Layout = new QGridLayout(mtx3Widget);
    createMtxLayout(mtx3Widget, gMtx3Layout, EPropertyType::PROPERTY_TYPE_MAT3, 0);
    createMtxLayout(mtx3Widget, gMtx3Layout, EPropertyType::PROPERTY_TYPE_MAT3, 1);
    createMtxLayout(mtx3Widget, gMtx3Layout, EPropertyType::PROPERTY_TYPE_MAT3, 2);
    mtx3Widget->setLayout(gMtx3Layout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_MAT3, mtx3Widget);
    view_->addWidget(mtx3Widget);

    // MTX4 Widget
    QWidget * mtx4Widget = new QWidget(view_);
    QGridLayout* gMtx4Layout = new QGridLayout(mtx4Widget);
    createMtxLayout(mtx4Widget, gMtx4Layout, EPropertyType::PROPERTY_TYPE_MAT4, 0);
    createMtxLayout(mtx4Widget, gMtx4Layout, EPropertyType::PROPERTY_TYPE_MAT4, 1);
    createMtxLayout(mtx4Widget, gMtx4Layout, EPropertyType::PROPERTY_TYPE_MAT4, 2);
    createMtxLayout(mtx4Widget, gMtx4Layout, EPropertyType::PROPERTY_TYPE_MAT4, 3);
    mtx4Widget->setLayout(gMtx4Layout);
    propertyTypeWidgetMap_.insert(EPropertyType::PROPERTY_TYPE_MAT4, mtx4Widget);
    view_->addWidget(mtx4Widget);

    // setcurrent widget
    EPropertyType type = EPropertyType::PROPERTY_TYPE_FLOAT;
    if (PropertyDataManager::GetInstance().getCustomProItemType(nameCombox_->currentText().toStdString(), type)) {
        QWidget* tempWidget = propertyTypeWidgetMap_.value(type);
        view_->setCurrentWidget(tempWidget);
    }

    view_->setContentsMargins(35, 0, 0, 0);
    view_->hide();

    auto* vLayout = new PropertyBrowserVBoxLayout{this};
    vLayout->setContentsMargins(5, 0, 0, 0);
    vLayout->addWidget(title_);
    vLayout->addWidget(view_);
    setLayout(vLayout);
}

void PropertyBrowserCustomView::expandedWidget() noexcept {
    if (view_ == nullptr) {
        return;
    }
    bExpanded = !bExpanded;
    if(bExpanded) {
        view_->show();
    }
    else {
        view_->hide();
    }
    Q_EMIT updateIcon(bExpanded);
    update();
}

void PropertyBrowserCustomView::serachCustom(QString search) {
    nameCombox_->clear();
    std::regex regex(search.toStdString());

    for (const auto &it : PropertyDataManager::GetInstance().getCustomPropertyTypeMap()) {
        if (std::regex_search(it.first, regex)) {
            nameCombox_->addItem(QString::fromStdString(it.first));
        }
    }
    if (nameCombox_->count() > 0) {
        nameCombox_->setCurrentIndex(0);
    } else {
        for (const auto &it : PropertyDataManager::GetInstance().getCustomPropertyTypeMap()) {
            nameCombox_->addItem(QString::fromStdString(it.first));
        }
        nameCombox_->setCurrentText(lastName_);
    }
}

void PropertyBrowserCustomView::slotNameChanged(QString name) {
    if (name.isEmpty()) {
        return;
    }
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
        if (lastName_.compare(name) != 0) {
            if (pNode->NodeExtendRef().customPropRef().hasType(name.toStdString())) {
                nameCombox_->setCurrentText(lastName_);
                return;
            }
        }
         // delete last data
        pNode->NodeExtendRef().customPropRef().deleteType(lastName_.toStdString());
        lastName_ = name;
    }

    // switch current widget & insert update data
    EPropertyType type;
    if (PropertyDataManager::GetInstance().getCustomProItemType(name.toStdString(), type)) {
        QWidget* widget = propertyTypeWidgetMap_.value(type);
        view_->setCurrentWidget(widget);
        reLoadCustomProperty(type);
    }
}

void PropertyBrowserCustomView::slotSearchBtnClicked() {
    searchEditor_->setGeoMetry(searchBtn_);
    if (searchEditor_->exec() == QDialog::Accepted) {
        QString search = searchEditor_->getSearchString();
        serachCustom(search);
    }
}

void PropertyBrowserCustomView::slotStringChanged() {
    QLineEdit *edit = dynamic_cast<QLineEdit *>(sender());
    std::any data = std::any_cast<std::string>(edit->text().toStdString());
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
        pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
    }
}

void PropertyBrowserCustomView::slotCheckBoxChanged(bool value) {
    std::any data = std::any_cast<bool>(value);
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
        pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
    }
}

void PropertyBrowserCustomView::slotSpinBoxChanged() {
    EPropertyType type;
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
        if (PropertyDataManager::GetInstance().getCustomProItemType(nameCombox_->currentText().toStdString(), type)) {
            if (type == EPropertyType::PROPERTY_TYPE_INT) {
                QSpinBox *spinBox = dynamic_cast<QSpinBox *>(sender());
                if (spinBox) {
                    std::any data = std::any_cast<int>(spinBox->value());
                    pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
                }
            } else {
                std::vector<int> vector;
                QWidget* widget = propertyTypeWidgetMap_.value(type);
                QGridLayout *layout = dynamic_cast<QGridLayout *>(widget->layout());
                int count = layout->count() / 2;
                for (int i{0}; i < count; i++) {
                    QSpinBox *spinBox = dynamic_cast<QSpinBox *>(layout->itemAtPosition(i, 1)->widget());
                    if (spinBox) {
                        vector.push_back(spinBox->value());
                    }
                }
                std::any data = std::any_cast<std::vector<int>>(vector);
                pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
            }
        }
    }
}

void PropertyBrowserCustomView::slotDoubleSpinBoxChanged() {
    EPropertyType type;
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
        if (PropertyDataManager::GetInstance().getCustomProItemType(nameCombox_->currentText().toStdString(), type)) {
            if (type == EPropertyType::PROPERTY_TYPE_FLOAT) {
                QDoubleSpinBox *spinBox = dynamic_cast<QDoubleSpinBox *>(sender());
                if (spinBox) {
                    std::any data = std::any_cast<double>(spinBox->value());
                    pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
                }
            } else {
                std::vector<double> vector;
                QWidget* widget = propertyTypeWidgetMap_.value(type);
                QGridLayout *layout = dynamic_cast<QGridLayout *>(widget->layout());
                int count = layout->count() / 2;
				for (int i{0}; i < count; i++) {
                    QDoubleSpinBox *spinBox = dynamic_cast<QDoubleSpinBox *>(layout->itemAtPosition(i, 1)->widget());
                    if (spinBox) {
                        vector.push_back(spinBox->value());
                    }
                }
                std::any data = std::any_cast<std::vector<double>>(vector);
                pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
            }
        }
    }
}

void PropertyBrowserCustomView::slotMartixDoubleSpinBoxChanged() {
    EPropertyType type;
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    if (pNode) {
		if (PropertyDataManager::GetInstance().getCustomProItemType(nameCombox_->currentText().toStdString(), type)) {
			std::vector<double> vector;
			QWidget* widget = propertyTypeWidgetMap_.value(type);
			QGridLayout* layout = dynamic_cast<QGridLayout*>(widget->layout());
			int count = layout->count();
			for (int i{0}; i < count; i++) {
				QDoubleSpinBox* spinBox = dynamic_cast<QDoubleSpinBox*>(layout->itemAt(i)->widget());
				if (spinBox) {
					vector.push_back(spinBox->value());
				}
			}
			std::any data = std::any_cast<std::vector<double>>(vector);
			pNode->NodeExtendRef().customPropRef().modifyData(nameCombox_->currentText().toStdString(), data);
		}
    }
}

void PropertyBrowserCustomView::slotUpdateCustomView() {
    QString curText = nameCombox_->currentText();
    nameCombox_->clear();
    for (const auto &it : PropertyDataManager::GetInstance().getCustomPropertyTypeMap()) {
        nameCombox_->addItem(QString::fromStdString(it.first));
        if (curText.compare(QString::fromStdString(it.first)) == 0) {
            nameCombox_->setCurrentText(curText);
        }
    }
}

void PropertyBrowserCustomView::readCustomProperty(QString name, std::any data) {
    lastName_ = name;
    EPropertyType type;
    if (PropertyDataManager::GetInstance().getCustomProItemType(name.toStdString(), type)) {
        switch (type) {
        case PROPERTY_TYPE_FLOAT: {
            if (data.type() == typeid(double)) {
				double value = std::any_cast<double>(data);
				QWidget* widget = propertyTypeWidgetMap_.value(type);
				view_->setCurrentWidget(widget);
				QDoubleSpinBox* spinBox = dynamic_cast<QDoubleSpinBox*>(widget->layout()->itemAt(1)->widget());
				spinBox->setValue(value);
			}
            break;
        }
        case PROPERTY_TYPE_INT: {
			if (data.type() == typeid(int)) {
				int value = std::any_cast<int>(data);
				QWidget* widget = propertyTypeWidgetMap_.value(type);
				view_->setCurrentWidget(widget);
				QSpinBox* spinBox = dynamic_cast<QSpinBox*>(widget->layout()->itemAt(1)->widget());
				spinBox->setValue(value);
			}
            break;
        }
        case PROPERTY_TYPE_BOOL: {
			if (data.type() == typeid(bool)) {
				bool value = std::any_cast<bool>(data);
				QWidget* widget = propertyTypeWidgetMap_.value(type);
				view_->setCurrentWidget(widget);
				QCheckBox* checkBox = dynamic_cast<QCheckBox*>(widget->layout()->itemAt(1)->widget());
				checkBox->setChecked(value);
			}
            break;
        }
        case PROPERTY_TYPE_STRING: {
			if (data.type() == typeid(std::string)) {
				std::string value = std::any_cast<std::string>(data);
				QWidget* widget = propertyTypeWidgetMap_.value(type);
				view_->setCurrentWidget(widget);
				QLineEdit* edit = dynamic_cast<QLineEdit*>(widget->layout()->itemAt(1)->widget());
				edit->setText(QString::fromStdString(value));
			}
            break;
        }
        case PROPERTY_TYPE_VEC2i:
        case PROPERTY_TYPE_VEC3i:
        case PROPERTY_TYPE_VEC4i: {
			if (data.type() == typeid(std::vector<int>)) {
				std::vector<int> vector = std::any_cast<std::vector<int>>(data);
				QWidget* widget = propertyTypeWidgetMap_.value(type);
				view_->setCurrentWidget(widget);
				int count = widget->layout()->count();
				for (int i{0}; i < vector.size(); i++) {
					if (i < count) {
						QGridLayout* layout = dynamic_cast<QGridLayout*>(widget->layout());
						QSpinBox* spinBox = dynamic_cast<QSpinBox*>(layout->itemAtPosition(i, 1)->widget());
						if (spinBox) {
							spinBox->setValue(vector.at(i));
						}
					}
				}
			}
            break;
        }
        case PROPERTY_TYPE_VEC2f:
        case PROPERTY_TYPE_VEC3f:
        case PROPERTY_TYPE_VEC4f: {
			if (data.type() == typeid(std::vector<double>)) {
                std::vector<double> vector = std::any_cast<std::vector<double>>(data);
                QWidget* widget = propertyTypeWidgetMap_.value(type);
                view_->setCurrentWidget(widget);
                int count = widget->layout()->count();
                for (int i{0}; i < vector.size(); i++) {
                    if (i < count) {
                        QGridLayout *layout = dynamic_cast<QGridLayout *>(widget->layout());
                        QDoubleSpinBox *spinBox = dynamic_cast<QDoubleSpinBox *>(layout->itemAtPosition(i, 1)->widget());
                        if (spinBox) {
                            spinBox->setValue(vector.at(i));
                        }
                    }
                }
			}
            break;
        }
        case PROPERTY_TYPE_MAT2:
        case PROPERTY_TYPE_MAT3:
        case PROPERTY_TYPE_MAT4: {
			if (data.type() == typeid(std::vector<double>)) {
			    std::vector<double> vector = std::any_cast<std::vector<double>>(data);
                QWidget* widget = propertyTypeWidgetMap_.value(type);
                view_->setCurrentWidget(widget);
                int count = widget->layout()->count();
                for (int i{0}; i < vector.size(); i++) {
                    if (i < count) {
                        QGridLayout *layout = dynamic_cast<QGridLayout *>(widget->layout());
                        QDoubleSpinBox *spinBox = dynamic_cast<QDoubleSpinBox *>(layout->itemAt(i)->widget());
                        if (spinBox) {
                            spinBox->setValue(vector.at(i));
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    nameCombox_->setCurrentText(name);
}

void PropertyBrowserCustomView::reLoadCustomProperty(EPropertyType type) {
    QString name = nameCombox_->currentText();
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
    QWidget* widget = propertyTypeWidgetMap_.value(type);
    switch (type) {
    case PROPERTY_TYPE_FLOAT: {
        QHBoxLayout *layout = dynamic_cast<QHBoxLayout *>(widget->layout());
        QDoubleSpinBox *spinBox = dynamic_cast<QDoubleSpinBox *>(layout->itemAt(1)->widget());
        if (spinBox) {
            double value = spinBox->value();
            std::any data = std::any_cast<double>(value);
            if (pNode) {
                if (pNode->NodeExtendRef().customPropRef().hasType(name.toStdString())) {
                    pNode->NodeExtendRef().customPropRef().modifyData(name.toStdString(), data);
                } else {
                    pNode->NodeExtendRef().customPropRef().insertType(name.toStdString(), data);
                }
            }
        }
        break;
    }
    case PROPERTY_TYPE_INT: {
        QHBoxLayout *layout = dynamic_cast<QHBoxLayout *>(widget->layout());
        QSpinBox *spinBox = dynamic_cast<QSpinBox *>(layout->itemAt(1)->widget());
        if (spinBox) {
            int value = spinBox->value();
            std::any data = std::any_cast<int>(value);
            if (pNode->NodeExtendRef().customPropRef().hasType(name.toStdString())) {
                pNode->NodeExtendRef().customPropRef().modifyData(name.toStdString(), data);
            } else {
                pNode->NodeExtendRef().customPropRef().insertType(name.toStdString(), data);
            }
        }
        break;
    }
    case PROPERTY_TYPE_BOOL: {
        QHBoxLayout *layout = dynamic_cast<QHBoxLayout *>(widget->layout());
        QCheckBox *checkBox = dynamic_cast<QCheckBox *>(layout->itemAt(1)->widget());
        if (checkBox) {
            bool value = checkBox->isChecked();
            std::any data = std::any_cast<bool>(value);
            if (pNode->NodeExtendRef().customPropRef().hasType(name.toStdString())) {
                pNode->NodeExtendRef().customPropRef().modifyData(name.toStdString(), data);
            } else {
                pNode->NodeExtendRef().customPropRef().insertType(name.toStdString(), data);
            }
        }
        break;
    }
    case PROPERTY_TYPE_STRING: {
        QHBoxLayout *layout = dynamic_cast<QHBoxLayout *>(widget->layout());
        QLineEdit *edit = dynamic_cast<QLineEdit *>(layout->itemAt(1)->widget());
        if (edit) {
            std::string value = edit->text().toStdString();
            std::any data = std::any_cast<std::string>(value);
            if (pNode->NodeExtendRef().customPropRef().hasType(name.toStdString())) {
                pNode->NodeExtendRef().customPropRef().modifyData(name.toStdString(), data);
            } else {
                pNode->NodeExtendRef().customPropRef().insertType(name.toStdString(), data);
            }
        }
        break;
    }
    case PROPERTY_TYPE_VEC2i:
    case PROPERTY_TYPE_VEC3i:
    case PROPERTY_TYPE_VEC4i: {
        slotSpinBoxChanged();
        break;
    }
    case PROPERTY_TYPE_VEC2f:
    case PROPERTY_TYPE_VEC3f:
    case PROPERTY_TYPE_VEC4f: {
        slotDoubleSpinBoxChanged();
        break;
    }
    case PROPERTY_TYPE_MAT2:
    case PROPERTY_TYPE_MAT3:
    case PROPERTY_TYPE_MAT4: {
        slotMartixDoubleSpinBoxChanged();
        break;
    }
    default:
        break;
    }
}

void PropertyBrowserCustomView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (!bIsSelected_) {
            QPalette pal;
            pal.setColor(QPalette::Window, QColor(206, 143, 26, 200));
            title_->setAutoFillBackground(true);
            title_->setPalette(pal);
        } else {
            title_->setAutoFillBackground(true);
            title_->setPalette(palette_);
        }
        bIsSelected_ = !bIsSelected_;
    }
}

}
