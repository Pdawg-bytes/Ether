ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET          := Ether
BUILD           := 3DS/build
OUTPUT_ROOT     := $(TOPDIR)/3DS/$(TARGET)
ROMFS           := Data
ARCH            := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
DEVKITPRO_UNIX  := $(shell if command -v cygpath >/dev/null 2>&1; then cygpath -u "$(DEVKITPRO)"; else echo "$(DEVKITPRO)"; fi)
LIBDIRS         := $(DEVKITPRO_UNIX)/libctru
INCLUDE         := -I$(TOPDIR)/Ether -I$(TOPDIR)/Ether/Platform \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(TOPDIR)/$(BUILD)

ARCH_FLAGS      := -g -Wall -O3 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS          := $(ARCH_FLAGS) -D__3DS__ -include $(TOPDIR)/Ether/GlobalTypes.h $(INCLUDE)
CXXFLAGS        := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS         := -g $(ARCH)
LDFLAGS         := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS            := -lctru -lm
LIBPATHS        := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

rwildcard       = $(foreach dir,$(wildcard $1*/),$(call rwildcard,$(dir),$2)) $(wildcard $1$2)
SOURCE_FILES    := $(call rwildcard,$(TOPDIR)/Ether/,*.cpp)
CPPFILES        := $(filter-out WindowsRuntime.cpp Window.cpp ThreadPool.cpp,$(notdir $(SOURCE_FILES)))
VPATH           := $(sort $(dir $(SOURCE_FILES)))

ifneq ($(notdir $(CURDIR)),build)

export OUTPUT  := $(OUTPUT_ROOT)
export TOPDIR  := $(TOPDIR)
export VPATH   := $(VPATH)
export DEPSDIR := $(TOPDIR)/$(BUILD)
export LD      := $(CXX)
export OFILES_SOURCES := $(CPPFILES:.cpp=.o)
export OFILES_BIN     :=
export OFILES         := $(OFILES_SOURCES)
export HFILES         :=
export INCLUDE        := $(INCLUDE)
export LIBPATHS       := $(LIBPATHS)
export CFLAGS         := $(CFLAGS)
export CXXFLAGS       := $(CXXFLAGS)
export _3DSXFLAGS     := --smdh=$(OUTPUT_ROOT).smdh --romfs=$(TOPDIR)/$(ROMFS)

.PHONY: all clean $(BUILD)

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(TOPDIR)/3DS

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif