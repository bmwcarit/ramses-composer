#include "visual_curve/VisualCurveNodeTreeView.h"
#include "visual_curve/VisualCurvePosManager.h"

namespace raco::visualCurve {

TreeModel::TreeModel(QWidget *parent) {

}

void TreeModel::setFolderData(FolderDataManager *mgr) {
    folderDataMgr_ = mgr;
}

bool TreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    QByteArray array = data->data("test");
    QDataStream stream(&array, QIODevice::ReadOnly);
    qint64 p;
    stream >> p;
    QModelIndex* index = (QModelIndex*)p;
    QStandardItem *parentItem = itemFromIndex(parent);
    QStandardItem *item = itemFromIndex(*index);

    if (index) {
        if (parentItem && item) {
            std::string folder = parentItem->text().toStdString();
            if (folderDataMgr_->hasFolderData(folder)) {
                FolderData folderData = folderDataMgr_->getFolderData(folder);
                std::string file = item->text().toStdString();
                if (item->parent()) {
                    std::string sourcefolder = item->parent()->text().toStdString();
                    FolderData sourceFolderData = folderDataMgr_->getFolderData(sourcefolder);
                    SCurveProperty curve = sourceFolderData.getCurve(file);
                    sourceFolderData.deleteCurve(file);
                    folderDataMgr_->replaceFolder(sourcefolder, sourceFolderData);

                    folderData.insertCurve(file, curve.curve_, curve.visible_);
                    folderDataMgr_->replaceFolder(folder, folderData);
                } else {
                    if (!folderDataMgr_->hasCurve(file)) {
                        return false;
                    }
                    SCurveProperty curve = folderDataMgr_->getCurve(file);
                    folderDataMgr_->deleteCurve(file);
                    folderData.insertCurve(file, curve.curve_, curve.visible_);
                    folderDataMgr_->replaceFolder(folder, folderData);
                }
                move(index->parent(), index->row(), parent, parentItem->rowCount());
            } else {
                return false;
            }
        } else {
            if (item) {
                if (!item->parent()) {
                    return false;
                }
                std::string folder = item->parent()->text().toStdString();
                std::string file = item->text().toStdString();
                if (folderDataMgr_->hasFolderData(folder)) {
                    FolderData folderData = folderDataMgr_->getFolderData(folder);
                    if (folderData.hasCurve(file)) {
                        SCurveProperty curve = folderData.getCurve(file);
                        folderData.deleteCurve(file);
                        folderDataMgr_->replaceFolder(folder, folderData);

                        folderDataMgr_->insertCurve(file, curve.curve_, curve.visible_);
                    }
                }
            }
            move(index->parent(), index->row(), parent, rowCount());
        }
    }
    delete index;
    return true;
}

Qt::DropActions TreeModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QMimeData *TreeModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData* mimeData = QAbstractItemModel::mimeData(indexes);

    for (int i = 0; i < indexes.count(); i++) {
        QModelIndex index = indexes[i];
        QStandardItem *item = itemFromIndex(index);
        if (!item->hasChildren()) {
            QModelIndex* p = new QModelIndex(index);
            QByteArray array;
            QDataStream stream(&array, QIODevice::WriteOnly);
            stream << (qint64)p;
            mimeData->setData(QString("test"), array);
        }
        return mimeData;
    }
    return mimeData;
}

bool TreeModel::move(QModelIndex source, int sourceRow, QModelIndex dest, int destRow) {
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
            to = sourceRow < to ? to - 1 : to;
        }

        QStandardItem *destItem = item(to);
        QModelIndex destIndex = item(to)->index();
        blockSignals(true);
        beginInsertRows(destIndex, destRow, destRow);
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
    deleteCurve_ = new QAction("Delete Curve");

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualCurveNodeTreeView::customContextMenuRequested, this, &VisualCurveNodeTreeView::slotShowContextMenu);
    connect(createFolder_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotCreateFolder);
    connect(deleteFolder_, &QAction::triggered, this, &VisualCurveNodeTreeView::slotDeleteFolder);
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
        folderDataMgr_->insertCurve(curve->getCurveName(), curve->getCurveName());
        model_->appendRow(curveItem);
    }

