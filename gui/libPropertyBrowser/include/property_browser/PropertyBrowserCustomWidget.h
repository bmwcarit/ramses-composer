#ifndef PROPERTYBROWSERCUSTOMWIDGET_H
#define PROPERTYBROWSERCUSTOMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include "property_browser/PropertyBrowserCustomView.h"
#include "AnimationData/animationData.h"

namespace raco::property_browser {
class PropertyBrowserCustomWidget : public QWidget {
    Q_OBJECT
public:
    explicit PropertyBrowserCustomWidget(QWidget *parent = nullptr);

    void initPropertyBrowserCustomWidget();
	void clearPropertyBrowserCustomWidget();

public Q_SLOTS:
    void insertData();
    void removeData();

private:
	QList<PropertyBrowserCustomView*> listWidget_;
    QGridLayout *layout_{nullptr};
    QScrollArea* scroll_{nullptr};
    int index_{1};
};
}

#endif // PROPERTYBROWSERCUSTOMWIDGET_H
