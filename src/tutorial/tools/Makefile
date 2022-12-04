LDLIBS = -lchannel_arm
LDFLAGS = -L. -L../vep_0/libchannel/lib

CFLAGS = -Wall -Wextra -pedantic -O0 -g3 -std=c99 -I. -I../vep_0/libbsp/include

TOOLS:=channel_init channel_dump channel_readall
# to be updated: channel_readline channel_cheap_bridge

all: ${TOOLS} generate-json

generate-json: generate-json.c
	@# libbsp is compiled for RISC-V, we only want to pick up some constants (arrays)
	gcc $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o generate-json generate-json.c ../vep_0/libbsp/libsrc/platform.c

clean:
	rm -f ${TOOLS} generate-json
	@# removed by succesful runs of rerun.sh
	rm -f vep-config.txt vep-config.cmd vep-config.json state.json

veryclean: clean

.PHONY: all clean veryclean