//    std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
//    NodeData* nodeData = NodeDataManager::GetInstance().getActiveNode();
//    std::map<std::string, std::string> bindingMap;
//    if (nodeData) {
//        nodeData->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty, bindingMap);
//    }

//    for (auto it : bindingMap) {
//        std::list<Curve *> curveList = CurveManager::GetInstance().getCurveList();
//        std::string curve = it.second;
//        QStandardItem *curveItem = new QStandardItem(QString::fromStdString(curve));
//        if (!folderDataMgr_->hasFile(curve)) {
//            folderDataMgr_->insertCurve(curve, curve);
//        }
//        model_->appendRow(curveItem);
//    }
}

void VisualCurveNodeTreeView::switchCurSelCurve(std::string curve) {
    if (folderDataMgr_->hasFile(curve)) {
        std::string file = folderDataMgr_->getFile(curve);
        for (int i{0}; i < model_->rowCount(); i++) {
            QStandardItem *item = model_->item(i);
            if (item->text().compare(QString::fromStdString(file)) == 0) {
                visualCurveTreeView_->setCurrentIndex(item->index());
                slotCurrentRowChanged(item->index());
            }
        }
    } else {
        for (auto it : folderDataMgr_->getFolderMap()) {
            if (it.second.hasFile(curve)) {
                std::string file = it.second.getFile(curve);
                std::string folder = it.first;
                for (int i{0}; i < model_->rowCount(); i++) {
                    QStandardItem *item = model_->item(i);
                    if (item->text().compare(QString::fromStdString(folder)) == 0) {
                        for (int j{0}; j < item->rowCount(); ++j) {
                            QStandardItem *childItem = item->child(j);
                            if (childItem->text().compare(QString::fromStdString(file)) == 0) {
                                visualCurveTreeView_->setCurrentIndex(childItem->index());
                                slotCurrentRowChanged(childItem->index());
                            }
                        }
                    }
                }
            }
        }
    }
}

void VisualCurveNodeTreeView::cancleSelCurve() {
    visualCurveTreeView_->setCurrentIndex(QModelIndex());
}

void VisualCurveNodeTreeView::slotInsertCurve(QString property, QString curve, QVariant value) {
    QStandardItem *curveItem = new QStandardItem(curve);
    model_->appendRow(curveItem);
    folderDataMgr_->insertCurve(curve.toStdString(), curve.toStdString());
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
        std::string node = item->text().toStdString();
        if (folderDataMgr_->hasFolderData(node)) {
            menu_->addAction(deleteFolder_);
        } else {
            menu_->addAction(deleteCurve_);
        }
    }
    menu_->addAction(createFolder_);
    menu_->exec(mapToGlobal(p));
}

