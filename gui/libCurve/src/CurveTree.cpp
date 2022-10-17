#include "curve/CurveTree.h"

namespace raco::curve {

TreeItem::TreeItem(QVariant data, TreeItem *parentItem, Point* point, Curve *curve) {
    parentItem_ = parentItem;
    valueData_ = data;
    point_ = point;
    curve_ = curve;
}

TreeItem::~TreeItem() {
    clearChildItem();
}

void raco::curve::TreeItem::clearChildItem() {
    while (!childItemList_.empty()) {
        auto it = childItemList_.begin();
        delete* it;
        childItemList_.erase(it);
    }
}

int raco::curve::TreeItem::childCount() {
    return childItemList_.count();
}

int raco::curve::TreeItem::columnCount() {
    return 1;
}

void raco::curve::TreeItem::appendChild(TreeItem *childItem) {
    childItemList_.append(childItem);
}

QVariant raco::curve::TreeItem::data(int column) {
    return valueData_;
}

void raco::curve::TreeItem::setData(QVariant data) {
    valueData_ = data;
}

raco::curve::TreeItem *raco::curve::TreeItem::parentItem() {
    return parentItem_;
}

raco::curve::TreeItem *raco::curve::TreeItem::childItem(int row) {
    return childItemList_.value(row);
}

int raco::curve::TreeItem::row() {
    if(parentItem_)
        return parentItem_->childItemList_.indexOf(const_cast<TreeItem *>(this));
    return 0;
}

Point *TreeItem::getPoint() {
    return point_;
}

Curve *TreeItem::getCurve() {
    return curve_;
}

CurveTree::CurveTree(QWidget *parent, CurveLogic *logic, NodeLogic *nodeLogic) : QTreeView(parent), curveLogic_(logic), nodeLogic_(nodeLogic) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    this->setEditTriggers(QTreeView::DoubleClicked);

    //展开时添加frameEditorView
    connect(this, &CurveTree::expanded, this, &CurveTree::slotAddPointPropertyView);
    //
    connect(this, &CurveTree::collapsed, this, &CurveTree::slotCollapsePointPropertyView);
    // 添加右键菜单栏
    connect(this, &CurveTree::customContextMenuRequested, this, &CurveTree::slotTreeMenu);
    // 插入Point
    connect(this, &CurveTree::sigInsertPoint, this, &CurveTree::slotInsertPoint);
    // 删除Point
    connect(this, &CurveTree::sigDeletePoint, this, &CurveTree::slotDeletePoint);
    // 删除Curve
    connect(this, &CurveTree::sigDeleteCurve, this, &CurveTree::slotDeleteCurve);
    //
    insertPointAct_ = new QAction(QStringLiteral("insert Point"), this);
    deletePointAct_ = new QAction(QStringLiteral("delete Point"), this);
    deleteCurveAct_ = new QAction(QStringLiteral("delete Curve"), this);
    createCurveAct_ = new QAction(QStringLiteral("create Curve"), this);
    copyCurveAct_   = new QAction(QStringLiteral("copy Curve"), this);
    pasteCurveAct_  = new QAction(QStringLiteral("paste Curve"), this);
    connect(insertPointAct_, &QAction::triggered, this, &CurveTree::slotInsertPoint);
    connect(deletePointAct_, &QAction::triggered, this, &CurveTree::slotDeletePoint);
    connect(deleteCurveAct_, &QAction::triggered, this, &CurveTree::slotDeleteCurve);
    connect(createCurveAct_, &QAction::triggered, this, &CurveTree::slotCreateCurve);
    connect(copyCurveAct_,   &QAction::triggered, this, &CurveTree::slotCopyCurve);
    connect(pasteCurveAct_,  &QAction::triggered, this, &CurveTree::slotPasteCurve);
}

void CurveTree::setItemExpandStatus() {
    for (QPair<int, int> pair : qAsConst(itemExpandList_)) {
        QModelIndex index = model()->index(pair.first, 0);
        QModelIndex childIndex = model()->index(pair.second, 0, index);
        this->setExpanded(index, true);
        this->setExpanded(childIndex, true);
    }
}

