# Motor de Bloques 3DS (C++). Requiere devkitPro: devkitARM + libctru + citro2d + citro3d.
#   dkp-pacman -S 3ds-dev        (una sola vez)
#   make                         -> MotorBloques3DS.3dsx (cópialo a /3ds/ en la SD y ábrelo con Homebrew Launcher)
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error Define DEVKITARM (instala devkitPro): export DEVKITARM=/opt/devkitpro/devkitARM)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET   := MotorBloques3DS
BUILD    := build
SOURCES  := source
INCLUDES := source
ROMFS    := romfs

ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -std=gnu++17
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS  := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export LD      := $(CXX)
export OFILES  := $(CPPFILES:.cpp=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
.PHONY: $(BUILD) clean all
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf
else
.PHONY: all
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).3dsx
$(OUTPUT).3dsx: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
