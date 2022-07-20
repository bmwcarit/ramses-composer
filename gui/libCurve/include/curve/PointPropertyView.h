#ifndef POINTPROPERTYVIEW_H
#define POINTPROPERTYVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include "qgridlayout.h"
#include "CurveData/CurveManager.h"
#include "NodeData/nodeManager.h"

using namespace raco::guiData;

class PointPropertyView : public QWidget
{
    Q_OBJECT
public:
    explicit PointPropertyView(Curve* curve, Point* point, QWidget *parent = nullptr);
    Point* getPoint();

Q_SIGNALS:
    void sigRefreshCurveView();

public Q_SLOTS:
    void slotKeyFrameChanged();
    void slotValueChanged();
    void slotLeftTangentChanged();
    void slotRightTangentChanged();

private:
    void initView();

private:
    QComboBox* typeComboBox_{nullptr};
    QSpinBox* keyFrameSpinBox_{nullptr};
    QLineEdit* valueEditor_{nullptr};
    QLineEdit* leftTangEditor_{nullptr};
    QLineEdit* rightTangEditor_{nullptr};

    Point* point_{nullptr};
    Curve* curve_{nullptr};
    QGridLayout* gridLayout_;
};

#endif // POINTPROPERTYVIEW_H
