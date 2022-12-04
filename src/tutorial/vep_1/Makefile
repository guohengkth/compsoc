USER_LIBS := $(wildcard lib*)
DIRS := $(wildcard app_* partition_*)
HEX_FILES:= $(foreach dir, $(DIRS),$(dir)/out.hex)

# all standard libraries are in WORKSPACE/../vep_0
# dependencies thereon are dealt with by the share/make_*.mk
all: $(foreach lib,$(USER_LIBS),make-$(lib)) $(HEX_FILES)

mm memmap: $(foreach dir,$(DIRS),memmap-$(dir))

mma memmap-all: $(foreach dir,$(DIRS),memmap-all-$(dir))

%/out.hex: %
	make -C $^

memmap-%: %
	make -C $^ memmap

memmap-all-%: %
	make -C $^ memmap-all

make-%: %
	make -C $^

clean veryclean: $(foreach dir, $(DIRS), clean-$(dir)) $(foreach lib,$(USER_LIBS),clean-$(lib))

clean-%: %
	make -C $^ clean

.PHONY: all clean veryclean mm memmap mma memmap-all
