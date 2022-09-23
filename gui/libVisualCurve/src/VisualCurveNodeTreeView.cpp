#include "visual_curve/VisualCurveNodeTreeView.h"
#include "visual_curve/VisualCurvePosManager.h"

namespace raco::visualCurve {

TreeModel::TreeModel(QWidget *parent) {

}

void TreeModel::setFolderData(FolderDataManager *mgr) {
    folderDataMgr_ = mgr;
}

bool TreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    auto curveFromItem = [=](QStandardItem *item)->QString {
        QString curve = item->text();
        while(item->parent()) {
            item = item->parent();
            QString tempStr = item->text() + "|";
            curve.insert(0, tempStr);
        }
        return curve;
    };

    QByteArray array = data->data("test");
    QDataStream stream(&array, QIODevice::ReadOnly);
    qint64 p;
    stream >> p;
    QModelIndex tempParent = parent;
    QStandardItem *parentItem = itemFromIndex(parent);
    QModelIndex* index = (QModelIndex*)p;

    if (index) {
        QStandardItem *item = itemFromIndex(*index);
        std::string curvePath = curveFromItem(item).toStdString();
        Folder *folder{nullptr};
        SCurveProperty *curveProp{nullptr};
        if (folderDataMgr_->folderFromCurveName(curvePath, folder, curveProp)) {
            if (folder) {
                if (curveProp) {
                    folder->takeCurve(curveProp->curve_);
                    if (parentItem) {
                        std::string destCurvePath = curveFromItem(parentItem).toStdString();
                        Folder *destFolder{nullptr};
                        SCurveProperty *destCurveProp{nullptr};
                        if (folderDataMgr_->folderFromCurveName(destCurvePath, destFolder, destCurveProp)) {
                            if (destFolder && !destCurveProp) {
                                destFolder->insertCurve(curveProp);
                                move(index->parent(), index->row(), tempParent, parentItem->rowCount());
                                if (CurveManager::GetInstance().getCurve(curvePath)) {
                                    destCurvePath = destCurvePath + "|" + curveProp->curve_;
                                    Curve *curve = CurveManager::GetInstance().getCurve(curvePath);
                                    curve->setCurveName(destCurvePath);
                                }
                            }
                        }
                    } else {
                        folderDataMgr_->getDefaultFolder()->insertCurve(curveProp);
                        move(index->parent(), index->row(), tempParent, rowCount());
                        if (CurveManager::GetInstance().getCurve(curvePath)) {
                            Curve *curve = CurveManager::GetInstance().getCurve(curvePath);
                            curve->setCurveName(curveProp->curve_);
                        }
                    }
                } else {
                    folder->parent()->takeFolder(folder->getFolderName());
                    if (parentItem) {
                        std::string destCurvePath = curveFromItem(parentItem).toStdString();
                        Folder *destFolder{nullptr};
                        SCurveProperty *destCurveProp{nullptr};
                        if (folderDataMgr_->folderFromCurveName(destCurvePath, destFolder, destCurveProp)) {
                            if (destFolder && !destCurveProp) {
                                destFolder->insertFolder(folder);
                                move(index->parent(), index->row(), tempParent, parentItem->rowCount());
                            }
                        }
                    } else {
                        folderDataMgr_->getDefaultFolder()->insertFolder(folder);
                        move(index->parent(), index->row(), tempParent, rowCount());
                    }
                }
            } else {
                if (parentItem) {
                    folderDataMgr_->getDefaultFolder()->insertCurve(curvePath);
                    move(index->parent(), index->row(), tempParent, parentItem->rowCount());
                    if (CurveManager::GetInstance().getCurve(curvePath)) {
                        Curve *curve = CurveManager::GetInstance().getCurve(curvePath);
                        std::string destCurve = parentItem->text().toStdString() + "|" + curvePath;
                        curve->setCurveName(destCurve);
                    }
                } else {
                    return false;
                }
            }
        }
        delete index;
    }
    return true;
}

Qt::DropActions TreeModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QMimeData *TreeModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData* mimeData = QAbstractItemModel::mimeData(indexes);

    QVector<qint64> vector;
    QByteArray array;
    for (int i = 0; i < indexes.count(); i++) {
        QModelIndex index = indexes[i];
        QModelIndex* p = new QModelIndex(index);
        QDataStream stream(&array, QIODevice::WriteOnly);
        stream << (qint64)p;
        mimeData->setData(QString("test"), array);
    }
    return mimeData;
}

