#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# libtremor
TREMORLIB := $(CODECDIR)/libtremor.a
TREMORLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/libtremor/SOURCES)
TREMORLIB_OBJ := $(call c2obj, $(TREMORLIB_SRC))
OTHER_SRC += $(TREMORLIB_SRC)

$(TREMORLIB): $(TREMORLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

TREMORFLAGS = -I$(APPSDIR)/codecs/libtremor $(filter-out -O%,$(CODECFLAGS)) 

# Tremor is slightly faster on coldfire with -O3
ifeq ($(CPU),coldfire)
    TREMORFLAGS += -O3
else
    TREMORFLAGS += -O2
endif

$(CODECDIR)/libtremor/%.o: $(ROOTDIR)/apps/codecs/libtremor/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(TREMORFLAGS) -c $< -o $@
