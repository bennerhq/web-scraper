/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#include <QtGui/QApplication>
#include <QMutex>
#include <QProcess>
#include <QDebug>

#include "main.h"
#include "mainwindow.h"
#include "crawler.h"
#include "starter.h"

// -------------------------------------------------------------------------

QString outputPath = "";
bool timestamp = false;
bool rawxml = false;
int verbose = 0;
QMutex __mutex;

void usage()
{
    printf("usage:\n");
    printf("    crawler [options] <file name>\n");
    printf("\n");
    printf("options:\n");
    printf("    -help        this text!\n");
    printf("    -o <path>    output path, default is ./\n");
    printf("    -s           silent mode\n");
    printf("    -t           time stamp all verbose info\n");
    printf("    -vall        verbose all!\n");
    printf("    -raw         xml file contain the product dump\n");
    printf("    -v[0-9]      verbose set to level #, -v0 eq -s, -v9 eq -all\n");
    printf("\n");
}

// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Starter starter;

    if (argc>1) {
        int i = 1;

        for (; i < argc; i++) {
            QString cmd = argv[i];

            if (cmd == "-o" && i + 1 < argc) {
                outputPath = argv[++ i];
                if (!outputPath.endsWith("/")) outputPath += "/";
            }
            else if (cmd == "-s") {
                verbose = 0;
            }
            else if (cmd == "-t") {
                timestamp = true;
            }
            else if (cmd == "-vall") {
                verbose = 0xFF;
            }
            else if (cmd == "-raw") {
                rawxml = true;
            }
            else if (cmd.startsWith("-v") && cmd.length() == 3) {
                verbose = cmd.at(2).toAscii() - '0';
                if (verbose < 0 || verbose > 9) {
                    fprintf(stderr, "invalid verbose level\n\n");
                    usage();
                    return 0;
                }
            }
            else if (cmd == "-help") {
                usage();
                return 0;
            }
            else {
                break;
            }
        }

        if (i == argc) {
            fprintf(stderr, "file name missing\n\n");
            usage();
            return -1;
        }

        for (; i < argc; i ++) {
            if (!starter.run(argv[i])) {
                fprintf(stderr, "can't open %s\n", argv[i]);
                return -1;
            }
        }
        if (verbose) {
        }
    }
    else {
        MainWindow *w = new MainWindow();
        w->show();
    }

    return a.exec();
}