bool TreeModel::move(QModelIndex source, int sourceRow, QModelIndex &dest, int destRow) {
    if (sourceRow < 0 || destRow < 0) {
        return false;
    }
    if(!beginMoveRows(source, sourceRow, sourceRow, dest, destRow)){
        return false;
    }

    if (dest.isValid()) {
        int to = dest.row();
        QStandardItem *sourceItem = itemFromIndex(source);
        QList<QStandardItem *> itemList;
        if (sourceItem) {
            itemList = sourceItem->takeRow(sourceRow);
        } else {
            itemList = takeRow(sourceRow);
            if (sourceRow < to) {
                to -= 1;
                dest = item(to)->index();
            }
        }

        QStandardItem *destItem = item(to);
        blockSignals(true);
        beginInsertRows(dest, destRow, destRow);
        if(destItem) {
            destItem->insertRow(destRow, itemList);
        }
//        endInsertRows();
        blockSignals(false);
    } else {
        QStandardItem *sourceItem = itemFromIndex(source);
        QList<QStandardItem *> itemList;
        if (sourceItem) {
            itemList = sourceItem->takeRow(sourceRow);
        } else {
            itemList = takeRow(sourceRow);
        }

        blockSignals(true);
        beginInsertRows(dest, destRow, destRow);
        insertRow(destRow, itemList);
//        endInsertRows();
        blockSignals(false);
    }
    endMoveRows();
    return true;
}

VisualCurveNodeTreeView::VisualCurveNodeTreeView(QWidget *parent)
    : QWidget{parent} {
    visualCurveTreeView_ = new QTreeView(this);
    model_ = new TreeModel(visualCurveTreeView_);
    model_->setColumnCount(2);
    visualCurveTreeView_->setModel(model_);
    visualCurveTreeView_->setHeaderHidden(true);
    visualCurveTreeView_->header()->resizeSection(0, 240);
    visualCurveTreeView_->header()->resizeSection(1, 30);

    visualCurveTreeView_->setDragEnabled(true);
    visualCurveTreeView_->setAcceptDrops(true);
    visualCurveTreeView_->setDragDropMode(QAbstractItemView::InternalMove);
    visualCurveTreeView_->setDropIndicatorShown(true);
    visualCurveTreeView_->setDragDropOverwriteMode(true);
    visualCurveTreeView_->setDefaultDropAction(Qt::DropAction::MoveAction);
    visualCurveTreeView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    folderDataMgr_ = new FolderDataManager;
    model_->setFolderData(folderDataMgr_);

    visibleButton_ = new ButtonDelegate(visualCurveTreeView_);
    visualCurveTreeView_->setItemDelegateForColumn(1, visibleButton_);
    visibleButton_->setFolderManager(folderDataMgr_);
    visibleButton_->setModel(model_);

    QVBoxLayout *vBoxLayout = new QVBoxLayout(this);
    vBoxLayout->addWidget(visualCurveTreeView_);
    vBoxLayout->setMargin(0);
    this->setLayout(vBoxLayout);
    QObject::connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigSwitchVisualCurve, this, &VisualCurveNodeTreeView::slotRefrenceBindingCurve);
    QObject::connect(&raco::signal::signalProxy::GetInstance(), &raco::signal::signalProxy::sigInsertCurve_To_VisualCurve, this, &VisualCurveNodeTreeView::slotInsertCurve);

    menu_ = new QMenu{this};
    createFolder_ = new QAction("Create Floder");
    deleteFolder_ = new QAction("Delete Floder");
    createCurve_ = new QAction("Create Curve");
    deleteCurve_ = new QAction("Delete Curve");

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualCurveNodeTreeView::customContextMenuRequested, this, &VisualCurveNodeTreeView::slotShowContextMenu);
    connect(createFolder_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotCreateFolder);
    connect(deleteFolder_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotDeleteFolder);
    connect(createCurve_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotCreateCurve);
    connect(deleteCurve_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotDeleteCurve);
    connect(model_, &QStandardItemModel::itemChanged, this, &VisualCurveNodeTreeView::slotItemChanged);
    connect(visualCurveTreeView_, &QTreeView::pressed, this, &VisualCurveNodeTreeView::slotCurrentRowChanged);
    connect(visibleButton_, &ButtonDelegate::clicked, this, &VisualCurveNodeTreeView::slotButtonDelegateClicked);
}

