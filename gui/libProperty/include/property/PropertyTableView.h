#ifndef PROPERTYTABLEVIEW_H
#define PROPERTYTABLEVIEW_H

#include <map>
#include <QTableView>
#include <QAbstractTableModel>
#include <QHeaderView>
#include <QMouseEvent>
#include <QMutex>
#include <QMutexLocker>
#include "PropertyTableDelegate.h"
#include "PropertyData/PropertyData.h"
#include "signal/SignalProxy.h"

using namespace raco::signal;
using namespace raco::guiData;
namespace raco::property {

enum TABLETYPE {
    PROPERTY_SYSTEM = 0,
    PROPERTY_CUSTOM = 1,
    PROPERTY_ANIMATION = 2,
    END_PROPERTY = 3
};

class PropertyTableView;
class PropertyModel : public QAbstractTableModel {
public:
    PropertyModel(TABLETYPE type, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    TABLETYPE type_;
    QMap<int, QString> roleMap;
    std::map<QString, EPropertyType> dataMap_;
    std::vector<QString> keyVec_;
    std::vector<EPropertyType> typeVec_;
    PropertyTableView *parent_;
    int key_ = 0;
};

class PropertyTableView : public QTableView {
    Q_OBJECT
public:
    PropertyTableView(TABLETYPE type, QWidget *parent);

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

Q_SIGNALS:
    void propertySelectedChanged(bool bIsChanged);

public Q_SLOTS:
    void addProperty();
    void delProperty();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    TABLETYPE type_;

    PropertyModel *sysModel;
};
}
#endif // PROPERTYTABLEVIEW_H
