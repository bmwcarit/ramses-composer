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
    auto curveFromItem = [=](QStandardItem *item)->QString {
        QString curve = item->text();
        while(item->parent()) {
            item = item->parent();
            QString tempStr = item->text() + "|";
            curve.insert(0, tempStr);
        }
        return curve;
    };

    QPushButton *btn = new QPushButton(parent);
    btn->setAutoFillBackground(false);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btn->setIcon(raco::style::Icons::instance().visibilityOn);

    bool visible{true};
    QModelIndex siblingIndex = index.siblingAtColumn(0);
    if (siblingIndex.isValid()) {
        QStandardItem *item = model_->itemFromIndex(siblingIndex);
        std::string curve = curveFromItem(item).toStdString();

        if (folderMgr_->isCurve(curve)) {
            Folder *folder{nullptr};
            STRUCT_CURVE_PROP *curveProp{nullptr};
            if (folderMgr_->curveFromPath(curve, &folder, &curveProp)) {
                visible = curveProp->visible_;
            }
        } else {
            Folder *folder{nullptr};
            if (folderMgr_->folderFromPath(curve, &folder)) {
                visible = folder->isVisible();
            }
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
        itemCollapsed(parentItem);
    }
}

void ButtonDelegate::btnClicked() {
    auto curveFromItem = [=](QStandardItem *item)->QString {
        QString curve = item->text();
        while(item->parent()) {
            item = item->parent();
            QString tempStr = item->text() + "|";
            curve.insert(0, tempStr);
        }
        return curve;
    };

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
        std::string curve = curveFromItem(tItem).toStdString();

        if (folderMgr_->isCurve(curve)) {
            Folder *folder{nullptr};
            STRUCT_CURVE_PROP *curveProp{nullptr};
            if (folderMgr_->curveFromPath(curve, &folder, &curveProp)) {
                tVisible = curveProp->visible_;
            }
        } else {
            Folder *folder{nullptr};
            if (folderMgr_->folderFromPath(curve, &folder)) {
                tVisible = folder->isVisible();
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

void ButtonDelegate::itemCollapsed(QStandardItem *item) {
    if (item->hasChildren()) {
        for (int i{0}; i < item->rowCount(); i++) {
            QModelIndex tempIndex = item->child(i)->index().siblingAtColumn(1);
            itemCollapsed(item->child(i));
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
