/* ----------------------------------------------------------------------------
 * Web Crawler
 *
 * Novembr, 2010, /benner (jens@bennerhq.com)
 */

#include <QUrl>
#include <QFile>
#include <QXmlStreamWriter>
#include <QIODevice>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "crawler.h"
#include "main.h"

QString shopPath = "/Users/jbenner/Programming/crawler/shops/";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    crawler(NULL)
{
    ui->setupUi(this);
    ui->filename->setText("www.superbest.dk.xml");
    ui->verbose->setText("9");

    crawler = new WebCrawler();
}

MainWindow::~MainWindow()
{
    if (crawler) delete crawler;
    delete ui;
}

void MainWindow::on_go_clicked()
{
    verbose = ui->verbose->text().toInt();
    crawler->go(shopPath + ui->filename->text());
}
