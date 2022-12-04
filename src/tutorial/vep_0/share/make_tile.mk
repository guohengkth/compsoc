###
# Copyright Verintec
# Makefile for compiling code on a tile.
#
# Use dependency files for dependency tracking.
##

# Include the tile specific flags.
include $(SYSVEP)/tiles/tile${TILE_ID}_mb.mk

PREFIX:=/opt/riscv/bin/riscv32-unknown-elf

# Name used by xilinx for this specific tile.
TILE_NAME:=tile${TILE_ID}_mb

TARGET  := mb.elf
LINKER_SCRIPT := lscript.ld

CFLAGS += ${COMPILER_FLAGS}
CFLAGS += $(USER_FLAGS)
CFLAGS += $(foreach lib,$(USER_LIBS),-I$(WORKSPACE)/lib$(lib)/include)
CFLAGS += -I$(SHAREDMEM) -I../shared
CFLAGS += -I${SYSVEP}/tiles
CFLAGS += -DVEP_ID=${VEP_ID} -DTILE_ID=${TILE_ID} -DPARTITION_ID=${PARTITION_ID}


# Library
# the order of the SYS_LIBS is important, do not change
SYS_LIBS += bsp channel userchannel-riscv
CFLAGS += $(foreach lib,$(SYS_LIBS),-I${SYSVEP}/lib$(lib)/include/)
SYS_LIBS_TARGETS := $(foreach lib,$(SYS_LIBS),${SYSVEP}/lib$(lib)/lib/lib$(lib)_$(TILE_NAME).a)

