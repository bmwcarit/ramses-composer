#include "property/PropertyTableDelegate.h"

namespace raco::property {
PropertyTableDelegate::PropertyTableDelegate(QObject *parent) : QStyledItemDelegate(parent) {

}

QWidget *PropertyTableDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {

    if (index.column() == 1) {
        QComboBox *box = new QComboBox(parent);
        box->addItem("FLOAT", Qt::DisplayRole);
        box->addItem("INT", Qt::DisplayRole);
        box->addItem("BOOL", Qt::DisplayRole);
        box->addItem("STRING", Qt::DisplayRole);
        box->addItem("SAMPLETEXTURE", Qt::DisplayRole);
        box->addItem("VEC2i", Qt::DisplayRole);
        box->addItem("VEC3i", Qt::DisplayRole);
        box->addItem("VEC4i", Qt::DisplayRole);
        box->addItem("VEC2f", Qt::DisplayRole);
        box->addItem("VEC3f", Qt::DisplayRole);
        box->addItem("VEC4f", Qt::DisplayRole);
        box->addItem("MTX2", Qt::DisplayRole);
        box->addItem("MTX3", Qt::DisplayRole);
        box->addItem("MTX4", Qt::DisplayRole);
        return box;
    }

    return nullptr;
}

void PropertyTableDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    dynamic_cast<QComboBox*>(editor)->setEditText(index.data(Qt::DisplayRole).toString());
}

void PropertyTableDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    model->setData(index, dynamic_cast<QComboBox*>(editor)->currentText(), Qt::DisplayRole);
}

PropertyTextDelegate::PropertyTextDelegate(QObject *parent) {

}

QWidget *PropertyTextDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (index.column() == 0) {
        QTextEdit *edit = new QTextEdit(parent);
        return edit;
    }

    return nullptr;
}

void PropertyTextDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    dynamic_cast<QTextEdit*>(editor)->setPlainText(index.data(Qt::DisplayRole).toString());
}

void PropertyTextDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    model->setData(index, dynamic_cast<QTextEdit*>(editor)->toPlainText(), Qt::DisplayRole);
}

}
