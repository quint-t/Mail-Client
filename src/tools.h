#ifndef TOOLS_H
#define TOOLS_H

#include <QList>
#include <QString>
#include <QStringConverter>

QString utf7encode(QString str)
{
    auto toUtf16be = QStringEncoder(QStringConverter::Utf16BE);
    QString unipart, out;
    str.replace("&", "&-");
    for (QChar ch : str)
    {
        uint16_t i16 = ch.unicode();
        if (0x20 <= i16 && i16 <= 0x7f)
        {
            if (!unipart.isEmpty())
            {
                QByteArray unipart16be_bytes = (QByteArray)toUtf16be(unipart);
                QString encoded_string(unipart16be_bytes.toBase64(
                    QByteArray::Base64Option::Base64Encoding | QByteArray::Base64Option::OmitTrailingEquals |
                    QByteArray::Base64Option::IgnoreBase64DecodingErrors));
                out += '&' + encoded_string.replace('/', ',') + '-';
                unipart.clear();
            }
            out += ch;
        }
        else
        {
            unipart += ch;
        }
    }
    if (!unipart.isEmpty())
    {
        QByteArray unipart16be_bytes = (QByteArray)toUtf16be(unipart);
        QString encoded_string(unipart16be_bytes.toBase64(QByteArray::Base64Option::Base64Encoding |
                                                          QByteArray::Base64Option::OmitTrailingEquals |
                                                          QByteArray::Base64Option::IgnoreBase64DecodingErrors));
        out += '&' + encoded_string.replace('/', ',') + '-';
        unipart.clear();
    }
    return out;
}

QString utf7decode(const QString &str)
{
    auto fromUtf16be = QStringDecoder(QStringConverter::Utf16BE);
    QList<QString> parts = str.split('&', Qt::KeepEmptyParts);
    if (parts.empty())
    {
        return QString();
    }
    QString out = parts[0];
    for (qsizetype i = 1, n = parts.size(); i < n; ++i)
    {
        QString part = parts[i], first, last;
        qsizetype pos = part.indexOf('-');
        if (pos != -1)
        {
            first = part.left(pos);
            last = part.right(part.size() - pos - 1);
        }
        else
        {
            first = part;
        }
        if (first.isEmpty())
        {
            out += '&';
        }
        else
        {
            first = first.replace(',', '/');
            QByteArray decoded_bytes =
                QByteArray::fromBase64(first.toUtf8(), QByteArray::Base64Option::Base64Encoding |
                                                           QByteArray::Base64Option::OmitTrailingEquals |
                                                           QByteArray::Base64Option::IgnoreBase64DecodingErrors);
            out += fromUtf16be(decoded_bytes);
        }
        out += last;
    }
    return out;
}

#endif // TOOLS_H
