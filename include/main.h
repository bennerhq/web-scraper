/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#ifndef MAIN_H
#define MAIN_H

#include <QString>
#include <QMutex>
#include <QTextStream>
#include <QTime>

extern QString outputPath;

extern bool timestamp;
extern int verbose;
extern bool rawxml;
extern QMutex __mutex;

#define LOG(vb, id, txt)    if (vb < verbose) {                                                 \
                                __mutex.lock();                                                 \
                                QTextStream __out(stdout);                                      \
                                if (timestamp) {                                                \
                                   __out << QTime::currentTime().toString("hh:mm:ss") << " : "; \
                                }                                                               \
                                __out << "["<<id<<"] " << txt << "\n";                          \
                                __mutex.unlock();                                               \
                            }

#define LOG1(str)      LOG(0, "INFO:1  ", str)
#define LOG2(str)      LOG(1, "INFO:2  ", str)
#define LOG3(str)      LOG(2, "INFO:3  ", str)
#define LOG4(str)      LOG(3, "INFO:4  ", str)
#define LOG5(str)      LOG(4, "INFO:5  ", str)
#define LOGALL(str)    LOG(9, "INFO:9  ", str)

#define DEBUG1(str)    LOG(0, "DEBUG:1 ", str)
#define DEBUG2(str)    LOG(1, "DEBUG:2 ", str)
#define DEBUG3(str)    LOG(2, "DEBUG:3 ", str)
#define DEBUG4(str)    LOG(3, "DEBUG:4 ", str)
#define DEBUG5(str)    LOG(4, "DEBUG:5 ", str)
#define DEBUGALL(str)  LOG(9, "DEBUG:9 ", str)

#endif // MAIN_H
