#ifndef PROPERTYBROWSERCUSTOMVIEW_H
#define PROPERTYBROWSERCUSTOMVIEW_H

#include <QWidget>
#include <QLabel>
#include <QDebug>
#include <QMouseEvent>
#include <any>
#include <QStackedWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QMap>
#include "property_browser/editors/StringEditor.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser_extend/controls/ExpandControlNoItemButton.h"
#include "PropertyData/PropertyData.h"
#include "signal/SignalProxy.h"
#include "NodeData/nodeManager.h"
#include "PropertyData/PropertyType.h"
#include "property_browser/editors/SearchEditor.h"

using namespace raco::signal;
using namespace raco::guiData;
namespace raco::property_browser {
class PropertyBrowserCustomView : public QWidget {
    Q_OBJECT
public:
    explicit PropertyBrowserCustomView(QWidget *parent = nullptr);

    void mousePressEvent(QMouseEvent* event);
    void initCustomView();
    void readCustomProperty(QString name, std::any data);
    void reLoadCustomProperty(EPropertyType type);
    void serachCustom(QString search);
Q_SIGNALS:
    void updateIcon(bool bExpanded);

public Q_SLOTS:
    void expandedWidget() noexcept;
    void slotNameChanged(QString name);
    void slotSearchBtnClicked();
    void slotStringChanged();
    void slotCheckBoxChanged(bool value);
    void slotSpinBoxChanged();
    void slotDoubleSpinBoxChanged();
    void slotMartixDoubleSpinBoxChanged();
    void slotUpdateCustomView();

    QString getName() {
        return nameCombox_->currentText();
    }

    bool isChecked() {
		return bIsSelected_;
    }

private:
    QWidget* title_{nullptr};
    QStackedWidget* view_{nullptr};
    QWidget* button_{nullptr};
    QLabel* label_{nullptr};
    QComboBox* nameCombox_{nullptr};
    QPushButton *searchBtn_{nullptr};
    SearchEditor *searchEditor_{nullptr};

    QLabel* labelData_{nullptr};

    int martixSize{12};
    bool bExpanded{false};
    bool bIsSelected_{0};
    QPalette palette_;

    QString lastName_;
    QMap<raco::guiData::EPropertyType, QWidget*> propertyTypeWidgetMap_;
};
}

#endif // PROPERTYBROWSERCUSTOMVIEW_H
