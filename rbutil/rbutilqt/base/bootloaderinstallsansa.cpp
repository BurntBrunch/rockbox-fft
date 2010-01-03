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
#include "bootloaderinstallbase.h"
#include "bootloaderinstallsansa.h"

#include "../sansapatcher/sansapatcher.h"
#include "autodetection.h"

BootloaderInstallSansa::BootloaderInstallSansa(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
    // initialize sector buffer. sansa_sectorbuf is instantiated by
    // sansapatcher.
    // The buffer itself is only present once, so make sure to not allocate
    // it if it was already allocated. The application needs to take care
    // no concurrent (i.e. multiple objects of this class running) requests
    // are done.
    if(sansa_sectorbuf == NULL)
        sansa_alloc_buffer(&sansa_sectorbuf, BUFFER_SIZE);
}


BootloaderInstallSansa::~BootloaderInstallSansa()
{
    if(sansa_sectorbuf) {
        free(sansa_sectorbuf);
        sansa_sectorbuf = NULL;
    }
}


/** Start bootloader installation.
 */
bool BootloaderInstallSansa::install(void)
{
    if(sansa_sectorbuf == NULL) {
        emit logItem(tr("Error: can't allocate buffer memory!"), LOGERROR);
        return false;
        emit done(true);
    }

    emit logItem(tr("Searching for Sansa"), LOGINFO);

    struct sansa_t sansa;

    int n = sansa_scan(&sansa);
    if(n == -1) {
        emit logItem(tr("Permission for disc access denied!\n"
                "This is required to install the bootloader"),
                LOGERROR);
        emit done(true);
        return false;
    }
    if(n == 0) {
        emit logItem(tr("No Sansa detected!"), LOGERROR);
        emit done(true);
        return false;
    }
    if(sansa.hasoldbootloader) {
        emit logItem(tr("OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
               "You must reinstall the original Sansa firmware before running\n"
               "sansapatcher for the first time.\n"
               "See http://www.rockbox.org/wiki/SansaE200Install\n"),
               LOGERROR);
        emit done(true);
        return false;
    }
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}


/** Finish bootloader installation.
 */
void BootloaderInstallSansa::installStage2(void)
{
    struct sansa_t sansa;

    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);
    QCoreApplication::processEvents();
    if(!sansaInitialize(&sansa)) {
            emit done(true);
            return;
    }

    if(sansa_reopen_rw(&sansa) < 0) {
        emit logItem(tr("Could not open Sansa in R/W mode"), LOGERROR);
        emit done(true);
        return;
    }

    // check model -- if sansapatcher reports a c200 don't install an e200
    // bootloader and vice versa.
    // The model is available in the mi4 file at offset 0x1fc and matches
    // the targetname set by sansapatcher.
    emit logItem(tr("Checking downloaded bootloader"), LOGINFO);
    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    char magic[4];
    m_tempfile.seek(0x1fc);
    m_tempfile.read(magic, 4);
    m_tempfile.close();
    if(memcmp(sansa.targetname, magic, 4) != 0) {
        emit logItem(tr("Bootloader mismatch! Aborting."), LOGERROR);
        qDebug("[BootloaderInstallSansa] Targetname: %s, mi4 magic: %c%c%c%c",
                sansa.targetname, magic[0], magic[1], magic[2], magic[3]);
        emit done(true);
        sansa_close(&sansa);
        return;
    }

    if(sansa_add_bootloader(&sansa, blfile.toLatin1().data(),
            FILETYPE_MI4) == 0) {
        emit logItem(tr("Successfully installed bootloader"), LOGOK);
        sansa_close(&sansa);
#if defined(Q_OS_MACX)
        m_remountDevice = sansa.diskname;
        connect(this, SIGNAL(remounted(bool)), this, SLOT(installStage3(bool)));
        waitRemount();
#else
        installStage3(true);
#endif
    }
    else {
        emit logItem(tr("Failed to install bootloader"), LOGERROR);
        sansa_close(&sansa);
        emit done(true);
        return;
    }

}


