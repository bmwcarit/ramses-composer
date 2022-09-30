#include "visual_curve/VisualCurveNodeTreeView.h"
#include "visual_curve/VisualCurvePosManager.h"

namespace raco::visualCurve {

TreeModel::TreeModel(QWidget *parent) {

}

void TreeModel::setFolderDataMgr(FolderDataManager *mgr) {
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

    QVector<qint64> vector;
    QByteArray array = data->data("test");
    QDataStream stream(&array, QIODevice::ReadOnly);
    stream >> vector;
    QModelIndex tempParent = parent;
    QStandardItem *parentItem = itemFromIndex(parent);

    for (auto p : qAsConst(vector)) {
        QModelIndex* index = (QModelIndex*)p;
        if (index) {
            QStandardItem *item = itemFromIndex(*index);
            std::string srcCurvePath = curveFromItem(item).toStdString();
            if (folderDataMgr_->isCurve(srcCurvePath)) {
                Folder *srcFolder{nullptr};
                SCurveProperty *srcCurveProp{nullptr};
                if (folderDataMgr_->curveFromPath(srcCurvePath, &srcFolder, &srcCurveProp)) {
                    if (parentItem) {
                        // curve move to node
                        std::string destCurvePath = curveFromItem(parentItem).toStdString();
                        if (!moveCurveToNode(srcFolder, srcCurveProp, srcCurvePath, destCurvePath)) {
                            return false;
                        }
                        move(index->parent(), index->row(), tempParent, parentItem->rowCount());
                    } else {
                        // curve move to default node
                        if (srcFolder == folderDataMgr_->getRootFolder()) {
                            return false;
                        }
                        moveCurveToDefaultNode(srcFolder, srcCurveProp, srcCurvePath);
                        move(index->parent(), index->row(), tempParent, rowCount());
                    }
                }
            } else {
                Folder *srcFolder{nullptr};
                if (folderDataMgr_->folderFromPath(srcCurvePath, &srcFolder)) {
                    if (parentItem) {
                        // move folder to node
                        std::string destCurvePath = curveFromItem(parentItem).toStdString();
                        if (!moveFolderToNode(srcFolder, srcCurvePath, destCurvePath)) {
                            return false;
                        }
                        move(index->parent(), index->row(), tempParent, parentItem->rowCount());
                    } else {
                        // move folder to default node
                        if (srcFolder == folderDataMgr_->getRootFolder() || folderDataMgr_->getRootFolder()->hasFolder(srcFolder->getFolderName())) {
                            return false;
                        }
                        moveFolderToDefaultNode(srcFolder, srcCurvePath);
                        move(index->parent(), index->row(), tempParent, rowCount());
                    }
                }
            }
            delete index;
        }
    }
    Q_EMIT signal::signalProxy::GetInstance().sigCheckCurveBindingValid_From_CurveUI();
    return true;
}

Qt::DropActions TreeModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QMimeData *TreeModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData* mimeData = QAbstractItemModel::mimeData(indexes);

    auto sortInsertData = [&](QVector<qint64> &vec, QModelIndex* p) {
        for (int i{0}; i < vec.size(); ++i) {
            QModelIndex *tIndex = (QModelIndex*)vec.at(i);
            if (tIndex->row() < p->row()) {
                vec.insert(i, (qint64)p);
                return;
            }
        }
        vec.push_front((qint64)p);
    };

    QVector<qint64> vector;
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    for (int i = 0; i < indexes.count(); i++) {
        QModelIndex index = indexes[i];
        QModelIndex* p = new QModelIndex(index);
        QStandardItem *item = itemFromIndex(*p);
        if (item->text().compare("") != 0) {
            sortInsertData(vector, p);
        }
    }
    stream << vector;
    mimeData->setData(QString("test"), array);
    return mimeData;
}

bool TreeModel::move(QModelIndex source, int sourceRow, QModelIndex &dest, int destRow) {
    if (sourceRow < 0 || destRow < 0) {
        return false;
    }
    if(!beginMoveRows(source, sourceRow, sourceRow, dest, destRow)){
        return false;
    }

    beginInsertRows(dest, destRow, destRow);
    if (dest.isValid()) {
        QStandardItem *destItem = itemFromIndex(dest);
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

        blockSignals(true);
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
//        beginInsertRows(dest, destRow, destRow);
        insertRow(destRow, itemList);
//        endInsertRows();
        blockSignals(false);
    }
    endMoveRows();
    return true;
}

