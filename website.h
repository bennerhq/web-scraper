/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#ifndef WEBSITE_H
#define WEBSITE_H

#include <QString>
#include <QList>
#include <QXmlStreamWriter>
#include <QIODevice>

enum TagTypes {
    TAG_UNKNOWN,
    TAG_INNER,
    TAG_PLAIN,
    TAG_ATTRIBUTE,
    TAG_EXIST,
    TAG_COUNT,
};

typedef struct {
    QList<QString> find;
    TagTypes type;
    QString attribute;
    QString name;
    int pauseMsec; // FIX ME!!!
} Tag;

typedef struct {
    QList<QString> find;
    QString href;
    int pauseMsec; // FIX ME!!!
    bool category;
} Value;

class Website
{
public:
    QString shopName;
    QString start;
    QString base;
    QList<Value> values;
    QList<Tag> tags;
    QString output;
    QString include;
    QString dups;
    QString mustTag;

    bool load(const QString filename, bool clear = true);

    void dumpXmlStream(QXmlStreamWriter *stream);
    void dumpXml(QIODevice *out = NULL);
    void dumpUnix(QIODevice *out = NULL);
};

#endif // WEBSITE_H
