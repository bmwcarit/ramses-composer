#ifndef PROPERTYTABLEDELEGATE_H
#define PROPERTYTABLEDELEGATE_H


#include <QStyledItemDelegate>
#include <QComboBox>
#include <QTextEdit>
namespace raco::property {
class PropertyTableDelegate : public QStyledItemDelegate
{
public:
    PropertyTableDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;

};

class PropertyTextDelegate : public QStyledItemDelegate{
public:
    PropertyTextDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};
}
#endif // PROPERTYTABLEDELEGATE_H