void BootloaderInstallSansa::installStage3(bool mounted)
{
    if(mounted) {
        logInstall(LogAdd);
        emit logItem(tr("Bootloader Installation complete"), LOGINFO);
        emit done(false);
        return;
    }
    else {
        emit logItem(tr("Writing log aborted"), LOGERROR);
        emit done(true);
    }
    qDebug() << "version installed:" << m_blversion.toString(Qt::ISODate);
}


/** Uninstall the bootloader.
 */
bool BootloaderInstallSansa::uninstall(void)
{
    struct sansa_t sansa;

    emit logItem(tr("Uninstalling bootloader"), LOGINFO);
    QCoreApplication::processEvents();

    if(!sansaInitialize(&sansa)) {
        emit done(true);
        return false;
    }

    if (sansa.hasoldbootloader) {
        emit logItem(tr("OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
                    "You must reinstall the original Sansa firmware before running\n"
                    "sansapatcher for the first time.\n"
                    "See http://www.rockbox.org/wiki/SansaE200Install\n"),
                    LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_reopen_rw(&sansa) < 0) {
        emit logItem(tr("Could not open Sansa in R/W mode"), LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_delete_bootloader(&sansa)==0) {
        emit logItem(tr("Successfully removed bootloader"), LOGOK);
        logInstall(LogRemove);
        emit done(false);
        sansa_close(&sansa);
        return true;
    }
    else {
        emit logItem(tr("Removing bootloader failed."),LOGERROR);
        emit done(true);
        sansa_close(&sansa);
        return false;
    }

    return false;
}


/** Check if bootloader is already installed
 */
BootloaderInstallBase::BootloaderType BootloaderInstallSansa::installed(void)
{
    struct sansa_t sansa;
    int num;

    if(!sansaInitialize(&sansa)) {
        return BootloaderUnknown;
    }
    if((num = sansa_list_images(&sansa)) == 2) {
        sansa_close(&sansa);
        return BootloaderRockbox;
    }
    else if(num == 1) {
        sansa_close(&sansa);
        return BootloaderOther;
    }
    return BootloaderUnknown;

}

bool BootloaderInstallSansa::sansaInitialize(struct sansa_t *sansa)
{
    if(!m_blfile.isEmpty()) {
#if defined(Q_OS_WIN32)
        sprintf(sansa->diskname, "\\\\.\\PhysicalDrive%i",
                Autodetection::resolveDevicename(m_blfile).toInt());
#elif defined(Q_OS_MACX)
        sprintf(sansa->diskname,
            qPrintable(Autodetection::resolveDevicename(m_blfile).remove(QRegExp("s[0-9]+$"))));
#else
        sprintf(sansa->diskname,
            qPrintable(Autodetection::resolveDevicename(m_blfile).remove(QRegExp("[0-9]+$"))));
#endif
        qDebug() << "[BootloaderInstallSansa] sansapatcher: overriding scan, using"
                 << sansa->diskname;
    }
    else if(sansa_scan(sansa) != 1) {
        emit logItem(tr("Can't find Sansa"), LOGERROR);
        return false;
    }

    if (sansa_open(sansa, 0) < 0) {
        emit logItem(tr("Could not open Sansa"), LOGERROR);
        return false;
    }

    if (sansa_read_partinfo(sansa,0) < 0) {
        emit logItem(tr("Could not read partition table"), LOGERROR);
        sansa_close(sansa);
        return false;
    }

    int i = is_sansa(sansa);
    if(i < 0) {
        emit logItem(tr("Disk is not a Sansa (Error %1), aborting.").arg(i), LOGERROR);
        sansa_close(sansa);
        return false;
    }
    return true;
}


/** Get capabilities of subclass installer.
 */
BootloaderInstallBase::Capabilities BootloaderInstallSansa::capabilities(void)
{
    return (Install | Uninstall | IsRaw | CanCheckInstalled);
}

