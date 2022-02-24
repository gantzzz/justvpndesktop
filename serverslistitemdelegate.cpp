#include "serverslistitemdelegate.h"
#include "QPainter"
#include <QFontMetrics>

void ServersListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ServerDataModel m = index.data(Qt::UserRole + 1).value<ServerDataModel>();
    if(option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
    }

    QFontMetrics fm(painter->font());
    int nXPos = 30;

    // server country
    auto r = option.rect.adjusted(nXPos, 0, 0, -5);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft|Qt::TextWordWrap|Qt::AlignBottom, m.get_server_country(), &r);
    nXPos += fm.boundingRect(m.get_server_country()).width() + 5;

    // server country flag
    r = option.rect.adjusted(nXPos, 0, 0, 0);
    auto country = m.get_server_country();
    QString sCountryIconPath;
    if (country == "US")
    {
        sCountryIconPath = ":icons/images/ic_us.png";
    }
    else if (country == "NL")
    {
        sCountryIconPath = ":icons/images/ic_nl.png";
    }
    else if (country == "RU")
    {
        sCountryIconPath = ":icons/images/ic_ru.png";
    }
    else
    {
        sCountryIconPath = ":icons/images/ic_unknown.png";
    }

    QPixmap pixmap = QPixmap(sCountryIconPath);
    QSize size = pixmap.size().scaled(r.size(), Qt::KeepAspectRatio);
    painter->drawPixmap(r.x(), r.y(), size.width(), size.height(), pixmap);
    nXPos += size.width() + 5;

    // server address
    r = option.rect.adjusted(nXPos, 0, 0, -5);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft|Qt::TextWordWrap|Qt::AlignBottom, m.get_server_address(), &r);
    nXPos += fm.boundingRect(m.get_server_address()).width() + 5;

    // Draw signal icon
    QString sImgPath;
    switch (m.get_signal())
    {
    case eSIG_BEST:
        sImgPath = ":icons/images/sig_best.png";
        break;

    case eSIG_GOOD:
        sImgPath = ":icons/images/sig_good.png";
        break;

    case eSIG_MID:
        sImgPath = ":icons/images/sig_min.png";
        break;

    case eSIG_LOW:
        sImgPath = ":icons/images/sig_low.png";
        break;

    case eSIG_UNKNOWN:
    default:
        sImgPath = ":icons/images/sig_unknown.png";
        break;
    }

    r = option.rect.adjusted(nXPos, 0, 0, 0);
    pixmap = QPixmap(sImgPath);
    size = pixmap.size().scaled(r.size(), Qt::KeepAspectRatio);
    painter->drawPixmap(r.x(), r.y(), size.width(), size.height(), pixmap);
}