void CurveTree::slotAddPointPropertyView(const QModelIndex &index) {
    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    if (item && item->childCount() != 0) {
        int childNum = item->childCount();
        for(int n = 0; n < childNum; n++) {
            TreeItem *childItem = item->childItem(n);
            if (childItem->getPoint() && childItem->getCurve()) {
                QPair<int, int> pair;
                pair.first = item->parentItem()->row();
                pair.second = item->row();
                if (!itemExpandList_.contains(pair)) {
                    itemExpandList_.push_back(pair);
                }

                PointPropertyView *view = new PointPropertyView(childItem->getCurve(), childItem->getPoint(), this);
                connect(view, &PointPropertyView::sigRefreshCurveView, this, &CurveTree::sigRefreshCurveView);
                connect(view, &PointPropertyView::sigUpdateCurve, this, &CurveTree::sigUpdateCurve);
                setIndexWidget(model()->index(n, 0, index), view);
                pointProViewList_.push_back(view);
            }

        }
    }
}

void CurveTree::slotDelPointPropertyView() {
	pointProViewList_.clear();
}

void CurveTree::slotTreeMenu(const QPoint &pos) {
    QMenu menu;
    QModelIndex curIndex = this->indexAt(pos);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (item) {
		if (index.isValid() && item->childItem() && item->childItem()->getPoint()) {
            menu.addAction(insertPointAct_);
            menu.addAction(deletePointAct_);
        }
		if (index.isValid() && item->childItem() && item->childItem()->getPoint() == nullptr) {
            menu.addAction(deleteCurveAct_);
            menu.addAction(copyCurveAct_);
            if (!copyCurve_.empty()) {
                menu.addAction(pasteCurveAct_);
            }
        }
    }
    menu.addAction(createCurveAct_);
    menu.exec(QCursor::pos());
}

void CurveTree::slotInsertPoint() {
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.size() <= 0) {
        return;
    }
    QModelIndex curIndex = selected.at(0);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (item) {
        if(item->childItem()) {
            TreeItem* pointProItem = item->childItem();
            Point* point = pointProItem->getPoint();
            Curve* curve = pointProItem->getCurve();
            if (point && curve) {
                Point* newPoint = new Point(point->getKeyFrame() + 1);
                newPoint->setKeyFrame(point->getKeyFrame() + 1);
				newPoint->setDataValue(point->getDataValue());
				newPoint->setInterPolationType(point->getInterPolationType());
				newPoint->setLeftTagent(point->getLeftTagent());
				newPoint->setRightTagent(point->getRightTagent());

                curve->insertPoint(newPoint);
                if (item->parentItem()) {
                    insertChildItem2ExpandList(item->parentItem()->row(), item->row());
                }

                Q_EMIT sigRefreshCurveView();
            }
        }
    }
}

void CurveTree::slotCreateCurve() {
    QString curveName = STR_CURVE + QString::number(index_);
    while (!curveLogic_->insertCurve(QString(), curveName)) {
        index_++;
        curveName = STR_CURVE + QString::number(index_);
    }
    index_++;
    Q_EMIT sigRefreshCurveView();
}

void CurveTree::slotDeletePoint() {
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.size() <= 0) {
        return;
    }
    QModelIndex curIndex = selected.at(0);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (item) {
        if (item->childItem()) {
            TreeItem* pointProItem = item->childItem();
            Point* point = pointProItem->getPoint();
            Curve* curve = pointProItem->getCurve();
            if (curve && point) {
                curve->takePoint(point->getKeyFrame());
                if (curve->getPointList().size() == 0) {
                    curveLogic_->delCurve(QString::fromStdString(curve->getCurveName()));
                    if (item->parentItem()) {
                       removeItem2ExpandList(item->parentItem()->row());
                    }
                }

                Q_EMIT sigRefreshCurveView();
            }
        }
    }
}

void CurveTree::slotDeleteCurve() {
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.size() <= 0) {
        return;
    }
    QModelIndex curIndex = selected.at(0);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (item) {
        if (item->getCurve()) {
            Curve* curve = item->getCurve();
            curveLogic_->delCurve(QString::fromStdString(curve->getCurveName()));
			nodeLogic_->delNodeBindingByCurveName(curve->getCurveName());
            removeItem2ExpandList(item->row());
            Q_EMIT sigRefreshCurveView();
        }
    }
}