void TreeModel::swapCurve(std::string oldCurve, std::string newCurve) {
    QList<SKeyPoint> keyPoints;
    QList<QPair<QPointF, QPointF> > workerPoints;
    VisualCurvePosManager::GetInstance().getKeyPointList(oldCurve, keyPoints);
    VisualCurvePosManager::GetInstance().getWorkerPointList(oldCurve, workerPoints);
    VisualCurvePosManager::GetInstance().deleteKeyPointList(oldCurve);
    VisualCurvePosManager::GetInstance().deleteWorkerPointList(oldCurve);
    VisualCurvePosManager::GetInstance().addKeyPointList(newCurve, keyPoints);
    VisualCurvePosManager::GetInstance().addWorkerPointList(newCurve, workerPoints);
}

void TreeModel::setFolderPath(Folder *folder, std::string path) {
    if(!folder)
        return ;

    for (const auto &it : folder->getCurveList()) {
        std::string curveName;
        folderDataMgr_->pathFromCurve(it->curve_, folder, curveName);
        std::string oldCurvePath = path + "|" + it->curve_;
        if (CurveManager::GetInstance().getCurve(oldCurvePath)) {
            Curve *tempCurve = CurveManager::GetInstance().getCurve(oldCurvePath);
            tempCurve->setCurveName(curveName);
            swapCurve(oldCurvePath, curveName);
        }
    }

    std::list<Folder *> folderList = folder->getFolderList();
    for (auto it = folderList.begin(); it != folderList.end(); ++it) {
        setFolderPath(*it, std::string(path + "|" + (*it)->getFolderName()));
    }
}

bool TreeModel::moveCurveToNode(Folder *srcFolder, SCurveProperty *srcCurveProp, std::string srcCurvePath, std::string destCurvePath) {
    Folder *destFolder{nullptr};
    if (!folderDataMgr_->isCurve(destCurvePath)) {
        if (folderDataMgr_->folderFromPath(destCurvePath, &destFolder)) {
            if (!destFolder->hasCurve(srcCurveProp->curve_)) {
                srcFolder->takeCurve(srcCurveProp->curve_);
                destFolder->insertCurve(srcCurveProp);
                if (CurveManager::GetInstance().getCurve(srcCurvePath)) {
                    destCurvePath = destCurvePath + "|" + srcCurveProp->curve_;
                    Curve *curve = CurveManager::GetInstance().getCurve(srcCurvePath);
                    curve->setCurveName(destCurvePath);
                    swapCurve(srcCurvePath, destCurvePath);
                }
                return true;
            }
        }
    }
    return false;
}

bool TreeModel::moveCurveToDefaultNode(Folder *srcFolder, SCurveProperty *srcCurveProp, std::string srcCurvePath) {
    srcFolder->takeCurve(srcCurveProp->curve_);
    folderDataMgr_->getRootFolder()->insertCurve(srcCurveProp);
    if (CurveManager::GetInstance().getCurve(srcCurvePath)) {
        Curve *curve = CurveManager::GetInstance().getCurve(srcCurvePath);
        curve->setCurveName(srcCurveProp->curve_);
        swapCurve(srcCurvePath, srcCurveProp->curve_);
    }
    return true;
}

bool TreeModel::moveFolderToNode(Folder *srcFolder, std::string srcCurvePath, std::string destCurvePath) {
    if (srcFolder->parent()) {
        srcFolder->parent()->takeFolder(srcFolder->getFolderName());
    } else {
        srcFolder->takeFolder(srcCurvePath);
    }

    Folder *destFolder{nullptr};
    SCurveProperty *destCurveProp{nullptr};
    if (!folderDataMgr_->isCurve(destCurvePath)) {
        if (folderDataMgr_->folderFromPath(destCurvePath, &destFolder)) {
            destFolder->insertFolder(srcFolder);
            setFolderPath(srcFolder, srcCurvePath);
            return true;
        }
    }
    return false;
}

