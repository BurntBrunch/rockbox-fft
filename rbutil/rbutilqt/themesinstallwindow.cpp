/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>

#include "ui_themesinstallfrm.h"
#include "themesinstallwindow.h"
#include "zipinstaller.h"
#include "progressloggergui.h"
#include "utils.h"
#include "rbsettings.h"
#include "systeminfo.h"

ThemesInstallWindow::ThemesInstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.listThemes->setAlternatingRowColors(true);
    ui.listThemes->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.listThemes->setSortingEnabled(true);
    ui.themePreview->clear();
    ui.themePreview->setText(tr("no theme selected"));
    ui.labelSize->setText(tr("no selection"));
    ui.listThemes->setLayoutDirection(Qt::LeftToRight);
    ui.themeDescription->setLayoutDirection(Qt::LeftToRight);

    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.listThemes, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this, SLOT(updateDetails(QListWidgetItem*, QListWidgetItem*)));
    connect(ui.listThemes, SIGNAL(itemSelectionChanged()), this, SLOT(updateSize()));
    connect(&igetter, SIGNAL(done(bool)), this, SLOT(updateImage(bool)));
}

ThemesInstallWindow::~ThemesInstallWindow()
{
    if(infocachedir!="")
        recRmdir(infocachedir);
}


void ThemesInstallWindow::downloadInfo()
{
    // try to get the current build information
    getter = new HttpGet(this);

    themesInfo.open();
    qDebug() << "[Themes] downloading info to" << themesInfo.fileName();
    themesInfo.close();

    QUrl url;
    url = QUrl(SystemInfo::value(SystemInfo::ThemesUrl).toString()
                + "/rbutilqt.php?target="
                + SystemInfo::value(SystemInfo::CurConfigureModel).toString());
    qDebug() << "[Themes] Info URL:" << url << "Query:" << url.queryItems();
    if(RbSettings::value(RbSettings::CacheOffline).toBool())
        getter->setCache(true);
    getter->setFile(&themesInfo);

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(logger, SIGNAL(aborted()), getter, SLOT(abort()));
    getter->getFile(url);
}


void ThemesInstallWindow::downloadDone(int id, bool error)
{
    downloadDone(error);
    qDebug() << "[Themes] Download" << id << "done, error:" << error;
}


void ThemesInstallWindow::downloadDone(bool error)
{
    qDebug() << "[Themes] Download done, error:" << error;

    disconnect(logger, SIGNAL(aborted()), getter, SLOT(abort()));
    disconnect(logger, SIGNAL(aborted()), this, SLOT(close()));
    themesInfo.open();

    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    QStringList tl = iniDetails.childGroups();
    qDebug() << "[Themes] Theme site result:"
             << iniDetails.value("error/code").toString()
             << iniDetails.value("error/description").toString()
             << iniDetails.value("error/query").toString();

    if(error) {
        logger->addItem(tr("Network error: %1.\n"
                "Please check your network and proxy settings.")
                .arg(getter->errorString()), LOGERROR);
        getter->abort();
        logger->setFinished();
        disconnect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(logger, SIGNAL(closed()), this, SLOT(close()));
        return;
    }
    // handle possible error codes
    if(iniDetails.value("error/code").toInt() != 0 || !iniDetails.contains("error/code")) {
        qDebug() << "[Themes] Theme site returned an error:"
                 << iniDetails.value("error/code");
        logger->addItem(tr("the following error occured:\n%1")
            .arg(iniDetails.value("error/description", "unknown error").toString()), LOGERROR);
        logger->setFinished();
        connect(logger, SIGNAL(closed()), this, SLOT(close()));
        return;
    }
    logger->addItem(tr("done."), LOGOK);
    logger->setFinished();
    logger->close();

    // setup list
    for(int i = 0; i < tl.size(); i++) {
        iniDetails.beginGroup(tl.at(i));
        // skip all themes without name field set (i.e. error section)
        if(iniDetails.value("name").toString().isEmpty()) {
            iniDetails.endGroup();
            continue;
        }
        qDebug() << "[Themes] adding to list:" << tl.at(i);
        // convert to unicode and replace HTML-specific entities
        QByteArray raw = iniDetails.value("name").toByteArray();
        QTextCodec* codec = QTextCodec::codecForHtml(raw);
        QString name = codec->toUnicode(raw);
        name.replace("&quot;", "\"").replace("&amp;", "&");
        name.replace("&lt;", "<").replace("&gt;", ">");
        QListWidgetItem *w = new QListWidgetItem;
        w->setData(Qt::DisplayRole, name.trimmed());
        w->setData(Qt::UserRole, tl.at(i));
        ui.listThemes->addItem(w);

        iniDetails.endGroup();
    }
    // check if there's a themes "MOTD" available
    if(iniDetails.contains("status/msg")) {
        // check if there's a localized msg available
        QString lang = RbSettings::value(RbSettings::Language).toString().split("_").at(0);
        QString msg;
        if(iniDetails.contains("status/msg." + lang))
            msg = iniDetails.value("status/msg." + lang).toString();
        else
            msg = iniDetails.value("status/msg").toString();
        qDebug() << "[Themes] MOTD" << msg;
        if(!msg.isEmpty())
            QMessageBox::information(this, tr("Information"), msg);
    }
}