void CurveTree::slotCopyCurve() {
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.size() <= 0) {
        return;
    }
    QModelIndex curIndex = selected.at(0);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (item) {
        if (item->getCurve()) {
            copyCurve_ = item->getCurve()->getCurveName();
        }
    }
}

void CurveTree::slotPasteCurve() {
    curveLogic_->copyCurve(copyCurve_);
    Q_EMIT sigRefreshCurveView();
}

void CurveTree::slotCollapsePointPropertyView(const QModelIndex &index) {
    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    if (item && item->childCount() != 0) {
        int childNum = item->childCount();
        for(int n = 0; n < childNum; n++) {
            TreeItem *childItem = item->childItem(n);
            if (childItem->getPoint() && childItem->getCurve()) {
                QPair<int, int> pair;
                pair.first = item->parentItem()->row();
                pair.second = item->row();
                itemExpandList_.removeOne(pair);
            }
        }
    }
}

void CurveTree::removeItem2ExpandList(int row) {
    for (QPair<int, int>& pair : itemExpandList_) {
        if (pair.first == row) {
            itemExpandList_.removeOne(pair);
        }
        if (pair.first > row) {
            pair.first--;
        }
    }
}

void CurveTree::insertChildItem2ExpandList(int row, int childRow) {
    for (QPair<int, int>& pair : itemExpandList_) {
        if (pair.first == row) {
            if (pair.second > childRow) {
                pair.second++;
            }
        }
    }
}

CurveTreeModel::CurveTreeModel(QObject *parent, CurveLogic *logic, NodeLogic *nodeLogic) : curveLogic_(logic), nodeLogic_(nodeLogic) {
    parentView_ = dynamic_cast<CurveTree*>(parent);
    rootItem_ = new TreeItem(QList<QVariant>());
}

CurveTreeModel::~CurveTreeModel() {
    if (rootItem_) {
        delete rootItem_;
        rootItem_ = nullptr;
    }
}

void CurveTreeModel::initModel(std::list<Curve*> list) {
    for (const auto& it : list) {
        QString strHeaderTitle = QString::fromStdString(it->getCurveName());
        TreeItem* curveItem = new TreeItem(strHeaderTitle, rootItem_, nullptr, it);
        rootItem_->appendChild(curveItem);

        std::list<Point*> pointList = it->getPointList();
        int index = 1;
        for (const auto& pointIt : pointList) {
            strHeaderTitle = QString(STR_POINT) + QString::number(index);
            TreeItem* pointItem = new TreeItem(strHeaderTitle, curveItem);
            TreeItem* pointProItem = new TreeItem(QVariant(), pointItem,  pointIt, it);
            pointItem->appendChild(pointProItem);
            curveItem->appendChild(pointItem);
            index++;
        }
    }
}

QVariant CurveTreeModel::data(const QModelIndex &index, int role) const {
    if(!index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole: {
        TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
        return item->data(0);
    }
    case Qt::EditRole: {
        TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
        return item->data(0);
    }
    }

    return QVariant();
}

bool CurveTreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (index.isValid() && role == Qt::EditRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        
        Curve* curve = item->getCurve();
        if (curve) {
            curveLogic_->modifyCurveName(QString::fromStdString(curve->getCurveName()), value.toString());
			item->setData(value);
			Q_EMIT dataChanged(index, index);
            Q_EMIT parentView_->sigRefreshCurveView();
        }
        return true;
    }
    return false;
}

QModelIndex CurveTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row,column,parent))
        return QModelIndex();

    TreeItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem *>(parent.internalPointer());

    TreeItem *childItem = parentItem->childItem(row);
    if (childItem)
        return  createIndex(row,column,childItem);
    else
        return QModelIndex();
}

int CurveTreeModel::rowCount(const QModelIndex &parent) const {
    TreeItem *parentItem;
    if(!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

int CurveTreeModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QModelIndex CurveTreeModel::parent(const QModelIndex &child) const {
    if(!child.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem *>(child.internalPointer());
    if (!childItem)
        return QModelIndex();

    TreeItem *parentItem = childItem->parentItem();

    if(!parentItem || parentItem == rootItem_)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

Qt::ItemFlags CurveTreeModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
            return Qt::ItemFlags();

    //节点是否允许编辑
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags |= Qt::ItemIsEditable;

    return flags;
}

}