VisualCurveNodeTreeView::~VisualCurveNodeTreeView() {
    if (folderDataMgr_) {
        delete folderDataMgr_;
        folderDataMgr_ = nullptr;
    }
}

void VisualCurveNodeTreeView::initCurves() {
    folderDataMgr_->clear();
    model_->removeRows(0, model_->rowCount());

    for (auto curve : CurveManager::GetInstance().getCurveList()) {
        QStandardItem *curveItem = new QStandardItem(QString::fromStdString(curve->getCurveName()));
        folderDataMgr_->getDefaultFolder()->insertCurve(curve->getCurveName());
        model_->appendRow(curveItem);
    }
}

void VisualCurveNodeTreeView::switchCurSelCurve(std::string curve) {
    QStringList list = QString::fromStdString(curve).split("|");
    QString root = list.takeFirst();
    QStandardItem *item{nullptr};
    for (int i{0}; i < model_->rowCount(); i++) {
        if (model_->item(i)->text() == root) {
            item = model_->item(i);
        }
    }
    if (!item) {
        return;
    }

    for (const QString &node : list) {
        if (!itemFromPath(item, node)) {
            return;
        }
    }
    visualCurveTreeView_->setCurrentIndex(item->index());
    slotCurrentRowChanged(item->index());
}

void VisualCurveNodeTreeView::cancleSelCurve() {
    visualCurveTreeView_->setCurrentIndex(QModelIndex());
}

void VisualCurveNodeTreeView::slotInsertCurve(QString property, QString curve, QVariant value) {
    QStandardItem *curveItem = new QStandardItem(curve);
    model_->appendRow(curveItem);
    folderDataMgr_->getDefaultFolder()->insertCurve(curve.toStdString());
}

void VisualCurveNodeTreeView::slotRefrenceBindingCurve(std::string smapleProp, std::string prop, std::string curve) {
    switchCurSelCurve(curve);
}

void VisualCurveNodeTreeView::slotShowContextMenu(const QPoint &p) {
    QModelIndex curIndex = visualCurveTreeView_->indexAt(p);
    QModelIndex index = curIndex.sibling(curIndex.row(), 0);
    QStandardItem* item = model_->itemFromIndex(index);

    menu_->clear();
    if (item) {
        std::string curve = curveFromItem(item).toStdString();
        if (folderDataMgr_->hasCurve(curve)) {
            menu_->addAction(deleteCurve_);
        } else {
            menu_->addAction(deleteFolder_);
        }
    }
    menu_->addAction(createFolder_);
    menu_->addAction(createCurve_);
    menu_->exec(mapToGlobal(p));
}

void VisualCurveNodeTreeView::slotCreateFolder() {
    QModelIndex index = visualCurveTreeView_->currentIndex();
    if (index.isValid()) {
        QStandardItem *item = model_->itemFromIndex(index);
        std::string curve = curveFromItem(item).toStdString();

        Folder *folder{nullptr};
        SCurveProperty *curveProp{nullptr};
        if (folderDataMgr_->folderFromCurveName(curve, folder, curveProp)) {
            if (folder) {
                std::string defaultFolder = folder->createDefaultFolder();
                QStandardItem *folderItem = new QStandardItem(QString::fromStdString(defaultFolder));
                folder->insertFolder(defaultFolder);
                if (curveProp) {
                    item->parent()->appendRow(folderItem);
                } else {
                    item->appendRow(folderItem);
                }
                return;
            }
        }
    }
    std::string defaultFolder = folderDataMgr_->getDefaultFolder()->createDefaultFolder();
    QStandardItem *folderItem = new QStandardItem(QString::fromStdString(defaultFolder));
    folderDataMgr_->getDefaultFolder()->insertFolder(defaultFolder);
    model_->appendRow(folderItem);
}

