WORKSPACE:=..
SYSWORKSPACE:=$(WORKSPACE)/..
SYSVEP:=$(SYSWORKSPACE)/vep_0

SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)

CFLAGS = -Wall -Wextra --pedantic -O2 -g3 -std=c99 -I.
CFLAGS += ${COMPILER_FLAGS}
CFLAGS += $(USER_CFLAGS)

# Library (libbsp for platform.h and cheap*.h)
# The order of the -l is important, do not change
CFLAGS += -I$(SYSVEP)/libbsp/include -I$(SYSVEP)/libchannel/include -I$(SYSVEP)/libuserchannel-arm/include
LDFLAGS += -L$(SYSVEP)/libchannel/lib -L$(SYSVEP)/libuserchannel-arm/lib
LDFLAGS += -luserchannel-arm -lchannel_arm
LDFLAGS += $(USER_LIBS)

##
# Default target
##
all: $(TARGET)

OBJECTS := ${SOURCES:.c=.o}
OBJECTS := ${OBJECTS:.s=.o}
OBJECTS := ${OBJECTS:.S=.o}
DEPENDENCIES := ${SOURCES:.c=.d}

GCC := gcc

%.d %.o: %.c
	${GCC} ${CFLAGS} $< -c -o ${^:.c=.o}

$(SYSVEP)/libchannel/lib/libchannel_arm.a:
	make -C $(SYSVEP)/libchannel

$(SYSVEP)/libuserchannel-arm/lib/libuserchannel-arm.a:
	make -C $(SYSVEP)/libuserchannel-arm

${TARGET}: $(OBJECTS) $(SYSVEP)/libchannel/lib/libchannel_arm.a $(SYSVEP)/libuserchannel-arm/lib/libuserchannel-arm.a
	${GCC} ${CFLAGS} -o $(TARGET) $(OBJECTS) $(LDFLAGS)

realclean: clean

clean:
	-rm -f ${OBJECTS} ${DEPENDENCIES} ${TARGET}

.PHONY: clean realclean all 
