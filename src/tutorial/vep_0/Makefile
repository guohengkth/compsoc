LIBS := bsp userchannel-arm userchannel-riscv
# libchannel is precompiled

all: $(foreach lib, $(LIBS), make-lib$(lib))

clean: $(foreach lib, $(LIBS), clean-lib$(lib))
	@# libbsp/libsrc is not cleaned by its makefile, somehow
	-rm -f libbsp/libsrc/*.o

veryclean: clean

make-%: %
	make -C $^

clean-%: %
	make -C $^ clean

.PHONY: all clean veryclean
