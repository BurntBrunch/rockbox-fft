/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Eric Linenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#ifdef USE_GAMES

#include <sprintf.h>
#include "sokoban.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include <string.h>

#define SOKOBAN_TITLE   "Sokoban"
#define SOKOBAN_TITLE_FONT  2
#define NUM_LEVELS  sizeof(levels)/320

static char board[16][20];
static int current_level=0;
static int moves=0;
static int row=0;
static int col=0;
static int boxes_to_go=0;
static int current_spot=1;

/* 320 boxes per level */
static const char levels[][320] = {
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222000000000"
    "00000000232000000000"
    "00000000212222000000"
    "00000022241432000000"
    "00000023145222000000"
    "00000022224200000000"
    "00000000023200000000"
    "00000000022200000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222220000000000"
    "00000211120000000000"
    "00000254420222000000"
    "00000214120232000000"
    "00000222122232000000"
    "00000022111132000000"
    "00000021112112000000"
    "00000021112222000000"
    "00000022222000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222220000000"
    "00000021111122200000"
    "00000224222111200000"
    "00000215141141200000"
    "00000213321412200000"
    "00000223321112000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000022222000000"
    "00000022221132000000"
    "00000021114172000000"
    "00000021441432000000"
    "00000022522332000000"
    "00000002222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222000000000"
    "00000002512220000000"
    "00000002141120000000"
    "00000022212122000000"
    "00000023212112000000"
    "00000023411212000000"
    "00000023111412000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222222200000"
    "00000002211215200000"
    "00000002111211200000"
    "00000002414141200000"
    "00000002142211200000"
    "00000222141212200000"
    "00000233333112000000"
    "00000222222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222222000000"
    "00000022211112000000"
    "00000223142212200000"
    "00000233414115200000"
    "00000233141412200000"
    "00000222222112000000"
    "00000000002222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222222000000"
    "00000000211112000000"
    "00000022244412000000"
    "00000025143312000000"
    "00000021433322000000"
    "00000022221120000000"
    "00000000022220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222200000000"
    "00000000233200000000"
    "00000002213220000000"
    "00000002114320000000"
    "00000022141122000000"
    "00000021124412000000"
    "00000021151112000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222220000000"
    "00000022211520000000"
    "00000021143122000000"
    "00000021134312000000"
    "00000022217412000000"
    "00000000211122000000"
    "00000000222220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222000000"
    "00000021121112000000"
    "00000021433412000000"
    "00000025437122000000"
    "00000021433412000000"
    "00000021121112000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222220000000"
    "00000002111122200000"
    "00000002141111200000"
    "00000222141221200000"
    "00000233314111200000"
    "00000233342412200000"
    "00000222212141200000"
    "00000000211511200000"
    "00000000222222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222200000000"
    "00000021111200000000"
    "00000021444220000000"
    "00000021123322200000"
    "00000022113341200000"
    "00000002151111200000"
    "00000002222222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222222200000"
    "00000002111231200000"
    "00000022114333200000"
    "00000021141273200000"
    "00000221224212200000"
    "00000211141141200000"
    "00000211121111200000"
    "00000222222251200000"
    "00000000000222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222220000000"
    "00000023333120000000"
    "00000222333422200000"
    "00000211424141200000"
    "00000214411241200000"
    "00000211112111200000"
    "00000222215122200000"
    "00000000222220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222000000"
    "00000023311112000000"
    "00000023341452000000"
    "00000024244422000000"
    "00000023341412000000"
    "00000023311112000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222222220000000"
    "00000211111120000000"
    "00000212441120000000"
    "00000213332120000000"
    "00000223334122000000"
    "00000021221412000000"
    "00000024114112000000"
    "00000021121152000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222200000000"
    "00000222111222200000"
    "00000211141411200000"
    "00000214111415200000"
    "00000222442222200000"
    "00000002113320000000"
    "00000002333320000000"
    "00000002222220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222222222000000"
    "00000211171112000000"
    "00000214343152000000"
    "00000213434312000000"
    "00000214343412000000"
    "00000211171112000000"
    "00000222222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222200000"
    "00000221111111200000"
    "00000211124241200000"
    "00000214411343200000"
    "00000215222333200000"
    "00000222202222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222220000000"
    "00000002313320000000"
    "00000002314320000000"
    "00000022211422000000"
    "00000021411412000000"
    "00000021242212000000"
    "00000021115112000000"
    "00000022222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222000000000"
    "00000021112222000000"
    "00000221241112000000"
    "00000214114412000000"
    "00000212423732000000"
    "00000211153332000000"
    "00000222222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000022222000000"
    "00000022221112200000"
    "00000021414111200000"
    "00000025237321200000"
    "00000021237321200000"
    "00000021114141200000"
    "00000022111222200000"
    "00000002222200000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222220000000"
    "00000222231122000000"
    "00000214343112000000"
    "00000254212412000000"
    "00000214313112000000"
    "00000222242412000000"
    "00000002313112000000"
    "00000002222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000222222220000"
    "00000222211113120000"
    "00000211414143120000"
    "00000211322223220000"
    "00000214341415200000"
    "00000211311112200000"
    "00000222222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222222200000000"
    "00000211111222220000"
    "00000214137314120000"
    "00000254371734120000"
    "00000214137314120000"
    "00000222222111120000"
    "00000000002222220000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222222000000000"
    "00000211112000000000"
    "00000214112222000000"
    "00000214733712000000"
    "00000217337412000000"
    "00000222211412000000"
    "00000000215112000000"
    "00000000222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000022222200000"
    "00000022223115200000"
    "00000021144411200000"
    "00000023223223200000"
    "00000021114111200000"
    "00000021143212200000"
    "00000022221112000000"
    "00000000022222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222200000"
    "00000023114315200000"
    "00000021343411200000"
    "00000022434142200000"
    "00000021343411200000"
    "00000023114311200000"
    "00000022222222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222222000000"
    "00000002115112000000"
    "00000002241412000000"
    "00000002114422000000"
    "00000002333320000000"
    "00000002222220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022220000000000"
    "00000021122222000000"
    "00000224122112000000"
    "00000211454112000000"
    "00000211122412000000"
    "00000222322122200000"
    "00000023334141200000"
    "00000022331111200000"
    "00000002222222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222000000"
    "00000021123312200000"
    "00000021431471200000"
    "00000021145411200000"
    "00000021741341200000"
    "00000022133211200000"
    "00000002222222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222220000000"
    "00000221171122000000"
    "00000213131312000000"
    "00000214171112000000"
    "00000274474472000000"
    "00000211171412000000"
    "00000213135312000000"
    "00000221171122000000"
    "00000022222220000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000222222222000000"
    "00000211134152000000"
    "00000214171412000000"
    "00000273337372000000"
    "00000214471112000000"
    "00000211131412000000"
    "00000222222222000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000022222200000"
    "00000222223111200000"
    "00000211233221200000"
    "00000211433111200000"
    "00000211213212200000"
    "00002221224211200000"
    "00002141111441200000"
    "00002124211211200000"
    "00002511222222200000"
    "00002222200000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000002222220000000"
    "00000002133320000000"
    "00002222333320000000"
    "00002112224122200000"
    "00002141411441200000"
    "00002514141111200000"
    "00002112222111200000"
    "00002222002222200000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222200000"
    "00000021122111200000"
    "00000021114111200000"
    "00000024122214200000"
    "00000021233321200000"
    "00000221233321220000"
    "00000214114114120000"
    "00000211111215120000"
    "00000222222222220000"
    "00000000000000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000222222200000"
    "00000022211111200000"
    "00000221112121200000"
    "00000211234441200000"
    "00000212372122200000"
    "00000211332120000000"
    "00000222334122000000"
    "00000002321412000000"
    "00000022121252000000"
    "00000021411412000000"
    "00000021111222000000"
    "00000022222200000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222220000"
    "00000221111111120000"
    "00000251444141120000"
    "00000221411414120000"
    "00000022122221220000"
    "00000002111141220000"
    "00000222122414120000"
    "00000211112111120000"
    "00000211112222220000"
    "00000211112200000000"
    "00000233333200000000"
    "00000233333200000000"
    "00000222222200000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000222222220000000"
    "00000211111120000000"
    "00000215142120000000"
    "00000221211120000000"
    "00000214232120000000"
    "00000211373420000000"
    "00000221232120000000"
    "00000021231122000000"
    "00000221432112000000"
    "00000211214112000000"
    "00000214111222000000"
    "00000211222200000000"
    "00000222200000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000222200000"
    "00000000000251200000"
    "00000002222211200000"
    "00002222111111200000"
    "00002111322212200000"
    "00002121211112200000"
    "00002121414231200000"
    "00002121171121200000"
    "00002132414121200000"
    "00002211112121200000"
    "00000212223111200000"
    "00000211111222200000"
    "00000222222200000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000002222200000000"
    "00000002111220000000"
    "00000002114120000000"
    "00000002141520000000"
    "00000002223120000000"
    "00000000023220000000"
    "00000000023120000000"
    "00000002223120000000"
    "00000002114120000000"
    "00000002141120000000"
    "00000002211220000000"
    "00000000222200000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000022220000000000"
    "00000021122222000000"
    "00000021121112000000"
    "00000021311212000000"
    "00000022421132000000"
    "00000002124212000000"
    "00000022311312000000"
    "00000021122112000000"
    "00000021121172000000"
    "00000021411412000000"
    "00000021427152000000"
    "00000021123222000000"
    "00000022222200000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000222000000000"
    "00000000232222220000"
    "00000022234121120000"
    "00000023331124120000"
    "00000023221411120000"
    "00000223414221220000"
    "00000234121221200000"
    "00000232411111200000"
    "00000234112414200000"
    "00000231422151200000"
    "00000234111141200000"
    "00000231222222200000"
    "00000222200000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000000022220000000"
    "00000002221122000000"
    "00000022111112200000"
    "00000021112133200000"
    "00000221427243200000"
    "00000211412143200000"
    "00000211415143200000"
    "00000211412143200000"
    "00000224427243200000"
    "00000023112133200000"
    "00000022311112200000"
    "00000002221122000000"
    "00000000022220000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000022222222200000"
    "00000021111111200000"
    "00000021212221200000"
    "00000021441121200000"
    "00000025411121200000"
    "00000022122121200000"
    "00000021114141200000"
    "00000021422121200000"
    "00000021112421200000"
    "00000023332111200000"
    "00000023232422200000"
    "00000023331120000000"
    "00000022222220000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000222222220000000"
    "00000211111320000000"
    "00000212423322000000"
    "00000211143332000000"
    "00000221223432000000"
    "00000021221122000000"
    "00000024221120000000"
    "00000221112420000000"
    "00000211211120000000"
    "00000211141222200000"
    "00000214141115200000"
    "00000211222111200000"
    "00000222202222200000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000022220000000000"
    "00000021122220000000"
    "00000021141120000000"
    "00000021232120000000"
    "00000021212120000000"
    "00000023434320000000"
    "00000021212120000000"
    "00000021232120000000"
    "00000021144120000000"
    "00000022151120000000"
    "00000002112220000000"
    "00000002222000000000"
    "00000000000000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00002222220222200000"
    "00002111122211200000"
    "00002144112211200000"
    "00002112141111200000"
    "00002212214211200000"
    "00000212333112200000"
    "00000211333242200000"
    "00000212333211200000"
    "00002212122251200000"
    "00002114141141200000"
    "00002124222211200000"
    "00002111200222200000"
    "00002222200000000000"
    "00000000000000000000",
    
    "00000000000000000000"
    "00000000000000000000"
    "00000222222222200000"
    "00000233414173200000"
    "00000237141433200000"
    "00000223414172200000"
    "00000027141432000000"
    "00000023414132000000"
    "00000023141432000000"
    "00000023454172000000"
    "00000227141432200000"
    "00000233414173200000"
    "00000237141433200000"
    "00000222222222200000"
    "00000000000000000000"
    "00000000000000000000"
};