bool TreeModel::moveFolderToDefaultNode(Folder *srcFolder, std::string srcCurvePath) {
    if (srcFolder->parent()) {
        srcFolder->parent()->takeFolder(srcFolder->getFolderName());
    } else {
        srcFolder->takeFolder(srcCurvePath);
    }
    folderDataMgr_->getRootFolder()->insertFolder(srcFolder);
    setFolderPath(srcFolder, srcCurvePath);
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
    model_->setFolderDataMgr(folderDataMgr_);

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
        std::string curvePath = curve->getCurveName();
        QStringList list = QString::fromStdString(curvePath).split("|");
        QString curveName = list.takeLast();

        Folder *folder = folderDataMgr_->getRootFolder();
        QStandardItem *item{nullptr};
        for (const QString &node : list) {
            if (folder->hasFolder(node.toStdString())) {
                if (item) {
                    for (int i{0}; i < item->rowCount(); i++) {
                        if (node.compare(item->child(i)->text()) == 0) {
                            item = item->child(i);
                            break;
                        }
                    }
                } else {
                    for (int i{0}; i < model_->rowCount(); i++) {
                        if (node.compare(model_->item(i)->text()) == 0) {
                            item = model_->item(i);
                            break;
                        }
                    }
                }
                folder = folder->getFolder(node.toStdString());
            } else {
                QStandardItem *nodeItem = new QStandardItem(node);
                if (item) {
                    item->appendRow(nodeItem);
                } else {
                    model_->appendRow(nodeItem);
                }
                item = nodeItem;
                item->setColumnCount(2);
                folder->insertFolder(node.toStdString());
                folder = folder->getFolder(node.toStdString());
            }
        }

        QStandardItem *curveItem = new QStandardItem(curveName);
        folder->insertCurve(curveName.toStdString());
        if (item) {
            item->appendRow(curveItem);
        } else {
            model_->appendRow(curveItem);
        }
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
        if (!itemFromPath(&item, node)) {
            return;
        }
    }
    visualCurveTreeView_->setCurrentIndex(item->index());
    slotCurrentRowChanged(item->index());
}

void VisualCurveNodeTreeView::cancelSelCurve() {
    visualCurveTreeView_->setCurrentIndex(QModelIndex());
}

void VisualCurveNodeTreeView::setFolderVisible(Folder *folder, bool visible) {
    if(!folder)
        return ;

    folder->setVisible(visible);
    for (const auto &it : folder->getCurveList()) {
        it->visible_ = visible;
        std::string curve;
        folderDataMgr_->pathFromCurve(it->curve_, folder, curve);
        if (visible) {
            VisualCurvePosManager::GetInstance().deleteHidenCurve(curve);
        } else {
            VisualCurvePosManager::GetInstance().insertHidenCurve(curve);
        }
    }

    std::list<Folder *> folderList = folder->getFolderList();
    for (auto it = folderList.begin(); it != folderList.end(); ++it) {
        setFolderVisible(*it, visible);
    }
}

