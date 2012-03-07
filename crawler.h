/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#ifndef CRAWLER_H
#define CRAWLER_H

#include <QMutex>
#include <QXmlStreamWriter>
#include <QUrl>
#include <QSet>
#include <QHash>
#include <QStack>
#include <QWebPage>
#include <QWebFrame>
#include <QThread>
#include <QWaitCondition>
#include <QWebElement>
#include <QWebElementCollection>

#include "website.h"

// -------------------------------------------------------------------------

// this struct is the glue between WebCrawler and WebCrawlerThread! Its
// allocated in WebCrawler and deallocated in WebCrawlerThread

typedef struct {
    QUrl url;
    bool ok;
    Website website;
    QWaitCondition wait;
    QWebElement curr;
    QWebElement elm;
    QWebElementCollection elms;
    QString res;
    uint loadCounter;
    uint byteCounter;
} Glue;

// -------------------------------------------------------------------------

class WebCrawlerThread : public QThread
{
   Q_OBJECT

private:
    bool abort;
    QMutex mutex;
    Glue *glue;

    void waitLoadUrl(const QUrl url);

    void waitFindFirst(const QString value = "", bool inner = true);
    void waitFindAll(const QString value);

    QString waitGetPlainText();
    QString waitGetInnerXml();
    QString waitGetAttribute(QString value);

    QString makeUrl(QString url, QString u);

    bool goHref(QHash<QString, QString> *now);
    bool goTags(QHash<QString, QString> *now);

signals:
    void loadUrl(const QUrl url);

    void findFirst(const QString value, bool inner);
    void findAll(const QString value);

    void getPlainText();
    void getInnerXml();
    void getAttribute(QString value);

    void finished(bool ok);

public:
    WebCrawlerThread();
    ~WebCrawlerThread();

    void run();
    void init(Glue *glue);
};

// -------------------------------------------------------------------------

class WebCrawler : public QObject
{
     Q_OBJECT

private:
     QWebPage *wp;  // QWebPage _have_ to live in the GUI thread due to
                    // usage of QPixmap. Any access (method calls) to
                    // QWebPage objects needs to happen in the GUI thread
                    // as well!
     Glue *glue;
     WebCrawlerThread *wct;

public:
    WebCrawler();
     ~WebCrawler();

     bool go(const QString filename);

     QString getUrl() { return glue->website.start; }
     uint getLoadCounrer() { return glue->loadCounter; }
     uint getByteCounter() { return glue->byteCounter; }

signals:
     void done(WebCrawler *wc, bool ok);

private slots:
     void loadUrl(const QUrl url);
     void loadFinished(bool ok);

     void findFirst(const QString value, bool inner);
     void findAll(const QString value);

     void getPlainText();
     void getInnerXml();
     void getAttribute(QString value);

     void finished(bool ok);
};

#endif // CRAWLER_H
