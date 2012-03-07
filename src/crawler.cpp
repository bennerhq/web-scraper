/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */
#include <QApplication>
#include <QThread>
#include <QSet>
#include <QFile>
#include <QXmlStreamWriter>
#include <QIODevice>
#include <QTextStream>
#include <QDateTime>
#include <QNetworkProxy>
#include <QTime>

#include "main.h"
#include "crawler.h"
#include "website.h"

// ----------------------------------------------------------------------------

WebCrawlerThread::WebCrawlerThread()
    : glue(NULL)
{
}

WebCrawlerThread::~WebCrawlerThread()
{
    abort = true;

    if (glue) {
        glue->wait.wakeAll();
        while (!mutex.tryLock()) { sleep(1); }
        mutex.unlock();

        //delete glue;
    }
}

void WebCrawlerThread::init(Glue *glue)
{
    Q_ASSERT(glue);

    this->glue = glue;
    abort = false;
}

void WebCrawlerThread::run()
{
    QHash<QString, QString> now;
    QString startUrl = glue->website.start;
    if (!glue->website.start.endsWith("/")) {
        startUrl += "/";
    }
    now.insert(startUrl, "");

    LOG1(glue->website.start<<": start - "<<startUrl);

    bool res = goHref(&now);
    if (res) {
        res = goTags(&now);
    }

    LOG1(glue->website.start<<": done!" << (res ? "" : " FAILED!!!"));

    emit finished(res);
}

QString WebCrawlerThread::makeUrl(QString url, QString u)
{
    if (u.contains("://")) {
        // full url specified, leave it!
    }
    else if (u.startsWith("?")) {
        // url relative to ?
        u = url.mid(0, url.lastIndexOf("?")) + u;
    }
    else if (u.startsWith("/")) {
        // url relative to base
        u = glue->website.base + u;
    }
    else {
        // url relative to current url
        u = url.mid(0, url.lastIndexOf("/") + 1) + u;
    }

    return u;
}

bool WebCrawlerThread::goHref(QHash<QString, QString> *now)
{
    for (int k = 0; k < glue->website.values.count(); k ++) {
        bool ending = k + 1 == glue->website.values.count();
        QHash<QString, QString> next;
        Value v = glue->website.values.at(k);

        LOG2(glue->website.start<<"H: "<<now->count() << " links");
        QHashIterator<QString, QString> i(*now);
        while (i.hasNext()) {
            i.next();
            if (v.pauseMsec) {
                LOG3(glue->website.start<<"H: sleep "<< v.pauseMsec);
                msleep(v.pauseMsec);
            }

            waitLoadUrl(i.key());
            if (!glue->ok) continue;
            if (abort) return false;

            for (int j = 0; j < v.find.count(); j++) {
                QString f = v.find.at(j);
                int no = 0;
                bool inserted = false;

                LOG4(glue->website.start<<"H: *** FIND: "<<f);

                if (f.startsWith("+")) {
                    LOG5(glue->website.start<<"H: + "<<i.key());

                    f.remove(0, 1);
                    if (next.value(i.key()) != i.value()) {
                        next.insertMulti(i.key(), i.value());
                        inserted = true;
                    }
                }

                if (f.length() && f.at(0).isDigit()) {
                    int k = 1;
                    while (k < f.length() && f.at(k).isDigit()) k ++;
                    no = f.mid(0, k).toInt();
                    f.remove(0, k);

                    f = f.trimmed();
                }

                QWebElement saveCurr = glue->curr;
                waitFindAll(f);
                if (no < glue->elms.count()) {
                    LOG5(glue->website.start<<"H: FOUND "<<f<<" / "<<no);
                    glue->curr = glue->elms.at(no);
                }
                else {
                    LOG5(glue->website.start<<"H: MISSED "<<f);

                    if (!inserted && next.value(i.key()) != i.value()) {
                        next.insertMulti(i.key(), i.value());
                    }
                    glue->curr = saveCurr;
                    break;
                }
            }

            for (int j=0; j < glue->elms.count(); j++) {
                if (v.href != "") {
                    LOG4(glue->website.start<<"H: HREF: "<<v.href);
                    glue->curr = glue->elms.at(j);
                    waitFindFirst(v.href);
                }
                else {
                    LOG4(glue->website.start<<"H: HREF !!! ");
                    glue->elm = glue->elms.at(j);
                }
                QString url = waitGetAttribute("href");
                if (url != "") {
                    url = makeUrl(i.key(), url);
                    if (url.startsWith(glue->website.start)) {
                        QString cat;
                        if (ending || !v.category) {
                            cat = i.value();
                        }
                        else {
                            cat = waitGetPlainText().replace("\n", " ").trimmed(); // FIXME!!??
                            if (cat == "") {
                                cat = i.value();
                            }
                            else if (i.value() != "") {
                                cat = i.value() + " > " + cat;
                            }
                        }
                        if (!next.contains(url) || next.value(url) != cat) {
                            LOG5(glue->website.start<<"H: = "<<url<< " ("<<cat<<")");
                            next.insertMulti(url, cat);
                        }
                        else {
                            LOG5(glue->website.start<<"H: = "<<url<< " SKIPPING! "<< next.value(url)<<"/"<<cat);
                        }
                    }
                    else {
                        LOG5(glue->website.start<<"H: out of scope "<<url);
                    }
                }
                else {
                    LOG4(glue->website.start<<"H: HREF MISSED ");
                }
            }
        }
        *now = next;
    }

    return true;
}

