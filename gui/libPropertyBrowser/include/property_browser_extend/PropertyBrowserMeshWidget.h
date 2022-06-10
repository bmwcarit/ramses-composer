#ifndef PROPERTYBROWSERMESHWIDGET_H
#define PROPERTYBROWSERMESHWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QDebug>
#include "property_browser/editors/StringEditor.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "NodeData/nodeManager.h"
#include "node_logic/NodeLogic.h"


namespace raco::property_browser {
class PropertyBrowserMeshWidget : public QWidget {
    Q_OBJECT
public:
    explicit PropertyBrowserMeshWidget(QWidget *parent = nullptr);
    void initPropertyBrowserMeshWidget();
    void clearMesh();

public Q_SLOTS:
    void slotUpdateMeshName();
    void slotUpdateMaterial();

private:
    QLabel* nameLabel_{nullptr};
    QLabel* materialLabel_{nullptr};
    QLineEdit* nameEditor_{nullptr};
    QLineEdit* materialEditor_{nullptr};
};
}

#endif // PROPERTYBROWSERMESHWIDGET_H