void load_level (int level_to_load) {
    int a = 0;
    int b = 0;
    int c = 0;
    current_spot=1;
    boxes_to_go = 0;
    /* load level into board */
    /* get to the current level in the level array */
    
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
            board[b][c] = levels[level_to_load][a] - '0';
            a++;
            if (board[b][c]==5) {
                row = b;
                col = c;
            }
            if (board[b][c]==3)
                boxes_to_go++;        
        }
    }
    return;
}

void update_screen(void) {
    int b = 0;
    int c = 0;
    char s[25];

    /* load the board to the screen */
    for(b=0 ; b<16 ; b++) {
        for (c=0 ; c<20 ; c++) {
            switch ( board[b][c] ) {
                case 0: /* this is a black space */
                    lcd_drawrect (c*4, b*4, 4, 4);
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;

                case 2: /* this is a wall */
                    lcd_drawpixel (c*4, b*4);
                    lcd_drawpixel (c*4+2, b*4);
                    lcd_drawpixel (c*4+1, b*4+1);
                    lcd_drawpixel (c*4+3, b*4+1);
                    lcd_drawpixel (c*4,   b*4+2);
                    lcd_drawpixel (c*4+2, b*4+2);
                    lcd_drawpixel (c*4+1, b*4+3);
                    lcd_drawpixel (c*4+3, b*4+3);
                    break;

                case 3: /* this is a home location */
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;

                case 4: /* this is a box */
                    lcd_drawrect (c*4, b*4, 4, 4);
                    break;

                case 5: /* this is you */
                    lcd_drawline (c*4+1, b*4, c*4+2, b*4);
                    lcd_drawline (c*4, b*4+1, c*4+3, b*4+1);
                    lcd_drawline (c*4+1, b*4+2, c*4+2, b*4+2);
                    lcd_drawpixel (c*4, b*4+3);
                    lcd_drawpixel (c*4+3, b*4+3);
                    break;

                case 7: /* this is a box on a home spot */ 
                    lcd_drawrect (c*4, b*4, 4, 4);
                    lcd_drawrect (c*4+1, b*4+1, 2, 2);
                    break;
            }
        }
    }
    

    snprintf (s, sizeof(s), "%d", current_level+1);
    lcd_putsxy (86, 22, s, 0);
    snprintf (s, sizeof(s), "%d", moves);
    lcd_putsxy (86, 54, s, 0);

    lcd_drawrect (80,0,32,32);
    lcd_drawrect (80,32,32,64);
    lcd_putsxy (81, 10, "Level", 0);
    lcd_putsxy (81, 42, "Moves", 0);
    /* print out the screen */
    lcd_update();
}