void VisualCurveNodeTreeView::slotCreateFolder() {
    std::string folder = folderDataMgr_->createDefaultFolder();
    QStandardItem *folderItem = new QStandardItem(QString::fromStdString(folder));
    folderDataMgr_->insertFolderData(folder, FolderData());
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

            std::string folder = item->text().toStdString();
            if (folderDataMgr_->hasFolderData(folder)) {
                for (const auto &it : folderDataMgr_->getFolderData(folder).getCurveMap()) {
                    CurveManager::GetInstance().delCurve(it.second.curve_);
                    VisualCurvePosManager::GetInstance().deleteKeyPointList(it.second.curve_);
                    VisualCurvePosManager::GetInstance().deleteWorkerPointList(it.second.curve_);
                }
                folderDataMgr_->deleteFolderData(folder);
            }
        }
        model_->removeRow(selected.row());
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveNodeTreeView::slotDeleteCurve() {
    QModelIndex selected = visualCurveTreeView_->currentIndex();
    QStandardItem *item = model_->itemFromIndex(selected);
    QStandardItem *parentItem = item->parent();
    if (item) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Ramses Composer",
            tr("Delete Curve?\n"),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (resBtn == QMessageBox::No) {
            return;
        }

        if (parentItem) {
            std::string folder = parentItem->text().toStdString();
            std::string file = item->text().toStdString();
            if (folderDataMgr_->hasFolderData(folder)) {
                FolderData folderData = folderDataMgr_->getFolderData(folder);
                if (folderData.hasCurve(file)) {
                    std::string curve = folderData.getCurve(file).curve_;
                    folderData.deleteCurve(file);
                    folderDataMgr_->replaceFolder(folder, folderData);
                    CurveManager::GetInstance().delCurve(curve);
                    VisualCurvePosManager::GetInstance().deleteKeyPointList(curve);
                    VisualCurvePosManager::GetInstance().deleteWorkerPointList(curve);
                }
            }
        } else {
            std::string file = item->text().toStdString();
            if (folderDataMgr_->hasCurve(file)) {
                std::string curve = folderDataMgr_->getCurve(file).curve_;
                folderDataMgr_->deleteCurve(curve);
                CurveManager::GetInstance().delCurve(curve);
                VisualCurvePosManager::GetInstance().deleteKeyPointList(curve);
                VisualCurvePosManager::GetInstance().deleteWorkerPointList(curve);
            }
        }
        model_->removeRow(selected.row(), selected.parent());
        Q_EMIT sigRefreshVisualCurve();
    }
}

void VisualCurveNodeTreeView::slotItemChanged(QStandardItem *item) {
    QStandardItem *parentItem = item->parent();
    if (parentItem) {
        std::string folder = parentItem->text().toStdString();
        if (folderDataMgr_->hasFolderData(folder)) {
            FolderData folderData = folderDataMgr_->getFolderData(folder);
            std::string file = item->text().toStdString();
            if (folderData.hasCurve(selNode_)) {
                if (folderData.hasCurve(file)) {
                    item->setText(QString::fromStdString(selNode_));
                    return;
                }
                folderData.swapCurve(selNode_, file);
                folderDataMgr_->replaceFolder(folder, folderData);
            }
        }
    } else {
        std::string node = item->text().toStdString();
        if (folderDataMgr_->hasCurve(selNode_)) {
            if (folderDataMgr_->hasCurve(node)) {
                item->setText(QString::fromStdString(selNode_));
                return;
            }
            folderDataMgr_->swapCurve(selNode_, node);
        }
        if (folderDataMgr_->hasFolderData(selNode_)) {
            if (folderDataMgr_->hasFolderData(node)) {
                item->setText(QString::fromStdString(selNode_));
                return;
            }
            folderDataMgr_->swapFolder(selNode_, node);
        }
    }
}

