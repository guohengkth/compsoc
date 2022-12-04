EMPTY:=

include $(SYSVEP)/tiles/tiles.mk

#hack for now
include $(SYSVEP)/tiles/*.mk

ifeq ($(LIBNAME),$(EMPTY))
$(error Library name is not set)
endif

##
# Default target
##
all:

PREFIX:=/opt/riscv/bin/riscv32-unknown-elf
#TODO move this/
MB_GCC := ${PREFIX}-gcc
MB_OBJCOPY := ${PREFIX}-objcopy
ARFLAGS := $(ARFLAGS)U
C_SOURCES:=$(filter %.c,$(SOURCES))
C_BASE_OBJECTS:=$(C_SOURCES:.c=.o)

S_SOURCES:=$(filter %.S,$(SOURCES))
S_BASE_OBJECTS:=$(S_SOURCES:.S=.o)


lib:
	@mkdir -p $@


define LIBRARY_TARGET_TILE
TARGET_LIB_$(1) := lib/lib$(LIBNAME)_$(1).a
OBJECTS_$(1):=$(foreach obj,$(C_BASE_OBJECTS),libsrc/$(1)_$(notdir $(obj))) $(foreach obj,$(S_BASE_OBJECTS),libsrc/$(1)_$(notdir $(obj)))
endef

define LIBRARY_TILE
$(info $(TARGET_LIB_$(1))($(OBJECTS_$(1))))

all:   $(TARGET_LIB_$(1))($(OBJECTS_$(1)))
clean: clean-$(TARGET_LIB_$(1))

$(TARGET_LIB_$(1))($(OBJECTS_$(1))): $(OBJECTS_$(1))

clean-$(TARGET_LIB_$(1)):
	rm -f $(TARGET_LIB_$(1)) $(OBJECTS_$(1))

$(foreach obj,$(C_BASE_OBJECTS),$(eval $(call LIBRARY_TILE_OBJECT_C,$(1)_$(notdir $(obj)),$(notdir $(obj)),$(1))))
$(foreach obj,$(S_BASE_OBJECTS),$(eval $(call LIBRARY_TILE_OBJECT_S,$(1)_$(notdir $(obj)),$(notdir $(obj)),$(1))))
endef

define LIBRARY_TILE_OBJECT_C
#todo load tile flags.
libsrc/$(1): libsrc/$(2:.o=.c)
	$(MB_GCC) $(COMPILER_FLAGS) $(CFLAGS) -c -o libsrc/$(1)  libsrc/$(2:.o=.c)
endef

define LIBRARY_TILE_OBJECT_S
#todo load tile flags.
libsrc/$(1): libsrc/$(2:.o=.S)
	$(MB_GCC) $(COMPILER_FLAGS) $(CFLAGS) -c -o libsrc/$(1)  libsrc/$(2:.o=.S)
endef

##
# Generate rules
##
$(foreach tile,$(TILES),$(eval $(call LIBRARY_TARGET_TILE,$(tile))))
$(foreach tile,$(TILES),$(eval $(call LIBRARY_TILE,$(tile))))

.PHONY: clean realclean all
