#include "property_browser_extend/PropertyBrowserCurveBindingView.h"
#include "style/Colors.h"
#include "style/Icons.h"

using namespace raco::style;
namespace raco::property_browser {
PropertyBrowserCurveBindingView::PropertyBrowserCurveBindingView(QString sampleProperty, QString property, QString curve, QWidget *parent)
    : QWidget{parent},
      sampleProperty_{sampleProperty},
      property_{property},
      curve_{curve} {
    title_ = new QWidget{this};
    auto* hLayout = new PropertyBrowserHBoxLayout{title_};
    button_  = new ExpandControlNoItemButton{title_, nullptr, nullptr, this};
    button_->setFixedWidth(28);
    propertyLabel_ = new QLabel(QString("Property"), title_);
    propertyEditor_ = new QLineEdit(title_);
    hLayout->addWidget(button_, 0);
    hLayout->addWidget(propertyLabel_, 0);
    hLayout->addSpacerItem(new QSpacerItem(20, 20));
    hLayout->addWidget(propertyEditor_, 1);
    title_->setLayout(hLayout);

    view_ = new QWidget{this};
    auto* viewLayout = new QGridLayout{view_};
    curveLabel_ = new QLabel(QString("curve"), view_);
    sampleLabel_ = new QLabel(QString("Animation"), view_);
    curveEditor_ = new QLineEdit(view_);
    sampleEditor_ = new QLineEdit(view_);

    searchBtn_ = new QPushButton(view_);
    searchBtn_->setFlat(true);
    searchBtn_->setIcon(Icons::instance().typeZoom);

    refrenceBtn_ = new QPushButton(view_);
    refrenceBtn_->setFlat(true);
    refrenceBtn_->setIcon(Icons::instance().goTo);

    viewLayout->addWidget(curveLabel_, 0, 0);
    viewLayout->addWidget(searchBtn_, 0, 1, Qt::AlignLeft);
    viewLayout->addWidget(curveEditor_, 0, 2);
    viewLayout->addWidget(refrenceBtn_, 0, 3, Qt::AlignLeft);
    viewLayout->addWidget(sampleLabel_, 1, 0);
    viewLayout->addWidget(new QLabel("", view_), 1, 1);
    viewLayout->addWidget(sampleEditor_, 1, 2);
    viewLayout->setContentsMargins(35, 0, 0, 0);
    viewLayout->setColumnStretch(0, 0);
    viewLayout->setColumnStretch(1, 0);
    viewLayout->setColumnStretch(2, 1);
    view_->setLayout(viewLayout);
    view_->hide();

    searchEditor_ = new SearchEditor{this};
    resultTree_ = new ResultTree{this};

    auto* vLayout = new PropertyBrowserVBoxLayout{this};
    vLayout->setContentsMargins(5, 0, 0, 0);
    vLayout->addWidget(title_);
    vLayout->addWidget(view_);
    setLayout(vLayout);

    curveEditor_->setText(curve_);
    propertyEditor_->setText(property_);
    sampleEditor_->setText(sampleProperty_);
    connect(curveEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserCurveBindingView::slotCurveChanged);
    connect(propertyEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserCurveBindingView::slotPropertyChanged);
    connect(sampleEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserCurveBindingView::slotSamplePropertyChanged);
    connect(sampleEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserCurveBindingView::slotSamplePropertyChanged);
    QObject::connect(searchBtn_, &QPushButton::clicked, this, &PropertyBrowserCurveBindingView::slotSearchBtnClicked);
    QObject::connect(refrenceBtn_, &QPushButton::clicked, this, &PropertyBrowserCurveBindingView::slotRefrenceBtnClicked);

    connect(&signalProxy::GetInstance(), &signalProxy::sigCheckCurveBindingValid_From_CurveUI, this, &PropertyBrowserCurveBindingView::slotCheckAllCurveIsValid);

    palette_ = title_->palette();
    curveStyleSheet_ = curveEditor_->styleSheet();
    checkCurveIsValid(curve_);
}

bool PropertyBrowserCurveBindingView::isSelected() {
    return bIsSelected_;
}

bool PropertyBrowserCurveBindingView::deleteCurveBinding() {
    if (bIsSelected_) {
        return NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().deleteBindingDataItem(sampleProperty_.toStdString(), property_.toStdString(), curve_.toStdString());
    }
    return false;
}

void PropertyBrowserCurveBindingView::checkCurveIsValid(QString curve) {
    if (CurveManager::GetInstance().hasCurve(curve_.toStdString())) {
        curveEditor_->setStyleSheet(curveStyleSheet_);
    } else {
        curveEditor_->setStyleSheet("color:red");
    }
}

void PropertyBrowserCurveBindingView::slotCheckAllCurveIsValid() {
    checkCurveIsValid(curveEditor_->text());
}

void PropertyBrowserCurveBindingView::slotExpandedWidget() noexcept {
    if (view_ == nullptr) {
        return;
    }
    bExpanded_ = !bExpanded_;
    if(bExpanded_) {
        view_->show();
    }
    else {
        view_->hide();
    }
    Q_EMIT updateIcon(bExpanded_);
    update();
}

void PropertyBrowserCurveBindingView::slotCurveChanged() {
    QString curve = curveEditor_->text();
    std::map<std::string, std::string> bindingDataMap;
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty_.toStdString(), bindingDataMap);

