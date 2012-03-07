/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "main.h"
#include "website.h"

QString TagNames[TAG_COUNT] = {
    "unknown",
    "inner",
    "plain",
    "attribute",
    "exist",
};

TagTypes TagName(const QString txt)
{
    for (int j=0; j < TAG_COUNT; j++) {
        if (TagNames[j] == txt) {
            return (TagTypes) j;
        }
    }
    return TAG_UNKNOWN;
}
bool Website::load(const QString filename, bool clear)
{
    Value value;
    Tag tag;

    if (clear) {
        start = "";
        base = "";
        output = "";
        include = "";
        dups = "name";
        mustTag = "price, name";
    }

    values.clear();
    value.find.clear();
    value.href = "";
    value.pauseMsec = 0;
    value.category = true;

    tags.clear();
    tag.type = TAG_UNKNOWN;
    tag.find.clear();
    tag.name = "";
    tag.attribute = "";
    tag.pauseMsec = 0;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream in(&file);


    LOG2("loading config file " << filename);
    bool started = true;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QString cmd;

        // ---( comments )---
        if (line == "" || line.startsWith("#")) {
            continue;
        }

        // ---( section control )---
        if (line == "section.webpage") {
            started = true;
            continue;
        }
        if (line == "section.category") {
            started = false;
            continue;
        }
        if (!started) {
            continue;
        }

        // ---( fetch command )---
        int p = line.indexOf("=");
        if (p == -1) continue;
        cmd = line.mid(0, p).trimmed();
        line = line.remove(0, p + 1).trimmed();

        // ---( include )---
        if (cmd == "include") {
            include = line;
            if (!load(filename.mid(0, filename.lastIndexOf("/") + 1) + include, false)) {
                load(include, false);
            }
            continue;
        }

        // ---( URL )---
        if (cmd == "shopName") {
            shopName = line;
            continue;
        }
        if (cmd == "url.start") {
            start = line;
            continue;
        }
        if (cmd == "url.base") {
            base = line;
            if (base.endsWith("/")) // no pending "/"
                base.remove(base.length() - 1, 1);
            continue;
        }
        if (cmd == "path.output") {
            output = line;
            continue;
        }

        // ---( Misc )---
        if (cmd == "duplicates") {
            dups = line;
            continue;
        }
        if (cmd == "mustTag") {
            mustTag = line;
            continue;
        }

        // ---( HREF )---
        if (cmd == "value.find") {
            if (value.find.count()) {
                values += value;
            }
            value.find.clear();
            value.find += line;
            value.href = "";
            value.pauseMsec = 0;
            value.category = true;
            continue;
        }
        if (cmd == "value.find+") {
            value.find += line;
            continue;
        }
        if (cmd == "value.href") {
            value.href = line;
            continue;
        }
        if (cmd == "value.pause") {
            value.pauseMsec = line.toInt();
            continue;
        }
        if (cmd == "value.category") {
            value.category = line == "yes";
            continue;
        }

        // ---( TAGS )---
        if (cmd == "tag.find") {
            if (tag.type != TAG_UNKNOWN && tag.name != "") {
                tags += tag;
            }

            tag.find.clear();
            tag.find += line;
            tag.type = TAG_INNER;
            tag.attribute = "";
            tag.name = "";
            tag.pauseMsec = 0;
            continue;
        }
        if (cmd == "tag.find+") {
            tag.find += line;
            continue;
        }
        if (cmd == "tag.type") {
            tag.type = TagName(line);
            continue;
        }
        if (cmd == "tag.attribute") {
            tag.attribute = line;
            continue;
        }
        if (cmd == "tag.name") {
            tag.name = line;
            continue;
        }
        if (cmd == "tag.pause") {
            tag.pauseMsec = line.toInt();
            continue;
        }
        qWarning()<<"unknown command: "<<cmd;
        return false;
    }
    if (value.find.count()) {
        values += value;
    }
    if (tag.type != TAG_UNKNOWN && tag.name != "") {
        tags += tag;
    }

    return true;
}

void Website::dumpXmlStream(QXmlStreamWriter *stream)
{
    if (shopName != "")
        stream->writeTextElement("shopName", shopName);
    if (start != "")
        stream->writeTextElement("startUrl", start);
    if (base != "")
        stream->writeTextElement("baseUrl", base);
    if (output != "")
        stream->writeTextElement("output", output);
    if (include != "")
        stream->writeTextElement("include", include);
    if (dups != "")
        stream->writeTextElement("duplicates", dups);
    if (mustTag != "")
        stream->writeTextElement("mustTag", mustTag);

    foreach (Value v, values) {
        stream->writeStartElement("value");
        if (v.find.count()) {
            stream->writeTextElement("find", v.find.at(0));

            for (int j=1; j < v.find.count(); j++) {
                stream->writeTextElement("findplus", v.find.at(j));
            }
        }
        if (v.href != "")
            stream->writeTextElement("href", v.href);
        if (v.pauseMsec)
            stream->writeTextElement("pause", QString::number(v.pauseMsec));
        if (!v.category)
            stream->writeTextElement("category", (v.category ? "yes" : "no"));
        stream->writeEndElement();
    }

    foreach (Tag t, tags) {
        stream->writeStartElement("tag");
        stream->writeTextElement("type", TagNames[t.type]);
        if (t.find.count()) {
            stream->writeTextElement("find", t.find.at(0));

            for (int j=1; j < t.find.count(); j++) {
                stream->writeTextElement("findplus", t.find.at(j));
            }
        }
        if (t.attribute != "")
            stream->writeTextElement("attribute", t.attribute);
        if (t.name != "")
            stream->writeTextElement("name", t.name);
        if (t.pauseMsec)
            stream->writeTextElement("pause", QString::number(t.pauseMsec));
        stream->writeEndElement();
    }
}