bool WebCrawlerThread::goTags(QHash<QString, QString> *now)
{
    LOG2(glue->website.start<<"T: reading products: " << now->size());

    // Merge duplicated Url's, but collect the
    // differenet categories. Side effect is that
    // a unique Url is only loaded once.
    QHash< QString, QSet<QString> > prodsCat;
    QHashIterator< QString, QString > j(*now);
    while (j.hasNext()) {
        j.next();
        if (!prodsCat.contains(j.key())) {
            prodsCat.insert(j.key(), QSet<QString>());
        }
        prodsCat[j.key()].insert(j.value());
    }

    // contains all products
    QHash< QString, QMap< QString, QString > > products;

    // struct to remove duplicates products
    QHash< QString, QHash< QString, QString > > rmDups;
    QStringList dupNames = glue->website.dups.split(",");
    for (int j = 0; j < dupNames.length(); j++) {
        rmDups[dupNames[j].trimmed()] = QHash< QString, QString >();
    }

    // load the products!
    LOG2(glue->website.start<<"T: tags "<< now->count());
    QHashIterator< QString, QSet<QString> > prods(prodsCat);
    while (prods.hasNext()) {
        prods.next();

        waitLoadUrl(prods.key());
        if (abort) return false;
        if (!glue->ok) continue;

        QMap< QString, QString > prod;
        prod.insert("productUrl", prods.key());

        QSetIterator< QString > pitr(prods.value());
        while (pitr.hasNext()) {
            QString v = pitr.next();
            if (!v.isEmpty()) {
                prod.insertMulti("category", v);
            }
        }

        QWebElement reset = glue->curr;
        for (int j=0; j < glue->website.tags.count(); j++) {
            Tag tag = glue->website.tags.at(j);
            if (tag.pauseMsec) {
                LOG3(glue->website.start<<"T: sleep "<< tag.pauseMsec);
                msleep(tag.pauseMsec);
            }

            glue->curr = reset;
            foreach (QString f, tag.find) {
                waitFindFirst(f, true);
            }
            if (glue->elm.isNull()) continue;

            QString res = "";
            switch (tag.type) {
            case TAG_UNKNOWN:
            case TAG_COUNT:
                Q_ASSERT(!"tag.type == TAG_UNKNOWN");
                break;

            case TAG_INNER:
                res = waitGetInnerXml();
                break;

            case TAG_PLAIN:
                res = waitGetPlainText();
                break;

            case TAG_ATTRIBUTE:
                res = waitGetAttribute(tag.attribute);
                break;

            case TAG_EXIST:
                res = "yes";
                break;
            }
            res = res.replace("\n", " ").trimmed(); // FIXME??
            if (res.isEmpty()) continue;

            QString tagName = tag.name;
            if (tagName.startsWith("#")) { // digit??
                tagName.remove(0, 1);

                res.replace(".", "");
                res.replace(",", ".");
                res.replace("<sup>", ".");
                res.replace("</sup>", "");

                // find digit sequence
                int i = 0;
                while (i < res.size() && !res.at(i).isDigit()) i ++;
                int j = i;
                while (j < res.size() && (res.at(j).isDigit() || res.at(j) == '.')) j ++;

                if (i == j) {
                    res == "";
                }
                else {
                    res = res.mid(i, j - i);
                    if (res.endsWith(".")) {
                        res += "00";
                    }
                }
            }
            else if (tagName.startsWith("/")) { // URL
                tagName.remove(0, 1);
                res = makeUrl(prods.key(), res);
            }
            else if (tagName.at(0).isDigit()) { // text cutting
                int k = 1;
                while (k < tagName.length() && tagName.at(k).isDigit()) k ++;
                int no = tagName.mid(0, k).toInt();
                tagName = tagName.remove(0, k+1).trimmed();

                res.remove(0, no);
            }

            tagName = tagName.trimmed();
            res = res.trimmed();
            if (!tagName.isEmpty() && !res.isEmpty()) {
                LOG4(glue->website.start<<"T: tag "<<tagName<<"="<<res);
                prod.insert(tagName, res);
            }
        }

        if (rawxml) {
            // Dump the raw product fetched from the web page, i.e.,
            // no merge of duplicates and no chech for must have tags.
            // this is usefull for debugging!
            products.insertMulti(prods.key(), prod);
        }
        else {
            // Merge duplicated products, one product can have
            // several id's.
            //
            // default is "name"
            QHashIterator< QString, QHash< QString, QString > > rmItr(rmDups);
            while (rmItr.hasNext()) {
                rmItr.next();
                QString id = prod[rmItr.key()];
                if (id.isEmpty()) continue;

                QHash< QString, QString > elms = rmItr.value();
                if (elms.contains(id)) {
                    LOG3(glue->website.start<<"T: merging: "<<rmItr.key()<<"="<<id<<" ("<<elms[id]<<")");

                    QMapIterator< QString, QString > mitr(products[elms[id]]);
                    while (mitr.hasNext()) {
                        mitr.next();
                        QString mid = mitr.key();
                        QString mvalue = mitr.value();

                        if (prod[mid] != mvalue) {
                            LOG4(glue->website.start<<"T: tag merge "<<mid<<"="<<mvalue);
                            prod.insertMulti(mid, mvalue);
                        }
                    }
                    products.remove(elms[id]);
                }
                rmDups[rmItr.key()][id] = prods.key();
            }

            // Ensure that "must tags" are present in this
            // product!
            //
            // default is "name" and "price"
            bool prodValid = true;
            QStringList mTags = glue->website.mustTag.split(",");
            for (int j = 0; j < mTags.length(); j++) {
                prodValid = prod.contains(mTags[j].trimmed());
                if (!prodValid) break;
            }
            if (prodValid) {
                products.insert(prods.key(), prod);
            }
        }
    }

    //  write xml formatted file
    QString n = glue->website.start;
    int p = n.indexOf("://");
    if (p != -1) n.remove(0, p + 3);
    p = n.indexOf("/");
    if (p != -1) n.remove(p, n.length());

    QString filename = glue->website.output;
    if (filename == "") filename = "%n-%d.xml";
    filename.replace("%n", n);
    filename.replace("%d", QDateTime::currentDateTime().toString("yyyy-MM-dd--hh-mm-ss"));

    QFile output(outputPath + filename);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning()<<"can't create "<<filename;
        return false;
    }
    LOG2(glue->website.start<<"T: writing: " << outputPath + filename << " / " << products.size());
    QXmlStreamWriter stream(&output);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("webshop");
    stream.writeAttribute("href", glue->website.start);
    stream.writeStartElement("creator");
    stream.writeTextElement("created", QDateTime::currentDateTime().toString());
    glue->website.dumpXmlStream(&stream);
    stream.writeEndElement(); // creator

    stream.writeStartElement("products");

    QHashIterator< QString, QMap<QString, QString> > itr(products);
    while (itr.hasNext()) {
        itr.next();

        stream.writeStartElement("product");

        QMapIterator<QString, QString> elms(itr.value());
        while (elms.hasNext()) {
            elms.next();
            QString tag = elms.key().trimmed();
            QString value = elms.value().trimmed();
            if (tag != "" && value !="") {      // FIXME!!!
                stream.writeTextElement(tag, value);
            }
        }

        stream.writeEndElement(); // <product>
    }

    stream.writeEndElement(); // <products>
    stream.writeEndElement(); // <webshop>
    stream.writeEndDocument();
    output.close();

    return true;
}

