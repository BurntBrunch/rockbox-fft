/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "talkgenerator.h"
#include "rbsettings.h"
#include "systeminfo.h"
#include "wavtrim.h"

TalkGenerator::TalkGenerator(QObject* parent): QObject(parent), encFutureWatcher(this), ttsFutureWatcher(this)
{

}

//! \brief Creates Talkfiles.
//!
TalkGenerator::Status TalkGenerator::process(QList<TalkEntry>* list,int wavtrimth)
{
    QString errStr;
    bool warnings = false;

    //tts
    emit logItem(tr("Starting TTS Engine"),LOGINFO);
    m_tts = TTSBase::getTTS(this,RbSettings::value(RbSettings::Tts).toString());
    if(!m_tts->start(&errStr))
    {
        emit logItem(errStr.trimmed(),LOGERROR);
        emit logItem(tr("Init of TTS engine failed"),LOGERROR);
        emit done(true);
        return eERROR;
    }
    QCoreApplication::processEvents();

    // Encoder
    emit logItem(tr("Starting Encoder Engine"),LOGINFO);
    m_enc = EncBase::getEncoder(this,SystemInfo::value(SystemInfo::CurEncoder).toString());
    if(!m_enc->start())
    {
        emit logItem(tr("Init of Encoder engine failed"),LOGERROR);
        emit done(true);
        m_tts->stop();
        return eERROR;
    }
    QCoreApplication::processEvents();

    emit logProgress(0,0);

    // Voice entries
    emit logItem(tr("Voicing entries..."),LOGINFO);
    Status voiceStatus= voiceList(list,wavtrimth);
    if(voiceStatus == eERROR)
    {
        m_tts->stop();
        m_enc->stop();
        emit done(true);
        return eERROR;
    }
    else if( voiceStatus == eWARNING)
        warnings = true;

    QCoreApplication::processEvents();

    // Encoding Entries
    emit logItem(tr("Encoding files..."),LOGINFO);
    Status encoderStatus = encodeList(list);
    if( encoderStatus == eERROR)
    {
        m_tts->stop();
        m_enc->stop();
        emit done(true);
        return eERROR;
    }
    else if( voiceStatus == eWARNING)
        warnings = true;

    QCoreApplication::processEvents();

    m_tts->stop();
    m_enc->stop();
    emit logProgress(1,1);

    if(warnings)
        return eWARNING;
    return eOK;
}

//! \brief Voices a List of string
//!
TalkGenerator::Status TalkGenerator::voiceList(QList<TalkEntry>* list,int wavtrimth)
{
    emit logProgress(0, list->size());

    QStringList duplicates;

    m_ttsWarnings = false;
    for(int i=0; i < list->size(); i++)
    {
        (*list)[i].tts = m_tts;
        (*list)[i].wavtrim = wavtrimth;
            
        // skip duplicated wav entrys
        if(!duplicates.contains(list->at(i).wavfilename))
            duplicates.append(list->at(i).wavfilename);
        else
        {
            qDebug() << "[TalkGen] duplicate skipped";
            (*list)[i].voiced = true;
            continue;
        }
    }

    /* If the engine can't be parallelized, we use only 1 thread */
    int maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    if ((m_tts->capabilities() & RunInParallel) == 0)
        QThreadPool::globalInstance()->setMaxThreadCount(1);

    connect(&ttsFutureWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(ttsProgress(int)));
    ttsFutureWatcher.setFuture(QtConcurrent::map(*list, &TalkGenerator::ttsEntryPoint));

    /* We use this loop as an equivalent to ttsFutureWatcher.waitForFinished() 
     * since the latter blocks all events */
    while(ttsFutureWatcher.isRunning())
        QCoreApplication::processEvents();
    
    /* Restore global settings, if we changed them */
    if ((m_tts->capabilities() & RunInParallel) == 0)
        QThreadPool::globalInstance()->setMaxThreadCount(maxThreadCount);

    if(ttsFutureWatcher.isCanceled())
        return eERROR;
    else if(m_ttsWarnings)
        return eWARNING;
    else
        return eOK;
} 

