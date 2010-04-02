/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include <QtDebug>
#include <QtDebug>
#include "bootloaderinstallmi4.h"
#include "utils.h"

BootloaderInstallMi4::BootloaderInstallMi4(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstallMi4::install(void)
{
    emit logItem(tr("Downloading bootloader"), LOGINFO);
    qDebug() << "[BootloaderInstallMi4] installing bootloader";
    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}

void BootloaderInstallMi4::installStage2(void)
{
    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);
    QCoreApplication::processEvents();

    // move old bootloader out of the way
    QString fwfile(Utils::resolvePathCase(m_blfile));
    QFile oldbl(fwfile);
    QString moved = QFileInfo(Utils::resolvePathCase(m_blfile)).absolutePath()
                        + "/OF.mi4";
    if(!QFileInfo(moved).exists()) {
        qDebug() << "[BootloaderInstallMi4] renaming" << fwfile << "to" << moved;
        oldbl.rename(moved);
    }
    else {
        qDebug() << "[BootloaderInstallMi4] OF.mi4 already present, not renaming again.";
        oldbl.remove();
    }

    // place new bootloader
    m_tempfile.open();
    qDebug() << "[BootloaderInstallMi4] renaming" << m_tempfile.fileName() << "to" << fwfile;
    m_tempfile.close();
    m_tempfile.copy(fwfile);

    emit logItem(tr("Bootloader successful installed"), LOGOK);
    logInstall(LogAdd);

    emit done(false);
}


bool BootloaderInstallMi4::uninstall(void)
{
    qDebug() << "[BootloaderInstallMi4] Uninstalling bootloader";

    // check if it's actually a Rockbox bootloader
    emit logItem(tr("Checking for Rockbox bootloader"), LOGINFO);
    if(installed() != BootloaderRockbox) {
        emit logItem(tr("No Rockbox bootloader found"), LOGERROR);
        return false;
    }

    // check if OF file present
    emit logItem(tr("Checking for original firmware file"), LOGINFO);
    QString original = QFileInfo(Utils::resolvePathCase(m_blfile)).absolutePath()
                        + "/OF.mi4";

    if(Utils::resolvePathCase(original).isEmpty()) {
        emit logItem(tr("Error finding original firmware file"), LOGERROR);
        return false;
    }

    // finally remove RB bootloader
    QString resolved = Utils::resolvePathCase(m_blfile);
    QFile blfile(resolved);
    blfile.remove();

    QFile::rename(Utils::resolvePathCase(original), m_blfile);
    emit logItem(tr("Rockbox bootloader successful removed"), LOGINFO);
    logInstall(LogRemove);
    emit done(false);

    return true;
}


//! check if a bootloader is installed and return its state.
BootloaderInstallBase::BootloaderType BootloaderInstallMi4::installed(void)
{
    // for MI4 files we can check if we actually have a RB bootloader
    // installed.
    // RB bootloader has "RBBL" at 0x1f8 in the mi4 file.

    // make sure to resolve case to prevent case issues
    QString resolved;
    resolved = Utils::resolvePathCase(m_blfile);
    if(resolved.isEmpty()) {
        qDebug() << "[BootloaderInstallMi4] installed: BootloaderNone";
        return BootloaderNone;
    }

    QFile f(resolved);
    f.open(QIODevice::ReadOnly);
    f.seek(0x1f8);
    char magic[4];
    f.read(magic, 4);
    f.close();

    if(!memcmp(magic, "RBBL", 4)) {
        qDebug() << "[BootloaderInstallMi4] installed: BootloaderRockbox";
        return BootloaderRockbox;
    }
    else {
        qDebug() << "[BootloaderInstallMi4] installed: BootloaderOther";
        return BootloaderOther;
    }
}


BootloaderInstallBase::Capabilities BootloaderInstallMi4::capabilities(void)
{
    qDebug() << "[BootloaderInstallMi4] getting capabilities";
    return Install | Uninstall | Backup | IsFile | CanCheckInstalled | CanCheckVersion;
}