void WebCrawlerThread::waitLoadUrl(const QUrl url)
{
    LOG3(glue->website.start<<": load - "<<url.toString());

    mutex.lock();
    emit loadUrl(url);
    glue->wait.wait(&mutex); // block until thread releases mutex
    mutex.unlock();
}

void WebCrawlerThread::waitFindFirst(const QString value, bool inner)
{
    emit findFirst(value, inner); // block until listener returns

    LOG4(glue->website.start<<": first - "<<value << (glue->elm.isNull() ? " / FAILED!! " : ""));
}

void WebCrawlerThread::waitFindAll(const QString value)
{
    emit findAll(value); // block until listener returns

    LOG4(glue->website.start<<": all - "<<glue->elms.count()<<" / "<<value);
}

QString WebCrawlerThread::waitGetPlainText()
{
    emit getPlainText(); // block until listener returns

    LOG5(glue->website.start<<": plainText - "<<glue->res);
    return glue->res;
}

QString WebCrawlerThread::waitGetInnerXml()
{
    emit getInnerXml(); // block until listener returns

    LOG5(glue->website.start<<": innerXml - "<<glue->res);
    return glue->res;
}

QString WebCrawlerThread::waitGetAttribute(QString value)
{
    emit getAttribute(value); // block until listener returns

    LOG5(glue->website.start<<": attribute - "<<glue->res);
    return glue->res;
}

