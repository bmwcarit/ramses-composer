#include "visual_curve/VisualCurveNodeDelegate.h"
#include "qabstractitemmodel.h"
#include "qitemselectionmodel.h"
#include "qpainter.h"
#include "qstyleoption.h"
#include "style/Icons.h"
#include "visual_curve/VisualCurveNodeTreeView.h"

namespace raco::visualCurve {
void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QPersistentModelIndex perIndex(index);

    QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index.model());
    QTreeView *treeView = static_cast<QTreeView *>(this->parent());

    connect(model,SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this,SLOT(clearWidget()),Qt::UniqueConnection);
    connect(model,SIGNAL(columnsRemoved(QModelIndex,int,int)),
            this,SLOT(clearWidget()),Qt::UniqueConnection);
    connect(model,SIGNAL(destroyed(QObject*)),
            this,SLOT(clearWidget()),Qt::UniqueConnection);
    connect(model,SIGNAL(modelReset()),this,SLOT(clearWidget()),Qt::UniqueConnection);
    connect(treeView, SIGNAL(collapsed(QModelIndex)), this, SLOT(collapsed(QModelIndex)));

    connect(model,SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),this,SLOT(updateWidget(QModelIndex,QModelIndex)));

    if(!m_iWidgets.contains(perIndex)) {
        QWidget *parentWidget = static_cast<QWidget *>(painter->device());
        if(nullptr == parentWidget)
            return;

        QWidget *tempWidget = this->createEditor(parentWidget,option,index);
        this->setEditorData(tempWidget,index);
        tempWidget->setGeometry(option.rect);
        tempWidget->setVisible(true);
        m_iWidgets.insert(perIndex,tempWidget);
    }
    else {
        QWidget *tempWidget = m_iWidgets.value(perIndex);
        if(tempWidget) {
            tempWidget->setGeometry(option.rect);
        }
    }
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QPushButton *btn = new QPushButton(parent);
    btn->setAutoFillBackground(false);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btn->setIcon(raco::style::Icons::instance().visibilityOn);

    bool visible{true};
    QModelIndex siblingIndex = index.siblingAtColumn(0);
    if (siblingIndex.isValid()) {
        QStandardItem *item = model_->itemFromIndex(siblingIndex);

        if (item->parent()) {
            std::string folder = item->parent()->text().toStdString();
            if (folderMgr_->hasFolderData(folder)) {
                std::string file = item->parent()->child(index.row())->text().toStdString();
                if (folderMgr_->getFolderData(folder).hasCurve(file)) {
                    visible = folderMgr_->getFolderData(folder).getCurve(file).visible_;
                }
            }
        } else {
            std::string node = item->text().toStdString();
            if (folderMgr_->hasFolderData(node)) {
                visible = folderMgr_->getFolderData(node).isVisible();
            }
            if (folderMgr_->hasCurve(node)) {
                visible = folderMgr_->getCurve(node).visible_;
            }
            if (item->hasChildren()) {
                for (int i{0}; i < item->rowCount(); ++i) {
                    QModelIndex childIndex = item->child(i)->index().siblingAtColumn(1);
                    if (childIndex.isValid()) {
                        QPersistentModelIndex perIndex(childIndex);
                        if (m_iWidgets.contains(perIndex)) {
                            QWidget *tempWidget = m_iWidgets.value(perIndex);
                            if (tempWidget) {
                                tempWidget->setVisible(false);
                            }
                        }
                    }
                }
            }
        }
    }
    if (visible) {
        btn->setIcon(raco::style::Icons::instance().visibilityOn);
    } else {
        btn->setIcon(raco::style::Icons::instance().visibilityOff);
    }

    QObject::connect(btn,&QPushButton::clicked,this, &ButtonDelegate::btnClicked);
    return btn;
}

QSize ButtonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QPersistentModelIndex perIndex(index);
    QTreeView *treeView = static_cast<QTreeView *>(this->parent());
    if(m_iWidgets.contains(perIndex)) {
        QWidget *tempWidget = m_iWidgets.value(perIndex);
        if (index.parent().isValid()) {
            if (!treeView->isExpanded(index.parent())) {
                tempWidget->setVisible(false);
            } else {
                tempWidget->setVisible(true);
            }
        }
    }
    return QSize(option.rect.width(),option.rect.height());
}

void ButtonDelegate::clearWidget() {
    auto i = m_iWidgets.begin();
    while (i != m_iWidgets.end()) {
        QWidget *tempWidget = i.value();
        tempWidget->setParent(nullptr);
        tempWidget->deleteLater();
        i = m_iWidgets.erase(i);
    }
}

void ButtonDelegate::updateWidget(QModelIndex begin, QModelIndex end) {
    QItemSelection selection(begin,end);
    QModelIndexList list = selection.indexes();

    foreach (QModelIndex index, list) {
        QPersistentModelIndex perIndex(index);
        if(m_iWidgets.contains(perIndex)) {
            QWidget *tempWidget = m_iWidgets.value(perIndex);
            if(tempWidget) {
                this->setEditorData(tempWidget,index);
            }
        }
    }
}

void ButtonDelegate::collapsed(const QModelIndex &index) {
    if (model_) {
        QStandardItem *parentItem = model_->itemFromIndex(index);
        if (parentItem->hasChildren()) {
            for (int i{0}; i < parentItem->rowCount(); i++) {
                QModelIndex tempIndex = parentItem->child(i)->index().siblingAtColumn(1);
                QPersistentModelIndex perIndex(tempIndex);
                if (m_iWidgets.contains(perIndex)) {
                    QWidget *tempWidget = m_iWidgets.value(perIndex);
                    if (tempWidget) {
                        tempWidget->setVisible(false);
                    }
                }
            }
        }
    }
}

void ButtonDelegate::btnClicked() {
    QPushButton *btn = static_cast<QPushButton *>(sender());
    QModelIndex index;
    for (const auto &it : m_iWidgets.toStdMap()) {
        QPushButton *tempBtn = static_cast<QPushButton *>(it.second);
        if (tempBtn == btn) {
            index = QModelIndex(it.first);
        }
    }

    bool tVisible{true};
    if (!index.isValid()) {
        return;
    }
    QModelIndex tSiblingIndex = index.siblingAtColumn(0);
    if (tSiblingIndex.isValid()) {
        QStandardItem *tItem = model_->itemFromIndex(tSiblingIndex);
        if (tItem->parent()) {
            std::string folder = tItem->parent()->text().toStdString();
            if (folderMgr_->hasFolderData(folder)) {
                std::string file = tItem->parent()->child(index.row())->text().toStdString();
                if (folderMgr_->getFolderData(folder).hasCurve(file)) {
                    tVisible = folderMgr_->getFolderData(folder).getCurve(file).visible_;
                }
            }
        } else {
            std::string node = tItem->text().toStdString();
            if (folderMgr_->hasFolderData(node)) {
                tVisible = folderMgr_->getFolderData(node).isVisible();
            }
            if (folderMgr_->hasCurve(node)) {
                tVisible = folderMgr_->getCurve(node).visible_;
            }
        }
    }
    if (tVisible) {
        btn->setIcon(raco::style::Icons::instance().visibilityOff);
    } else {
        btn->setIcon(raco::style::Icons::instance().visibilityOn);
    }
    auto temp = const_cast<ButtonDelegate *>(this);
    emit temp->clicked(index);
}
}