void VisualCurveNodeTreeView::slotDeleteFolder() {
    QModelIndex selected = visualCurveTreeView_->currentIndex();
    QStandardItem *item = model_->itemFromIndex(selected);
    if (item) {
        if (item->hasChildren()) {
            QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Ramses Composer",
                tr("Delete Folder?\n"),
                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                QMessageBox::Yes);
            if (resBtn == QMessageBox::No) {
                return;
            }

            Folder *folder{nullptr};
            SCurveProperty *curveProp{nullptr};
            std::string curve = curveFromItem(item).toStdString();
            if (folderDataMgr_->folderFromCurveName(curve, folder, curveProp)) {
                if (folder) {
                    for (auto it : folder->getCurveList()) {
                        CurveManager::GetInstance().delCurve(it->curve_);
                        VisualCurvePosManager::GetInstance().deleteKeyPointList(it->curve_);
                        VisualCurvePosManager::GetInstance().deleteWorkerPointList(it->curve_);
                    }
                    folder->parent()->deleteFolder(item->text().toStdString());
                }
            }
        }
        model_->removeRow(selected.row(), selected.parent());
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveNodeTreeView::slotCreateCurve() {
    std::string curve;
    QModelIndex selected = visualCurveTreeView_->currentIndex();
    if (selected.isValid()) {
        QStandardItem *item = model_->itemFromIndex(selected);
        std::string curCurve = curveFromItem(item).toStdString();
        Folder *folder{nullptr};
        SCurveProperty *curveProp{nullptr};
        if (folderDataMgr_->folderFromCurveName(curCurve, folder, curveProp)) {
            if (folder) {
                curve = folder->createDefaultCurve();
                folder->insertCurve(curve);
                QStandardItem *curveItem = new QStandardItem(QString::fromStdString(curve));
                if (curveProp) {
                    item->parent()->appendRow(curveItem);
                } else {
                    item->appendRow(curveItem);
                }
                return;
            }
        }
    }
    curve = folderDataMgr_->getDefaultFolder()->createDefaultCurve();
    QStandardItem *curveItem = new QStandardItem(QString::fromStdString(curve));
    model_->appendRow(curveItem);

    Curve *tempCurve = new Curve;
    tempCurve->setCurveName(curve);
    CurveManager::GetInstance().addCurve(tempCurve);
}

void VisualCurveNodeTreeView::slotDeleteCurve() {
    QModelIndex selected = visualCurveTreeView_->currentIndex();
    QStandardItem *item = model_->itemFromIndex(selected);
    if (item) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Ramses Composer",
            tr("Delete Curve?\n"),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (resBtn == QMessageBox::No) {
            return;
        }

        std::string curve = item->text().toStdString();
        std::string curvePath = curveFromItem(item).toStdString();
        Folder *folder{nullptr};
        SCurveProperty *curveProp{nullptr};
        if (folderDataMgr_->folderFromCurveName(curvePath, folder, curveProp)) {
            if (folder) {
                folder->deleteCurve(curve);
            } else {
                folderDataMgr_->getDefaultFolder()->deleteCurve(curvePath);
            }
            CurveManager::GetInstance().delCurve(curvePath);
            VisualCurvePosManager::GetInstance().deleteKeyPointList(curvePath);
            VisualCurvePosManager::GetInstance().deleteWorkerPointList(curvePath);
        }
        model_->removeRow(selected.row(), selected.parent());
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveNodeTreeView::slotItemChanged(QStandardItem *item) {
    auto swapPoints = [=](std::string oldCurve, std::string newCurve) {
        QList<SKeyPoint> keyPoints;
        QList<QPair<QPointF, QPointF>> workerPoints;

        VisualCurvePosManager::GetInstance().getKeyPointList(oldCurve, keyPoints);
        VisualCurvePosManager::GetInstance().getWorkerPointList(oldCurve, workerPoints);
        VisualCurvePosManager::GetInstance().deleteKeyPointList(oldCurve);
        VisualCurvePosManager::GetInstance().deleteWorkerPointList(oldCurve);
        VisualCurvePosManager::GetInstance().addKeyPointList(newCurve, keyPoints);
        VisualCurvePosManager::GetInstance().addWorkerPointList(newCurve, workerPoints);
    };

    std::string curve = item->text().toStdString();
    std::string curvePath = selNode_;
    Folder *folder{nullptr};
    SCurveProperty *curveProp{nullptr};
    if (folderDataMgr_->folderFromCurveName(curvePath, folder, curveProp)) {
        if (curveProp) {
            curveProp->curve_ = curve;
            std::string newCurve;
            folderDataMgr_->curveNameFromFolder(curve, folder, newCurve);
            swapPoints(curvePath, newCurve);
            Q_EMIT signal::signalProxy::GetInstance().sigCheckCurveBindingValid_From_CurveUI();
        }
    }
}

void VisualCurveNodeTreeView::slotCurrentRowChanged(const QModelIndex &index) {
    QStandardItem *item = model_->itemFromIndex(index);
    std::string curvePath = curveFromItem(item).toStdString();
    selNode_ = curvePath;

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    if (folderDataMgr_->hasCurve(curvePath)) {
        if (curCurve != curvePath) {
            VisualCurvePosManager::GetInstance().setCurrentPointInfo(curvePath, 0);
            VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_ACTION::MOUSE_PRESS_KEY);
            Q_EMIT sigSwitchVisualCurveInfoWidget();
            Q_EMIT sigRefreshVisualCurve();
        }
        return;
    }
    VisualCurvePosManager::GetInstance().setCurrentPointInfo(std::string(), 0);
    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_ACTION::MOUSE_PRESS_NONE);
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveNodeTreeView::slotButtonDelegateClicked(const QModelIndex &index) {
    QStandardItem *item = model_->itemFromIndex(index.siblingAtColumn(0));
    std::string curvePath = curveFromItem(item).toStdString();
    Folder *folder{nullptr};
    SCurveProperty *curveProp{nullptr};
    if (folderDataMgr_->folderFromCurveName(curvePath, folder, curveProp)) {
        if (folder) {
            if (curveProp) {
                curveProp->visible_ = !curveProp->visible_;
                if (curveProp->visible_) {
                    VisualCurvePosManager::GetInstance().deleteHidenCurve(curvePath);
                } else {
                    VisualCurvePosManager::GetInstance().insertHidenCurve(curvePath);
                }
            } else {
                bool visible = folder->isVisible();
                folder->setVisible(!visible);
                for (auto it : folder->getCurveList()) {

                }
            }
        }
    }
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveNodeTreeView::slotDeleteCurveFromVisualCurve(std::string curve) {
    QStringList list = QString::fromStdString(curve).split("|");
    QString root = list.takeFirst();
    QStandardItem *item{nullptr};
    for (int i{0}; i < model_->rowCount(); i++) {
        if (model_->item(i)->text() == root) {
            item = model_->item(i);
        }
    }
    if (!item) {
        return;
    }

    for (const QString &node : list) {
        if (!itemFromPath(item, node)) {
            return;
        }
    }
    QModelIndex index = item->index();
    model_->removeRow(index.row(), index.parent());

    Folder *folder{nullptr};
    SCurveProperty *curveProp{nullptr};
    if (folderDataMgr_->folderFromCurveName(curve, folder, curveProp)) {
        if (folder) {
            folder->deleteCurve(curveProp->curve_);
        }
    }
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveNodeTreeView::searchCurve(NodeData *pNode, std::string &property, std::string curve, std::string sampleProp) {
    if(!pNode)
        return ;

    std::map<std::string, std::string> bindingMap;
    if (pNode->NodeExtendRef().curveBindingRef().getPropCurve(sampleProp, bindingMap)) {
        for (const auto &it : bindingMap) {
            if (it.second.compare(curve)) {
                property = it.first;
                return;
            }
        }
    }

    for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
        searchCurve(&(it->second), property, curve, sampleProp);
    }
}

bool VisualCurveNodeTreeView::itemFromPath(QStandardItem *item, QString node) {
    for (int i{0}; i < item->rowCount(); i++) {
        QStandardItem *tempItem = item->child(i);
        if (tempItem->text() == node) {
            item = tempItem;
            return true;
        }
    }
    return false;
}

QString VisualCurveNodeTreeView::curveFromItem(QStandardItem *item) {
    QString curve = item->text();
    while(item->parent()) {
        item = item->parent();
        QString tempStr = item->text() + "|";
        curve.insert(0, tempStr);
    }
    return curve;
}
}