void VisualCurveNodeTreeView::slotInsertCurve(QString property, QString curve, QVariant value) {
    QStandardItem *curveItem = new QStandardItem(curve);
    model_->appendRow(curveItem);
    folderDataMgr_->getRootFolder()->insertCurve(curve.toStdString());
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
        if (folderDataMgr_->isCurve(curve)) {
            menu_->addAction(deleteCurve_);
        } else {
            menu_->addAction(deleteFolder_);
        }
    } else {
        visualCurveTreeView_->setCurrentIndex(QModelIndex());
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

        if (folderDataMgr_->isCurve(curve)) {
            Folder *folder{nullptr};
            SCurveProperty *curveProp{nullptr};
            if (folderDataMgr_->curveFromPath(curve, &folder, &curveProp)) {
                std::string defaultFolder = folder->createDefaultFolder();
                QStandardItem *folderItem = new QStandardItem(QString::fromStdString(defaultFolder));
                folder->insertFolder(defaultFolder);
                if (item->parent()) {
                    item->parent()->appendRow(folderItem);
                } else {
                    model_->appendRow(folderItem);
                }
            }
        } else {
            Folder *folder{nullptr};
            if (folderDataMgr_->folderFromPath(curve, &folder)) {
                std::string defaultFolder = folder->createDefaultFolder();
                QStandardItem *folderItem = new QStandardItem(QString::fromStdString(defaultFolder));
                folder->insertFolder(defaultFolder);
                item->appendRow(folderItem);
            }
        }
    } else {
        std::string defaultFolder = folderDataMgr_->getRootFolder()->createDefaultFolder();
        QStandardItem *folderItem = new QStandardItem(QString::fromStdString(defaultFolder));
        folderDataMgr_->getRootFolder()->insertFolder(defaultFolder);
        model_->appendRow(folderItem);
    }
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
            std::string curve = curveFromItem(item).toStdString();
            if (!folderDataMgr_->isCurve(curve)) {
                Folder *folder{nullptr};
                if (folderDataMgr_->folderFromPath(curve, &folder)) {
                    for (auto it : folder->getCurveList()) {
                        CurveManager::GetInstance().delCurve(it->curve_);
                        VisualCurvePosManager::GetInstance().deleteKeyPointList(curve + it->curve_);
                        VisualCurvePosManager::GetInstance().deleteWorkerPointList(curve + it->curve_);
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
    QModelIndex selected = visualCurveTreeView_->currentIndex();
    if (selected.isValid()) {
        QStandardItem *item = model_->itemFromIndex(selected);
        std::string curve = curveFromItem(item).toStdString();

        if (folderDataMgr_->isCurve(curve)) {
            Folder *folder{nullptr};
            SCurveProperty *curveProp{nullptr};
            if (folderDataMgr_->curveFromPath(curve, &folder, &curveProp)) {
                std::string createCurve = folder->createDefaultCurve();
                folder->insertCurve(createCurve);
                QStandardItem *curveItem = new QStandardItem(QString::fromStdString(createCurve));
                if (item->parent()) {
                    item->parent()->appendRow(curveItem);
                } else {
                    model_->appendRow(curveItem);
                }

                Curve *tempCurve = new Curve;
                tempCurve->setCurveName(createCurve);
                CurveManager::GetInstance().addCurve(tempCurve);
            }
        } else {
            Folder *folder{nullptr};
            if (folderDataMgr_->folderFromPath(curve, &folder)) {
                std::string createCurve = folder->createDefaultCurve();
                folder->insertCurve(createCurve);
                QStandardItem *curveItem = new QStandardItem(QString::fromStdString(createCurve));
                item->appendRow(curveItem);

                Curve *tempCurve = new Curve;
                tempCurve->setCurveName(createCurve);
                CurveManager::GetInstance().addCurve(tempCurve);
            }
        }
    } else {
        std::string createCurve = folderDataMgr_->getRootFolder()->createDefaultCurve();
        folderDataMgr_->getRootFolder()->insertCurve(createCurve);
        QStandardItem *curveItem = new QStandardItem(QString::fromStdString(createCurve));
        model_->appendRow(curveItem);

        Curve *tempCurve = new Curve;
        tempCurve->setCurveName(createCurve);
        CurveManager::GetInstance().addCurve(tempCurve);
    }
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
        if (folderDataMgr_->isCurve(curvePath)) {
            if (folderDataMgr_->curveFromPath(curvePath, &folder, &curveProp)) {
                folder->deleteCurve(curve);
                CurveManager::GetInstance().delCurve(curvePath);
                VisualCurvePosManager::GetInstance().deleteKeyPointList(curvePath);
                VisualCurvePosManager::GetInstance().deleteWorkerPointList(curvePath);
            }
            model_->removeRow(selected.row(), selected.parent());
            Q_EMIT sigRefreshVisualCurve();
        }
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
    if (folderDataMgr_->isCurve(curvePath)) {
        if (folderDataMgr_->curveFromPath(curvePath, &folder, &curveProp)) {
            curveProp->curve_ = curve;
            std::string newCurve;
            folderDataMgr_->pathFromCurve(curve, folder, newCurve);
            swapPoints(curvePath, newCurve);
            Q_EMIT signal::signalProxy::GetInstance().sigCheckCurveBindingValid_From_CurveUI();
        }
    }
}

void VisualCurveNodeTreeView::slotCurrentRowChanged(const QModelIndex &index) {
    QStandardItem *item = model_->itemFromIndex(index);
    std::string curvePath = curveFromItem(item).toStdString();
    selNode_ = curvePath;
    qDebug() << QString::fromStdString(curvePath);

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    if (folderDataMgr_->isCurve(curvePath)) {
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

    if (folderDataMgr_->isCurve(curvePath)) {
        Folder *folder{nullptr};
        SCurveProperty *curveProp{nullptr};
        if (folderDataMgr_->curveFromPath(curvePath, &folder, &curveProp)) {
            curveProp->visible_ = !curveProp->visible_;
            if (curveProp->visible_) {
                VisualCurvePosManager::GetInstance().deleteHidenCurve(curvePath);
            } else {
                VisualCurvePosManager::GetInstance().insertHidenCurve(curvePath);
            }
        }
    } else {
        Folder *folder{nullptr};
        if (folderDataMgr_->folderFromPath(curvePath, &folder)) {
            bool visible = !folder->isVisible();
            setFolderVisible(folder, visible);
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
        if (!itemFromPath(&item, node)) {
            return;
        }
    }
    QModelIndex index = item->index();
    model_->removeRow(index.row(), index.parent());

    Folder *folder{nullptr};
    SCurveProperty *curveProp{nullptr};
    if (folderDataMgr_->isCurve(curve)) {
        if (folderDataMgr_->curveFromPath(curve, &folder, &curveProp)) {
            folder->deleteCurve(curveProp->curve_);
        }
        Q_EMIT sigRefreshVisualCurve();
    }
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

bool VisualCurveNodeTreeView::itemFromPath(QStandardItem **item, QString node) {
    QStandardItem *tempItem = *item;
    for (int i{0}; i < tempItem->rowCount(); i++) {
        QStandardItem *childItem = tempItem->child(i);
        if (childItem->text() == node) {
            *item = childItem;
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