void VisualCurveNodeTreeView::slotCurrentRowChanged(const QModelIndex &index) {
    QStandardItem *item = model_->itemFromIndex(index);
    selNode_ = item->text().toStdString();

    std::string curCurve = VisualCurvePosManager::GetInstance().getCurrentPointInfo().first;
    if (item->parent()) {
        QStandardItem *parentItem = item->parent();
        std::string folder = parentItem->text().toStdString();
        std::string file = item->text().toStdString();
        if (folderDataMgr_->hasFolderData(folder)) {
            if (folderDataMgr_->getFolderData(folder).hasCurve(file)) {
                std::string curve = folderDataMgr_->getFolderData(folder).getCurve(file).curve_;
                if (curCurve != curve) {
                    VisualCurvePosManager::GetInstance().setCurrentPointInfo(curve, 0);
                    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_ACTION::MOUSE_PRESS_KEY);
                    Q_EMIT sigSwitchVisualCurveInfoWidget();
                    Q_EMIT sigRefreshVisualCurve();
                }
                return;
            }
        }
    } else {
        std::string file = item->text().toStdString();
        if (folderDataMgr_->hasCurve(file)) {
            std::string curve = folderDataMgr_->getCurve(file).curve_;
            if (curCurve != curve) {
                VisualCurvePosManager::GetInstance().setCurrentPointInfo(curve, 0);
                VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_ACTION::MOUSE_PRESS_KEY);
                Q_EMIT sigSwitchVisualCurveInfoWidget();
                Q_EMIT sigRefreshVisualCurve();
            }
            return;
        }
    }
    VisualCurvePosManager::GetInstance().setCurrentPointInfo(std::string(), 0);
    VisualCurvePosManager::GetInstance().setPressAction(MOUSE_PRESS_ACTION::MOUSE_PRESS_NONE);
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveNodeTreeView::slotButtonDelegateClicked(const QModelIndex &index) {
    QStandardItem *item = model_->itemFromIndex(index);
    bool isVisible{true};
    if (item->parent()) {
        std::string folder = item->parent()->text().toStdString();
        if (folderDataMgr_->hasFolderData(folder)) {
            std::string file = model_->itemFromIndex(index.siblingAtColumn(0))->text().toStdString();
            if (folderDataMgr_->getFolderData(folder).hasCurve(file)) {
                SCurveProperty curve = folderDataMgr_->getFolderData(folder).getCurve(file);
                isVisible = !curve.visible_;
                folderDataMgr_->replaceFolderCurve(folder, file, curve.curve_, isVisible);
                if (isVisible) {
                    VisualCurvePosManager::GetInstance().deleteHidenCurve(curve.curve_);
                } else {
                    VisualCurvePosManager::GetInstance().insertHidenCurve(curve.curve_);
                }
            }
        }
    } else {
        std::string node = model_->itemFromIndex(index.siblingAtColumn(0))->text().toStdString();
        if (folderDataMgr_->hasFolderData(node)) {
            FolderData folderData = folderDataMgr_->getFolderData(node);
            folderDataMgr_->setFolderVisible(node, !folderData.isVisible());
            isVisible = !folderData.isVisible();

            for (const auto &it : folderData.getCurveMap()) {
                if (isVisible) {
                    VisualCurvePosManager::GetInstance().deleteHidenCurve(it.first);
                } else {
                    VisualCurvePosManager::GetInstance().insertHidenCurve(it.first);
                }
            }
        }
        if (folderDataMgr_->hasCurve(node)) {
            std::string curve = folderDataMgr_->getCurve(node).curve_;
            isVisible = !folderDataMgr_->getCurve(node).visible_;
            folderDataMgr_->modifyCurve(node, curve, isVisible);
            if (isVisible) {
                VisualCurvePosManager::GetInstance().deleteHidenCurve(curve);
            } else {
                VisualCurvePosManager::GetInstance().insertHidenCurve(curve);
            }
        }
    }
    Q_EMIT sigRefreshVisualCurve();
}

void VisualCurveNodeTreeView::slotDeleteCurveFromVisualCurve(std::string curve) {
    if (folderDataMgr_->hasFile(curve)) {
        std::string file = folderDataMgr_->getFile(curve);
        folderDataMgr_->deleteCurve(file);
    } else {
        for (auto it : folderDataMgr_->getFolderMap()) {
            if (it.second.hasFile(curve)) {
                std::string folder = it.first;
                std::string file = it.second.getFile(curve);
                FolderData folderData = it.second;
                folderData.deleteCurve(file);
                folderDataMgr_->replaceFolder(folder, folderData);
            }
        }
    }
    QString strCurve = QString::fromStdString(curve);
    for (int i{0}; i < model_->rowCount(); ++i) {
        QStandardItem *tempItem = model_->item(i);
        if (tempItem->text().compare(strCurve) == 0) {
            model_->removeRow(tempItem->row());
            return;
        }
        if (tempItem->hasChildren()) {
            for (int j{0}; j < tempItem->rowCount(); ++j) {
                QStandardItem *childItem = tempItem->child(j);
                if (childItem->text().compare(strCurve) == 0) {
                    tempItem->removeRow(childItem->row());
                    return;
                }
            }
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
}
