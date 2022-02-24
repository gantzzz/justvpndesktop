#ifndef SERVERSLISTITEMDELEGATE_H
#define SERVERSLISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>
#include <serverdatamodel.h>

class ServersListItemDelegate : public QStyledItemDelegate
{
public:
    ServersListItemDelegate()
    {
        width = 200;
        height = 30;
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
    {
        return QSize(width, height);
    }
private:
    int width;
    int height;
};

#endif // SERVERSLISTITEMDELEGATE_H