void ThemesInstallWindow::updateSize(void)
{
    long size = 0;
    // sum up size for all selected themes
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    int items = ui.listThemes->selectedItems().size();
    for(int i = 0; i < items; i++) {
        iniDetails.beginGroup(ui.listThemes->selectedItems()
                              .at(i)->data(Qt::UserRole).toString());
        size += iniDetails.value("size").toInt();
        iniDetails.endGroup();
    }
    ui.labelSize->setText(tr("Download size %L1 kiB (%n item(s))", "", items)
                             .arg((size + 512) / 1024));
}


void ThemesInstallWindow::updateDetails(QListWidgetItem* cur, QListWidgetItem* prev)
{
    if(cur == prev)
        return;

    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);

    QCoreApplication::processEvents();
    ui.themeDescription->setText(tr("fetching details for %1")
        .arg(cur->data(Qt::DisplayRole).toString()));
    ui.themePreview->clear();
    ui.themePreview->setText(tr("fetching preview ..."));
    imgData.clear();

    iniDetails.beginGroup(cur->data(Qt::UserRole).toString());

    QUrl img, txt;
    txt = QUrl(QString(SystemInfo::value(SystemInfo::ThemesUrl).toString() + "/"
        + iniDetails.value("descriptionfile").toString()));
    img = QUrl(QString(SystemInfo::value(SystemInfo::ThemesUrl).toString() + "/"
        + iniDetails.value("image").toString()));

    QString text;
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    text = tr("<b>Author:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("author", tr("unknown")).toByteArray()));
    text += tr("<b>Version:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("version", tr("unknown")).toByteArray()));
    text += tr("<b>Description:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("about", tr("no description")).toByteArray()));

    text.trimmed();
    text.replace("\n", "<br/>");
    ui.themeDescription->setHtml(text);
    iniDetails.endGroup();

    igetter.abort();
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        igetter.setCache(true);
    else
    {
        if(infocachedir=="")
        {
            infocachedir = QDir::tempPath() + "rbutil-themeinfo";
            QDir d = QDir::temp();
            d.mkdir("rbutil-themeinfo");
        }
        igetter.setCache(infocachedir);
    }

    igetter.getFile(img);
}


void ThemesInstallWindow::updateImage(bool error)
{
    qDebug() << "[Themes] Updating image:"<< !error;

    if(error) {
        ui.themePreview->clear();
        ui.themePreview->setText(tr("Retrieving theme preview failed.\n"
            "HTTP response code: %1").arg(igetter.httpResponse()));
        return;
    }

    QPixmap p;
    if(!error) {
        imgData = igetter.readAll();
        if(imgData.isNull()) return;
        p.loadFromData(imgData);
        if(p.isNull()) {
            ui.themePreview->clear();
            ui.themePreview->setText(tr("no theme preview"));
        }
        else
            ui.themePreview->setPixmap(p);
    }
}


void ThemesInstallWindow::resizeEvent(QResizeEvent* e)
{
    qDebug() << "[Themes]" << e;

    QPixmap p, q;
    QSize img;
    img.setHeight(ui.themePreview->height());
    img.setWidth(ui.themePreview->width());

    p.loadFromData(imgData);
    if(p.isNull()) return;
    q = p.scaled(img, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui.themePreview->setScaledContents(false);
    ui.themePreview->setPixmap(p);
}



void ThemesInstallWindow::show()
{
    QDialog::show();
    logger = new ProgressLoggerGui(this);
    logger->show();
    logger->addItem(tr("getting themes information ..."), LOGINFO);

    connect(logger, SIGNAL(aborted()), this, SLOT(close()));

    downloadInfo();

}


void ThemesInstallWindow::abort()
{
    igetter.abort();
    logger->setFinished();
    this->close();
}


void ThemesInstallWindow::accept()
{
    if(ui.listThemes->selectedItems().size() == 0) {
        this->close();
        return;
    }
    QStringList themes;
    QStringList names;
    QStringList version;
    QString zip;
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    for(int i = 0; i < ui.listThemes->selectedItems().size(); i++) {
        iniDetails.beginGroup(ui.listThemes->selectedItems().at(i)->data(Qt::UserRole).toString());
        zip = SystemInfo::value(SystemInfo::ThemesUrl).toString()
                + "/" + iniDetails.value("archive").toString();
        themes.append(zip);
        names.append("Theme: " +
                ui.listThemes->selectedItems().at(i)->data(Qt::DisplayRole).toString());
        // if no version info is available use installation (current) date
        version.append(iniDetails.value("version",
                QDate().currentDate().toString("yyyyMMdd")).toString());
        iniDetails.endGroup();
    }
    qDebug() << "[Themes] installing:" << themes;

    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = RbSettings::value(RbSettings::Mountpoint).toString();
    qDebug() << "[Themes] mountpoint:" << mountPoint;
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return;
    }

    installer = new ZipInstaller(this);
    installer->setUrl(themes);
    installer->setLogSection(names);
    installer->setLogVersion(version);
    installer->setMountPoint(mountPoint);
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);

    connect(logger, SIGNAL(closed()), this, SLOT(close()));
    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();

}

