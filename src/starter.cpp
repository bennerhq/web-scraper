/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#include <QApplication>

#include "starter.h"
#include "main.h"

#include <QDebug>

Starter::Starter()
{
    timestart = QDateTime::currentDateTime();
    count = 0;
}

bool Starter::run(const QString filename)
{
    bool res;
    mutex.lock();

    WebCrawler *crawler = new WebCrawler();
    connect(crawler, SIGNAL(done(WebCrawler *, bool)), this, SLOT(done(WebCrawler *, bool)));
    if (crawler->go(filename)) {
        count ++;
        res = true;
    }
    else {
        delete crawler;
        res = false;
    }

    mutex.unlock();
    return res;
}

void Starter::done(WebCrawler *wc, bool ok)
{
    Q_UNUSED(wc);
    Q_UNUSED(ok);

    mutex.lock();

    Record rec;
    rec.secs = timestart.secsTo(QDateTime::currentDateTime());
    rec.ok = ok;
    rec.url = wc->getUrl();
    rec.loadCounter = wc->getLoadCounrer();
    recs += rec;

    count --;
    if (count == 0) {
        uint totalLoaded = 0;
        uint totalBytes = 0;
        for (int j = 0; j < recs.size(); j++) {
            Record rec = recs.at(j);
            int secs = rec.secs;
            uint s = secs % 60; secs = secs / 60;
            uint m = secs % 60; secs = secs / 60;
            uint h = secs;

            QString show = "";
            if (h)
                show += QString::number(h) + " hr" + (h > 1 ? "s" : " ") + " ";
            if (h || m)
                show += QString::number(m) + " min" + (m > 1 ? "s" : " ") + " ";
            if (h || m || s)
                show += QString::number(s) + " sec" + (s > 1 ? "s" : " ") + " ";

            LOG1(rec.url
                 << ": " << rec.loadCounter << " pages (" << rec.byteCounter/1024/1024 << " MB) "
                 << "loaded in " << show << (rec.ok ? "" : " FAILED!!!"));

            totalLoaded += rec.loadCounter;
            totalBytes += rec.byteCounter;
        }

        LOG1(totalLoaded << " pages loaded / " << totalBytes/1024/1024 << " MB");

        QApplication::exit(0);
    }

    mutex.unlock();
}