void TalkGenerator::ttsEntryPoint(TalkEntry& entry)
{
    if (!entry.voiced && !entry.toSpeak.isEmpty())
    {
        QString error;
        qDebug() << "[TalkGen] voicing: " << entry.toSpeak << "to" << entry.wavfilename;
        TTSStatus status = entry.tts->voice(entry.toSpeak,entry.wavfilename, &error);
        if (status == Warning || status == FatalError)
        {
            entry.generator->ttsFailEntry(entry, status, error);
            return;
        }
        if (entry.wavtrim != -1)
        {
            char buffer[255];
            wavtrim(entry.wavfilename.toLocal8Bit().data(),entry.wavtrim,buffer,255);
        }
    }
    entry.voiced = true;
}

void TalkGenerator::ttsFailEntry(const TalkEntry& entry, TTSStatus status, QString error)
{
    if (status == Warning)
    {
        m_ttsWarnings = true;
        emit logItem(tr("Voicing of %1 failed: %2").arg(entry.toSpeak).arg(error),
                    LOGWARNING);
    }
    else if (status == FatalError)
    {
        emit logItem(tr("Voicing of %1 failed: %2").arg(entry.toSpeak).arg(error),
                    LOGERROR);
        abort();
    }
}

void TalkGenerator::ttsProgress(int value)
{
    emit logProgress(value,ttsFutureWatcher.progressMaximum());
}

//! \brief Encodes a List of strings
//!
TalkGenerator::Status TalkGenerator::encodeList(QList<TalkEntry>* list)
{
    QStringList duplicates;

    int itemsCount = list->size();
    emit logProgress(0, itemsCount);

    /* Do some preprocessing and remove the invalid entries. As far as I can see,
     * this list is not going to be used anywhere else, so we might as well butcher it*/
    for (int idx=0; idx < itemsCount; idx++)
    {
        if(list->at(idx).voiced == false)
        {
            qDebug() << "non voiced entry" << list->at(idx).toSpeak <<"detected";
            list->removeAt(idx);
            itemsCount--;
            idx--;
            continue;
        }
        if(duplicates.contains(list->at(idx).talkfilename))
        {
            list->removeAt(idx);
            itemsCount--;
            idx--;
            continue;
        }
        duplicates.append(list->at(idx).talkfilename);
        (*list)[idx].encoder = m_enc;
        (*list)[idx].generator = this;
    }

    connect(&encFutureWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(encProgress(int)));
    encFutureWatcher.setFuture(QtConcurrent::map(*list, &TalkGenerator::encEntryPoint));

    /* We use this loop as an equivalent to encFutureWatcher.waitForFinished() 
     * since the latter blocks all events */
    while (encFutureWatcher.isRunning())
        QCoreApplication::processEvents(QEventLoop::AllEvents);

    if (encFutureWatcher.isCanceled())
        return eERROR;
    else
        return eOK;
}

void TalkGenerator::encEntryPoint(TalkEntry& entry)
{
    bool res = entry.encoder->encode(entry.wavfilename, entry.talkfilename);
    entry.encoded = res;
    if (!entry.encoded)
        entry.generator->encFailEntry(entry);
    return;
}

void TalkGenerator::encProgress(int value)
{
    emit logProgress(value, encFutureWatcher.progressMaximum());
}

void TalkGenerator::encFailEntry(const TalkEntry& entry)
{
    emit logItem(tr("Encoding of %1 failed").arg(entry.wavfilename), LOGERROR);
    abort();      
}

//! \brief slot, which is connected to the abort of the Logger. Sets a flag, so Creating Talkfiles ends at the next possible position
//!
void TalkGenerator::abort()
{
    if (ttsFutureWatcher.isRunning())
    {
        ttsFutureWatcher.cancel();
        ttsFutureWatcher.waitForFinished();
        emit logItem(tr("Voicing aborted"), LOGERROR);
    }
    if (encFutureWatcher.isRunning())
    {
        encFutureWatcher.cancel();
        encFutureWatcher.waitForFinished();
        emit logItem(tr("Encoding aborted"), LOGERROR);
    }
}

