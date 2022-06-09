#ifndef PROPERTYBROWSERCURVEBINDINGVIEW_H
#define PROPERTYBROWSERCURVEBINDINGVIEW_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QPalette>
#include <QSpinBox>
#include <QSpacerItem>
#include "property_browser/editors/StringEditor.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/controls/ExpandButton.h"
#include "CurveData/CurveManager.h"
#include "NodeData/nodeManager.h"
#include "signal/SignalProxy.h"
#include "editors/SearchEditor.h"
#include "controls/ResultTree.h"

using namespace raco::signal;
using namespace raco::guiData;
namespace raco::property_browser {
class PropertyBrowserCurveBindingView : public QWidget {
    Q_OBJECT
public:
    explicit PropertyBrowserCurveBindingView(QString sampleProperty, QString property, QString curve, QWidget *parent = nullptr);
    bool isSelected();
    bool deleteCurveBinding();
    void checkCurveIsValid(QString curve);

Q_SIGNALS:
    void updateIcon(bool bExpanded);

public Q_SLOTS:
    void slotCheckAllCurveIsValid();
    void slotExpandedWidget() noexcept;
    void slotCurveChanged();
    void slotPropertyChanged();
    void slotSamplePropertyChanged();
    void slotSearchBtnClicked();
protected:
    void mousePressEvent(QMouseEvent *event);

private:
    QWidget* title_{nullptr};
    QWidget* view_{nullptr};
    QWidget* button_{nullptr};
    QLabel* propertyLabel_{nullptr};
    QLabel* curveLabel_{nullptr};
    QLabel* sampleLabel_{nullptr};
    QLineEdit* propertyEditor_{nullptr};
    QLineEdit* curveEditor_{nullptr};
    QLineEdit* sampleEditor_{nullptr};
    QPushButton *searchBtn_{nullptr};
    SearchEditor *searchEditor_{nullptr};
    ResultTree *resultTree_{nullptr};
    QPalette palette_;
    QString curveStyleSheet_;
    bool bExpanded_{false};
    bool bIsSelected_{false};

    QString curve_;
    QString property_;
    QString sampleProperty_;
};
}

#endif // PROPERTYBROWSERCURVEBINDINGVIEW_H