SHAREDMEMH = $(wildcard $(SHAREDMEM)/*.h)
SHAREDMEMC = $(SHAREDMEM)/vep_shared_memories.c
SHAREDMEMO = $(SHAREDMEM)/partition_$(TILE_ID)_$(PARTITION_ID)_vep_shared_memories.o

##
# Default target
##
all: $(LINKER_SCRIPT) $(TARGET)


mm memmap: $(TOOLS)/generate-json
	@echo "----- memory map for tile ${TILE_ID} partition ${PARTITION_ID} (without other veps)"
	@if [ -f $(TOOLS)/vep-config.txt ] ; then \
	egrep "(mem${TILE_ID}.|mem.${TILE_ID})" ${TOOLS}/vep-config.txt | sed -e 's/#.*$$//g' -e 's/vep \([0-9]\) in \(mem..\) have \(.*\) memory starting at \(.*\)$$/\2 from \4 range \3 for vep \1/g' -e 's/  / /g' -e 's/K/ K/g' | egrep "(public|vep ${VEP_ID})" | sort | sed -e 's/ K/K/g' ; \
	else \
	echo 'warning: showing memory map with all veps (without ./run.sh -xv option)' ; \
	rm -f $(TOOLS)/vep-config.make ; \
	grep -v -H '^#' $(SYSWORKSPACE)/vep_[1-9]*/vep-config.txt | sed -e 's+../../vep_\([0-9][0-9]*\)/vep-config.txt:+vep \1 +g' -e 's/#.*$$//g' > $(TOOLS)/vep-config.make ; \
	egrep "(mem${TILE_ID}.|mem.${TILE_ID})" ${TOOLS}/vep-config.make | sed -e 's/#.*$$//g' -e 's/vep \([0-9]\) in \(mem..\) have \(.*\) memory starting at \(.*\)$$/\2 from \4 range \3 for vep \1/g' -e 's/  / /g' -e 's/K/ K/g' | egrep "(public|vep ${VEP_ID})" | sort | sed -e 's/ K/K/g' ; \
	rm -f $(TOOLS)/vep-config.make ; \
	fi
	@echo "-----"

mma memmap-all: $(TOOLS)/generate-json
	@echo "----- memory map for tile ${TILE_ID} partition ${PARTITION_ID} (including other veps)"
	@if [ -f $(TOOLS)/vep-config.txt ] ; then \
	egrep "(mem${TILE_ID}.|mem.${TILE_ID})" ${TOOLS}/vep-config.txt | sed -e 's/#.*$$//g' -e 's/vep \([0-9]\) in \(mem..\) have \(.*\) memory starting at \(.*\)$$/\2 from \4 range \3 for vep \1/g' -e 's/  / /g'  -e 's/K/ K/g' | sort | sed -e 's/ K/K/g' ; \
else \
echo 'warning: showing memory map with all veps (without ./run.sh -xv option)' ; \
	rm -f $(TOOLS)/vep-config.make ; \
	grep -v -H '^#' $(SYSWORKSPACE)/vep_[1-9]*/vep-config.txt | sed -e 's+../../vep_\([0-9][0-9]*\)/vep-config.txt:+vep \1 +g' -e 's/#.*$$//g' > $(TOOLS)/vep-config.make ; \
	egrep "(mem${TILE_ID}.|mem.${TILE_ID})" ${TOOLS}/vep-config.make | sed -e 's/#.*$$//g' -e 's+../../vep \([0-9]\) in \(mem..\) have \(.*\) memory starting at \(.*\)$$+\2 from \4 range \3 for vep \1+g' -e 's/  / /g'  -e 's/K/ K/g' | sort | sed -e 's/ K/K/g' ; \
	rm -f $(TOOLS)/vep-config.make ; \
	fi
	@echo "-----"

$(SYS_LIBS_TARGETS):
	make -C $(SYSVEP)

$(TOOLS)/generate-json:
	make -C $(TOOLS)

$(LINKER_SCRIPT): $(TOOLS)/generate-json $(WORKSPACE)/vep-config.txt
	@rm -f vep-config.tmp
	@# add vep id to vep-config.txt
	grep -v -e '^[ 	]*#' -e '^[ 	]*$$' $(WORKSPACE)/vep-config.txt | sed "s/^/vep ${VEP_ID} /" > vep-config.tmp
	${MB_GCC} ${CFLAGS} -c $(SHAREDMEMC) -o $(SHAREDMEMO)
	@# remove empty lscript.ld if generate-jason failed
	$(TOOLS)/generate-json vep-config.tmp -ld $(VEP_ID) $(TILE_ID) $(PARTITION_ID) > $(LINKER_SCRIPT) || rm -f $(LINKER_SCRIPT) vep-config.tmp
	[ -f $(LINKER_SCRIPT) ]
	@rm -f vep-config.tmp

# TODO filter on C/C++ sources separate.
OBJECTS 	 := ${SOURCES:.c=.o}
OBJECTS 	 := ${OBJECTS:.s=.o}
OBJECTS 	 := ${OBJECTS:.S=.o}
DEPENDENCIES := ${SOURCES:.c=.d}

MB_GCC := ${PREFIX}-gcc
MB_OBJCOPY := ${PREFIX}-objcopy

realclean: clean

clean:
	-rm -f ${LINKER_SCRIPT} out.hex ${OBJECTS} ${DEPENDENCIES} ${TARGET} $(SHAREDMEMO)

%.d  %.o: %.c
	${MB_GCC} ${CFLAGS} -MMD -MP $< -c -o ${^:.c=.o}

ifeq (clean,$(MAKECMDGOALS))
else
ifeq (realclean,$(MAKECMDGOALS))
else
-include ${DEPENDENCIES}
endif
endif

# Create elf file
${TARGET}: ${LINKER_SCRIPT} $(HEADERS) ${OBJECTS} $(SYS_LIBS_TARGETS) $(WORKSPACE)/vep-config.txt 
	${MB_GCC} ${CFLAGS} -Wl,-T -Wl,${LINKER_SCRIPT}  -o $@ ${OBJECTS} $(SHAREDMEMO) \
        $(foreach lib,$(SYS_LIBS),-L$(SYSVEP)/lib$(lib)/lib/ -l$(lib)_$(TILE_NAME)) \
        $(foreach lib,$(USER_LIBS),-L$(WORKSPACE)/lib$(lib)/lib/ -l$(lib)_$(TILE_NAME)) $(USER_LINK_FLAGS)
	${MB_OBJCOPY} --remove-section='.publicmem' --remove-section='.mem*' -g -Obinary ${TARGET} out.hex

.PHONY: clean realclean all mm memmap mma memmap-all
