/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#ifndef STARTER_H
#define STARTER_H

#include <QString>
#include <QMutex>
#include <QDateTime>

#include "crawler.h"

class Record {
public:
    uint secs;
    QString url;
    bool ok;
    uint loadCounter;
    uint byteCounter;
};

class Starter : public QObject {
    Q_OBJECT

private:
    int count;
    QMutex mutex;
    QDateTime timestart;
    QList<Record> recs;

public:
    Starter();
    bool run(const QString filename);

public slots:
    void done(WebCrawler *wc, bool ok);
};

#endif // STARTER_H
