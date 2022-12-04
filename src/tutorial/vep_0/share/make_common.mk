WORKSPACE:=..
SYSWORKSPACE:=$(WORKSPACE)/..
TOOLS:=$(SYSWORKSPACE)/tools
SYSVEP:=$(SYSWORKSPACE)/vep_0
SHAREDMEM:=$(WORKSPACE)/shared_memories

UPDATEMEM_BIN:=updatemem

# VEP_ID, TILE_ID, PARTITION_ID are also #defined for use in the C code
VEP_ID := $(shell pwd | sed -e 's/.*\/vep_\([0-9][0-9]*\)\/.*/\1/g' )
TILE_ID := $(shell pwd | sed -e 's/.*partition_\([0-9][0-9]*\)_[0-9][0-9]*/\1/g' )
PARTITION_ID := $(shell pwd | sed -e "s/.*partition_[0-9]_//g" )
SOURCES := $(wildcard *.c)
HEADERS := $(wildcard $(SHAREDMEM)/*.h) $(wildcard *.h)

-include ${SYSVEP}/tiles/tiles.mk
