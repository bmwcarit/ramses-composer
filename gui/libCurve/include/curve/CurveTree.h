#ifndef CURVETREE_H
#define CURVETREE_H

#include <QMenu>
#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QPair>
#include <QAbstractItemModel>
#include "PointPropertyView.h"
#include "CurveLogic.h"

namespace raco::curve {

#define STR_CURVE           QString("Curve")
#define STR_POINT           QString("Point")

class TreeItem {
public:
    TreeItem(QVariant data, TreeItem* parentItem = 0, Point* point = nullptr, Curve* curve = nullptr);
    ~TreeItem();
    void clearChildItem();
    int childCount();
    int columnCount();
    void appendChild(TreeItem* childItem);
    QVariant data(int column);
    void setData(QVariant data);
    TreeItem* parentItem();
    TreeItem* childItem(int row = 0);
    int row();

    Point* getPoint();
    Curve* getCurve();

private:
    TreeItem* parentItem_;
    QList<TreeItem*> childItemList_;
    QVariant valueData_;
    Point* point_{nullptr};
    Curve* curve_{nullptr};
};


class CurveTree : public QTreeView {
    Q_OBJECT
public:
    CurveTree(QWidget* parent, CurveLogic* logic = nullptr);
    void setItemExpandStatus();

public Q_SLOTS:
    void slotAddPointPropertyView(const QModelIndex &index);
    void slotDelPointPropertyView();
    void slotTreeMenu(const QPoint &pos);
    void slotInsertPoint();
    void slotCreateCurve();
    void slotDeletePoint();
    void slotDeleteCurve();
    void slotCollapsePointPropertyView(const QModelIndex &index);
Q_SIGNALS:
    void sigRefreshCurveView();
    void sigInsertPoint(QPoint pos);
    void sigDeletePoint(QPoint pos);
    void sigDeleteCurve(QPoint pos);
private:
    void removeItem2ExpandList(int row);
    void insertChildItem2ExpandList(int row, int childRow);
private:
    int index_{1};
    QList<PointPropertyView*> pointProViewList_;
    QList<QPair<int, int>> itemExpandList_;
    CurveLogic* curveLogic_{nullptr};
    QAction *insertPointAct_{nullptr};
    QAction *deletePointAct_{nullptr};
    QAction *deleteCurveAct_{nullptr};
    QAction *createCurveAct_{nullptr};
};


class CurveTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    CurveTreeModel(QObject *parent = nullptr, CurveLogic* logic = nullptr);
    ~CurveTreeModel();

    void initModel(std::list<Curve *> list);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;  

private:
    TreeItem* rootItem_{nullptr};
    CurveTree* parentView_{nullptr};
    CurveLogic* curveLogic_{nullptr};
};

}

#endif // CURVETREE_H
