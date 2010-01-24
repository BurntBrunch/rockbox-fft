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

#include "rbsettings.h"
#include "systeminfo.h"
#include <QSettings>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

// user settings
const static struct {
    RbSettings::UserSettings setting;
    const char* name;
    const char* def;
} UserSettingsList[] = {
    { RbSettings::RbutilVersion,        "rbutil_version",       "" },
    { RbSettings::CurrentPlatform,      "platform",             "" },
    { RbSettings::Mountpoint,           "mountpoint",           "" },
    { RbSettings::CachePath,            "cachepath",            "" },
    { RbSettings::Build,                "build",                "" },
    { RbSettings::ProxyType,            "proxytype",            "" },
    { RbSettings::Proxy,                "proxy",                "" },
    { RbSettings::OfPath,               "ofpath",               "" },
    { RbSettings::Platform,             "platform",             "" },
    { RbSettings::Language,             "lang",                 "" },
    { RbSettings::Tts,                  "tts",                  "" },
    { RbSettings::LastTalkedFolder,     "last_talked_folder",   "" },
    { RbSettings::VoiceLanguage,        "voicelanguage",        "" },
    { RbSettings::TtsLanguage,          ":tts:/language",       "" },
    { RbSettings::TtsOptions,           ":tts:/options",        "" },
    { RbSettings::TtsPath,              ":tts:/path",           "" },
    { RbSettings::TtsVoice,             ":tts:/voice",          "" },
    { RbSettings::EncoderPath,          ":encoder:/encoderpath",        "" },
    { RbSettings::EncoderOptions,       ":encoder:/encoderoptions",     "" },
    { RbSettings::CacheOffline,         "offline",              "false" },
    { RbSettings::CacheDisabled,        "cachedisable",         "false" },
    { RbSettings::TtsUseSapi4,          "sapi/useSapi4",        "false" },
    { RbSettings::EncoderNarrowBand,    ":encoder:/narrowband", "false" },
    { RbSettings::WavtrimThreshold,     "wavtrimthreshold",     "500"},
    { RbSettings::TtsSpeed,             ":tts:/speed",          "0" },
    { RbSettings::EncoderComplexity,    ":encoder:/complexity", "10" },
    { RbSettings::EncoderQuality,       ":encoder:/quality",    "8.0" },
    { RbSettings::EncoderVolume,        ":encoder:/volume",     "1.0" },
};

//! pointer to setting object to NULL
QSettings* RbSettings::userSettings = NULL;

void RbSettings::ensureRbSettingsExists()
{
    if(userSettings == NULL)
    {
        // portable installation:
        // check for a configuration file in the program folder.
        QFileInfo config;
        config.setFile(QCoreApplication::instance()->applicationDirPath()
            + "/RockboxUtility.ini");
        if(config.isFile())
        {
            userSettings = new QSettings(QCoreApplication::instance()->applicationDirPath()
                + "/RockboxUtility.ini", QSettings::IniFormat, NULL);
            qDebug() << "[Settings] configuration: portable";
        }
        else
        {
            userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility",NULL);
            qDebug() << "[Settings] configuration: system";
        }
    }
}

void RbSettings::sync()
{
    ensureRbSettingsExists();

    userSettings->sync();
#if defined(Q_OS_LINUX)
    // when using sudo it runs rbutil with uid 0 but unfortunately without a
    // proper login shell, thus the configuration file gets placed in the
    // calling users $HOME. This in turn will cause issues if trying to
    // run rbutil later as user. Try to detect this case via the environment
    // variable SUDO_UID and SUDO_GID and if set chown the user config file.
    if(getuid() == 0)
    {
        char* realuser = getenv("SUDO_UID");
        char* realgroup = getenv("SUDO_GID");
        if(realuser != NULL && realgroup != NULL)
        {
            int realuid = atoi(realuser);
            int realgid = atoi(realgroup);
            // chown is attribute warn_unused_result, but in case this fails
            // we can't do anything useful about it. Notifying the user
            // is somewhat pointless. Add hack to suppress compiler warning.
            if(chown(qPrintable(userSettings->fileName()), realuid, realgid))
            { }
        }
    }
#endif
}

QString RbSettings::userSettingFilename()
{
    ensureRbSettingsExists();
    return userSettings->fileName();
}

QVariant RbSettings::value(enum UserSettings setting)
{
    QString empty;
    return subValue(empty, setting);
}

QVariant RbSettings::subValue(QString sub, enum UserSettings setting)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    qDebug() << "[Settings] GET U:" << s << userSettings->value(s).toString();
    return userSettings->value(s, UserSettingsList[i].def);
}

void RbSettings::setValue(enum UserSettings setting , QVariant value)
{
   QString empty;
   return setSubValue(empty, setting, value);
}

void RbSettings::setSubValue(QString sub, enum UserSettings setting, QVariant value)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    userSettings->setValue(s, value);
    qDebug() << "[Settings] SET U:" << s << userSettings->value(s).toString();
}

QString RbSettings::constructSettingPath(QString path, QString substitute)
{
    QString platform = userSettings->value("platform").toString();
    if(!substitute.isEmpty()) {
        path.replace(":tts:", substitute);
        path.replace(":encoder:", substitute);
    }
    else {
        path.replace(":tts:", userSettings->value("tts").toString());
        path.replace(":encoder:", SystemInfo::platformValue(platform,SystemInfo::CurEncoder).toString());
    }
    path.replace(":platform:", platform);

    return path;
}