    auto it = bindingDataMap.find(property_.toStdString());
    if (it == bindingDataMap.end()) {
        return;
    }
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().deleteBindingDataItem(sampleProperty_.toStdString(), property_.toStdString(), curve_.toStdString());
    curve_ = curve;
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().insertBindingDataItem(sampleProperty_.toStdString(), property_.toStdString(), curve_.toStdString());
    Q_EMIT signalProxy::GetInstance().sigRepaintTimeAxis_From_NodeUI();

    checkCurveIsValid(curve_);
}

void PropertyBrowserCurveBindingView::slotPropertyChanged() {
    QString property = propertyEditor_->text();
    std::map<std::string, std::string> bindingDataMap;
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty_.toStdString(), bindingDataMap);

    for (const auto &it : bindingDataMap) {
        if (it.first.compare(property.toStdString()) == 0) {
            propertyEditor_->setText(property_);
            return;
        }
    }

    auto it = bindingDataMap.find(property_.toStdString());
    if (it == bindingDataMap.end()) {
        return;
    }
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().deleteBindingDataItem(sampleProperty_.toStdString(), property_.toStdString(), curve_.toStdString());
    property_ = property;
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().insertBindingDataItem(sampleProperty_.toStdString(), property_.toStdString(), curve_.toStdString());
    Q_EMIT signalProxy::GetInstance().sigRepaintTimeAxis_From_NodeUI();
}

void PropertyBrowserCurveBindingView::slotSamplePropertyChanged() {
    QString sampleProperty = sampleEditor_->text();

    std::map<std::string, std::string> bindingDataMap;
    NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty_.toStdString(), bindingDataMap);

    auto it = bindingDataMap.find(property_.toStdString());
    if (it == bindingDataMap.end()) {
        return;
    }

    bindingDataMap.erase(it);
    sampleProperty_ = sampleProperty;
    std::string str = sampleProperty_.toStdString();
    auto bindingIt = NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().bindingMap().find(str);
    if (bindingIt != NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().bindingMap().end()) {
        NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().insertBindingDataItem(str, property_.toStdString(), curve_.toStdString());
    } else {
        std::map<std::string, std::string> newMap;
        newMap.emplace(property_.toStdString(), curve_.toStdString());
        NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().insertAnimation(str, newMap);
    }
    Q_EMIT signalProxy::GetInstance().sigRepaintTimeAxis_From_NodeUI();
}

void PropertyBrowserCurveBindingView::slotSearchBtnClicked() {
    searchEditor_->setGeoMetry(searchBtn_);
    if (searchEditor_->exec() == QDialog::Accepted) {
        QString search = searchEditor_->getSearchString();
        std::regex regex(search.toStdString());

        resultTree_->setGeoMetry(searchBtn_);
        resultTree_->clear();
        std::list<Curve*> curveList = CurveManager::GetInstance().getCurveList();
        for (auto it : curveList) {
            if (std::regex_search(it->getCurveName(), regex)) {
                resultTree_->addCurve(QString::fromStdString(it->getCurveName()));
            }
        }
        if (resultTree_->exec() == QDialog::Accepted) {
            QString curve = resultTree_->getSelectedCurve();
            curveEditor_->setText(curve);
			slotCurveChanged();
        }
    }
}

void PropertyBrowserCurveBindingView::slotRefrenceBtnClicked() {
    std::string samplePro = sampleEditor_->text().toStdString();
    std::string curve = curveEditor_->text().toStdString();
    std::string property = propertyEditor_->text().toStdString();
    Q_EMIT signalProxy::GetInstance().sigSwitchVisualCurve(samplePro, property, curve);
}

void PropertyBrowserCurveBindingView::mousePressEvent(QMouseEvent *event) {
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
