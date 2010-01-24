#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# single-file plugins:
PLUGINS_SRC = $(call preprocess, $(APPSDIR)/plugins/SOURCES)
OTHER_SRC += $(PLUGINS_SRC)
ROCKS1 := $(PLUGINS_SRC:.c=.rock)
ROCKS1 := $(subst $(ROOTDIR),$(BUILDDIR),$(ROCKS1))

ROCKS := $(ROCKS1)

ROCKS1 := $(ROCKS1:%.lua=)

# libplugin.a
PLUGINLIB := $(BUILDDIR)/apps/plugins/libplugin.a
PLUGINLIB_SRC = $(call preprocess, $(APPSDIR)/plugins/lib/SOURCES)
OTHER_SRC += $(PLUGINLIB_SRC)

PLUGINLIB_OBJ := $(PLUGINLIB_SRC:.c=.o)
PLUGINLIB_OBJ := $(PLUGINLIB_OBJ:.S=.o)
PLUGINLIB_OBJ := $(subst $(ROOTDIR),$(BUILDDIR),$(PLUGINLIB_OBJ))

### build data / rules
ifndef SIMVER
CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h
PLUGIN_LDS := $(APPSDIR)/plugins/plugin.lds
PLUGINLINK_LDS := $(BUILDDIR)/apps/plugins/plugin.link
OVERLAYREF_LDS := $(BUILDDIR)/apps/plugins/overlay_ref.link
endif

# multifile plugins (subdirs):
PLUGINSUBDIRS := $(call preprocess, $(APPSDIR)/plugins/SUBDIRS)

# include <dir>.make from each subdir (yay!)
$(foreach dir,$(PLUGINSUBDIRS),$(eval include $(dir)/$(notdir $(dir)).make))

OTHER_INC += -I$(APPSDIR)/plugins -I$(APPSDIR)/plugins/lib

# special compile flags for plugins:
PLUGINFLAGS = -I$(APPSDIR)/plugins -DPLUGIN $(CFLAGS)

# single-file plugins depend on their respective .o
$(ROCKS1): $(BUILDDIR)/%.rock: $(BUILDDIR)/%.o

# dependency for all plugins
$(ROCKS): $(APPSDIR)/plugin.h $(PLUGINLINK_LDS) $(PLUGINLIB) $(PLUGINBITMAPLIB)

$(PLUGINLIB): $(PLUGINLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

$(PLUGINLINK_LDS): $(PLUGIN_LDS) $(CONFIGFILE)
	$(call PRINTS,PP $(@F))
	$(shell mkdir -p $(dir $@))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(OVERLAYREF_LDS): $(PLUGIN_LDS)
	$(call PRINTS,PP $(@F))
	$(shell mkdir -p $(dir $@))
	$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=0)

$(BUILDDIR)/credits.raw credits.raw: $(DOCSDIR)/CREDITS
	$(call PRINTS,Create credits.raw)perl $(APPSDIR)/plugins/credits.pl < $< > $(BUILDDIR)/$(@F)

# special dependencies
$(BUILDDIR)/apps/plugins/wav2wv.rock: $(BUILDDIR)/apps/codecs/libwavpack.a $(PLUGINLIB)

# special pattern rule for compiling plugin lib (with function and data sections)
$(BUILDDIR)/apps/plugins/lib/%.o: $(ROOTDIR)/apps/plugins/lib/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PLUGINFLAGS) -ffunction-sections -fdata-sections -c $< -o $@

# special pattern rule for compiling plugins with extra flags
$(BUILDDIR)/apps/plugins/%.o: $(ROOTDIR)/apps/plugins/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PLUGINFLAGS) -c $< -o $@

ifdef SIMVER
 PLUGINLDFLAGS = $(SHARED_FLAG) # <-- from Makefile
else
 PLUGINLDFLAGS = -T$(PLUGINLINK_LDS) -Wl,--gc-sections -Wl,-Map,$*.map
 OVERLAYLDFLAGS = -T$(OVERLAYREF_LDS) -Wl,--gc-sections -Wl,-Map,$*.refmap
endif

$(BUILDDIR)/%.rock:
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $(BUILDDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(PLUGINLDFLAGS)
ifdef SIMVER
	$(SILENT)cp $(BUILDDIR)/$*.elf $@
else
	$(SILENT)$(OC) -O binary $(BUILDDIR)/$*.elf $@
endif

$(BUILDDIR)/apps/plugins/%.lua: $(ROOTDIR)/apps/plugins/%.lua
	$(call PRINTS,CP $(subst $(ROOTDIR)/,,$<))cp $< $(BUILDDIR)/apps/plugins/

$(BUILDDIR)/%.refmap: $(APPSDIR)/plugin.h $(OVERLAYREF_LDS) $(PLUGINLIB) $(PLUGINBITMAPLIB)
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o /dev/null \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(OVERLAYLDFLAGS)
