#ifndef PROPERTYBROWSERCURVEBINDINGWIDGET_H
#define PROPERTYBROWSERCURVEBINDINGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include "property_browser/PropertyBrowserCurveBindingView.h"
#include "AnimationData/animationData.h"

namespace raco::property_browser {
class PropertyBrowserCurveBindingWidget : public QWidget {
    Q_OBJECT
public:
    explicit PropertyBrowserCurveBindingWidget(QWidget *parent = nullptr);

    void initCurveBindingWidget();
    void insertCurveBinding(QString sampleProperty, QString property, QString curve);
    void clearCurveBinding();

public Q_SLOTS:
    void insertData();
    void removeData();

private:
    bool isInValidBindingData(QString sampleProperty, QString property, QString curve);

private:
    QList<QWidget*> listWidget_;
    QGridLayout *layout_{nullptr};
    QScrollArea* scroll_{nullptr};
    int index_{1};
};
}

#endif // PROPERTYBROWSERCURVEBINDINGWIDGET_H