void Website::dumpXml(QIODevice *dev)
{
    QTextStream tout(stdout);
    QXmlStreamWriter stream((dev != NULL ? dev : tout.device()));

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    dumpXmlStream(&stream);

    stream.writeEndDocument();
}

void Website::dumpUnix(QIODevice *dev)
{
    QTextStream out(stdout);
    if (dev != NULL) {
        out.setDevice(dev);
    }

    out<<"# -----------------------------------------------------------"<<"\n";
    out<<"# <css selector>: standard CSS2 selector syntax is used for"<<"\n";
    out<<"# the query. Detail description of usage here;"<<"\n";
    out<<"# http://www.w3.org/TR/CSS2/selector.html#q1"<<"\n";
    out<<"#"<<"\n";
    out<<"\n";
    out<<"# -----------------------------------------------------------"<<"\n";
    out<<"# House keeping commands"<<"\n";
    out<<"#"<<"\n";
    out<<"# shopName=<text>"<<"\n";
    out<<"#     name of the web shop"<<"\n";
    out<<"#"<<"\n";
    out<<"# url.start=<url>"<<"\n";
    out<<"#     start url"<<"\n";
    out<<"#"<<"\n";
    out<<"# url.base=<url>"<<"\n";
    out<<"#     if href references started with \"/\", this url will be"<<"\n";
    out<<"#     added in front of the given url"<<"\n";
    out<<"#"<<"\n";
    out<<"# duplicates=<tag name>, [<tag name>]"<<"\n";
    out<<"#     unique product identifyer(s). product with these ids will"<<"\n";
    out<<"#     be merged first will be used as id"<<"\n";
    out<<"#"<<"\n";
    out<<"#     default: productUrl, name"<<"\n";
    out<<"#"<<"\n";
    out<<"# path.output=<file name>"<<"\n";
    out<<"#     default is \"./%n-%d.xml\" where %n is filename genereted"<<"\n";
    out<<"#     from web page address and %d is current date and time"<<"\n";
    out<<"#"<<"\n";
    out<<"# mustTag=<tag name>,[<tag name>]"<<"\n";
    out<<"#     tags that must be present in each product"<<"\n";
    out<<"#"<<"\n";
    out<<"#     default: price, name"<<"\n";
    out<<"#"<<"\n";
    out<<"# include=<file name>"<<"\n";
    out<<"#     nested include file name"<<"\n";
    out<<"#"<<"\n";
    out<<"\n";

    out<<"shopName="<<shopName<<"\n";
    out<<"url.start="<<start<<"\n";
    out<<"url.base="<<base<<"\n";
    if (output != "")
        out<<"path.output="<<output<<"\n";
    if (include != "")
        out<<"path.include="<<include<<"\n";
    if (dups != "")
        out<<"duplicates="<<dups<<"\n";
    out<<"\n";

    out<<"# ---( HREF )--------------------------------------------"<<"\n";
    out<<"#"<<"\n";
    out<<"# value.find[+]=[+][#]<css selector>"<<"\n";
    out<<"# value.href=<attribute>"<<"\n";
    out<<"#"<<"\n";
    out<<"\n";
    foreach (Value v, values) {
        if (v.find.count()) {
            out<<"value.find="<<v.find.at(0)<<"\n";
            for (int j=1; j < v.find.count(); j++) {
                out<<"value.find+="<<v.find.at(j)<<"\n";
            }
        }
        if (v.href != "")
            out<<"value.href="<<v.href<<"\n";
        if (v.pauseMsec)
            out<<"value.pause="<<v.pauseMsec<<"\n";
        if (!v.category)
            out<<"value.category="<<(v.category ? "yes" : "no");
        out<<"\n";
    }

    out<<"# ---( TAGS )--------------------------------------------"<<"\n";
    out<<"#"<<"\n";
    out<<"# tag.type={ inner | plain | attribute | next | reset }"<<"\n";
    out<<"# tag.find[+]=<css selector>"<<"\n";
    out<<"# tag.attribute=<attribute>"<<"\n";
    out<<"# tag.name=[id]"<<"\n";
    out<<"#"<<"\n";
    out<<"\n";
    foreach (Tag t, tags) {
        if (t.find.count()) {
            out<<"tag.find="<<t.find.at(0)<<"\n";
            for (int j=1; j < t.find.count(); j++) {
                out<<"tag.find+="<<t.find.at(j)<<"\n";
            }
        }
        out<<"tag.type="<<TagNames[t.type]<<"\n";
        if (t.attribute != "")
            out<<"tag.attribute="<<t.attribute<<"\n";
        if (t.name != "")
            out<<"tag.name="<<t.name<<"\n";
        if (t.pauseMsec)
            out<<"tag.pause="<<t.pauseMsec<<"\n";
        out<<"\n";
    }
}
