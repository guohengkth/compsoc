WORKSPACE:=../
PREFIX:=
LIBNAME:=userchannel-arm
SOURCES:=$(wildcard libsrc/*.[cS])
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS += -I${WORKSPACE}/libbsp/include/ -I$(WORKSPACE)/tiles -Iinclude/ -O2

all: lib/libuserchannel-arm.a(${OBJECTS})
lib/libuserchannel-arm.a(${OBJECTS}): lib 

lib:
	mkdir -p $@

clean:
	@rm -f lib/libuserchannel-arm.a