void sokoban_loop(void) {
    int ii = 0;
    moves = 0;
    current_level = 0;
    load_level(current_level);
    update_screen();

    while(1) {
        bool idle = false;
        switch ( button_get(true) ) {

            case BUTTON_OFF:
                /* get out of here */
                return; 

            case BUTTON_F3:
                /* increase level */
                boxes_to_go=0;
                idle=true;
                break;

            case BUTTON_F2:
                /* same level */
                load_level(current_level);
                moves=0;
                idle=true;
                load_level(current_level);
                lcd_clear_display();
                update_screen();
                break;
            
            case BUTTON_F1:
                /* previous level */
                if (current_level)
                    current_level--;
                load_level(current_level);
                moves=0;
                idle=true;
                load_level(current_level);
                lcd_clear_display();
                update_screen();
                break;

            case BUTTON_LEFT:
                switch ( board[row][col-1] ) {
                    case 1: /* if it is a blank spot */
                        board[row][col-1]=5;
                        board[row][col]=current_spot;
                        current_spot=1;
                        break;

                    case 3: /* if it is a home spot */
                        board[row][col-1]=5;
                        board[row][col]=current_spot;
                        current_spot=3;
                        break;

                    case 4:
                        switch ( board[row][col-2] ) {
                            case 1: /* if we are going from blank to blank */
                                board[row][col-2]=board[row][col-1];
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=1;
                                break;

                            case 3: /* if we are going from a blank to home */
                                board[row][col-2]=7;
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=1;
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case 7:
                        switch ( board[row][col-2] ) {
                            case 1: /* we are going from a home to a blank */
                                board[row][col-2]=4;
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=3;
                                boxes_to_go++;
                                break;

                            case 3: /* if we are going from a home to home */
                                board[row][col-2]=7;
                                board[row][col-1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=3;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    col--;
                break;

            case BUTTON_RIGHT: /* if it is a blank spot */
                switch ( board[row][col+1] ) {
                    case 1:
                        board[row][col+1]=5;
                        board[row][col]=current_spot;
                        current_spot=1;
                        break;

                    case 3: /* if it is a home spot */
                        board[row][col+1]=5;
                        board[row][col]=current_spot;
                        current_spot=3;
                        break;

                    case 4: 
                        switch ( board[row][col+2] ) {
                            case 1: /* if we are going from blank to blank */
                                board[row][col+2]=board[row][col+1];
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=1;
                                break;

                            case 3: /* if we are going from a blank to home */
                                board[row][col+2]=7;
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=1;
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case 7:
                        switch ( board[row][col+2] ) {
                            case 1: /* we are going from a home to a blank */
                                board[row][col+2]=4;
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=3;
                                boxes_to_go++;
                                break;

                            case 3:
                                board[row][col+2]=7;
                                board[row][col+1]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=3;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    col++;
                break;

            case BUTTON_UP:
                switch ( board[row-1][col] ) {
                    case 1: /* if it is a blank spot */
                        board[row-1][col]=5;
                        board[row][col]=current_spot;
                        current_spot=1;
                        break;

                    case 3: /* if it is a home spot */
                        board[row-1][col]=5;
                        board[row][col]=current_spot;
                        current_spot=3;
                        break;

                    case 4:
                        switch ( board[row-2][col] ) {
                            case 1: /* if we are going from blank to blank */
                                board[row-2][col]=board[row-1][col];
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=1;
                                break;

                            case 3: /* if we are going from a blank to home */
                                board[row-2][col]=7;
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=1;
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case 7:
                        switch ( board[row-2][col] ) {
                            case 1: /* we are going from a home to a blank */
                                board[row-2][col]=4;
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=3;
                                boxes_to_go++;
                                break;

                            case 3: /* if we are going from a home to home */
                                board[row-2][col]=7;
                                board[row-1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=3;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    row--;
                break;

            case BUTTON_DOWN:
                switch ( board[row+1][col] ) {
                    case 1: /* if it is a blank spot */
                        board[row+1][col]=5;
                        board[row][col]=current_spot;
                        current_spot=1;
                        break;

                    case 3: /* if it is a home spot */
                        board[row+1][col]=5;
                        board[row][col]=current_spot;
                        current_spot=3;
                        break;

                    case 4:
                        switch ( board[row+2][col] ) {
                            case 1: /* if we are going from blank to blank */
                                board[row+2][col]=board[row+1][col];
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=1;
                                break;

                            case 3: /* if we are going from a blank to home */
                                board[row+2][col]=7;
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=1;
                                boxes_to_go--;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    case 7:
                        switch ( board[row+2][col] ) {
                            case 1: /* we are going from a home to a blank */
                                board[row+2][col]=4;
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot;
                                current_spot=3;
                                boxes_to_go++;
                                break;

                            case 3: /* if we are going from a home to home */
                                board[row+2][col]=7;
                                board[row+1][col]=board[row][col];
                                board[row][col]=current_spot; 
                                current_spot=3;
                                break;

                            default:
                                idle = true;
                                break;
                        }
                        break;

                    default:
                        idle = true;
                        break;
                }
                if (!idle)
                    row++;
                break;

            default:
                idle = true;
                break;
        }
        
        if (!idle) {
            moves++;
            lcd_clear_display();
            update_screen();
        }

        if (boxes_to_go==0) {
            moves=0;
            current_level++;
            if (current_level == NUM_LEVELS) {
                for(ii=0; ii<30 ; ii++) {
                    lcd_clear_display();
                    lcd_putsxy(10, 20, "YOU WIN!!", 2);
                    lcd_update();
                    lcd_invertrect(0,0,111,63);
                    lcd_update();
                    if ( button_get(false) )
                        return;
                }
                return;
            }
            load_level(current_level);
            lcd_clear_display();
            update_screen();
        }
    }
}


Menu sokoban(void)
{
    int w, h;
    int len = strlen(SOKOBAN_TITLE);

    lcd_getfontsize(SOKOBAN_TITLE_FONT, &w, &h);

    /* Get horizontel centering for text */
    len *= w;
    if (len%2 != 0)
        len = ((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    lcd_clear_display();
    lcd_putsxy(LCD_WIDTH/2-len, (LCD_HEIGHT/2)-h, SOKOBAN_TITLE, 
               SOKOBAN_TITLE_FONT);

    lcd_update();
    sleep(HZ*2);

    lcd_clear_display();

    lcd_putsxy( 3,12,  "[Off] to stop", 0);
    lcd_putsxy( 3,22, "[F1] - level",0);
    lcd_putsxy( 3,32, "[F2] same level",0);
    lcd_putsxy( 3,42, "[F3] + level",0);

    lcd_update();
    sleep(HZ*2);
    lcd_clear_display();
    sokoban_loop();

    return MENU_OK;
}

#endif