// ----------------------------------------------------------------------------

WebCrawler::WebCrawler()
    : wp(NULL), glue(NULL), wct(NULL)
{
    wct = new WebCrawlerThread();
    connect(wct, SIGNAL(loadUrl(QUrl)), this, SLOT(loadUrl(QUrl)), Qt::QueuedConnection);

    connect(wct, SIGNAL(findFirst(QString, bool)), this, SLOT(findFirst(QString, bool)), Qt::BlockingQueuedConnection);
    connect(wct, SIGNAL(findAll(QString)), this, SLOT(findAll(QString)), Qt::BlockingQueuedConnection);
    connect(wct, SIGNAL(getPlainText()), this, SLOT(getPlainText()), Qt::BlockingQueuedConnection);
    connect(wct, SIGNAL(getInnerXml()), this, SLOT(getInnerXml()), Qt::BlockingQueuedConnection);
    connect(wct, SIGNAL(getAttribute(QString)), this, SLOT(getAttribute(QString)), Qt::BlockingQueuedConnection);

    connect(wct, SIGNAL(finished(bool)), this, SLOT(finished(bool)));

    wp = new QWebPage();
    connect(wp, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    wp->settings()->setAttribute(QWebSettings::AutoLoadImages, false); // faster loading!
    wp->settings()->setAttribute(QWebSettings::PluginsEnabled, false); // faster loading!
}

WebCrawler::~WebCrawler()
{
    if (wp) delete wp;
    if (wct) delete wct;
}

bool WebCrawler::go(const QString filename)
{
    Q_ASSERT(glue == NULL);

    glue = new Glue();
    bool res = glue->website.load(filename);
    if (res) {
        glue->ok = false;
        glue->loadCounter = 0;
        glue->byteCounter = 0;

        wct->init(glue); // wct will deallocate glue!
        wct->start();
    }
    else {
        delete glue;
    }

    return res;
}

void WebCrawler::loadUrl(const QUrl url)
{
    glue->url = url;
    glue->ok = false;
    wp->mainFrame()->load(glue->url);
}

void WebCrawler::loadFinished(bool ok)
{
    if (!ok) {
        qWarning()<<"can't load "<<glue->url.toString();
    }
    glue->ok = ok;
    glue->loadCounter ++;
    glue->byteCounter += wp->totalBytes();
    glue->elm = glue->curr = wp->mainFrame()->documentElement();
    glue->wait.wakeAll();
}

void WebCrawler::findFirst(const QString value, bool inner)
{
    if (value == "") {
        glue->elm = glue->curr = wp->mainFrame()->documentElement();
    }
    else {
        glue->elm = glue->curr.findFirst(value);
        if (inner) {
            glue->curr = glue->elm;
        }
    }
}

void WebCrawler::findAll(const QString value)
{
    glue->elms = glue->curr.findAll(value);
}

void WebCrawler::getPlainText()
{
    glue->res = glue->elm.toPlainText();
    if (glue->res.isEmpty()) {
        glue->res = "";
    }
    else {
        glue->res = glue->res.trimmed();
    }
}

void WebCrawler::getInnerXml()
{
    glue->res = glue->elm.toInnerXml();
    if (glue->res.isEmpty()) {
        glue->res = "";
    }
    else {
        glue->res = glue->res.trimmed();
    }
}

void WebCrawler::getAttribute(QString value)
{
    glue->res = glue->elm.attribute(value, "");
    if (glue->res.isEmpty()) {
        glue->res = "";
    }
    else {
        glue->res = glue->res.trimmed();
    }
}

void WebCrawler::finished(bool ok)
{
    emit done(this, ok);
}
