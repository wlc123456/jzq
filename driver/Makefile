PWD=$(shell pwd)

#CROSS_COMPILE=arm-none-linux-gnueabi-
CROSS_COMPILE=/opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar 
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip
CXX=$(CROSS_COMPILE)g++
export CROSS_COMPILE CC AR LD
RM = rm
ECHO = echo
ARFLAGS = -rcs

export $(RM) $(ECHO) $(CC) $(AR)


#include ./MakeInclude

INCLUDES = -I./include -I/usr/local/sqlite/include -I../protocol/include -I../102dpa/include -I../modbus/include -I../bacnet/include -I../bacnet/object
MODULEDIR = ./src
LIBNAME = ../lib/libdriver.a
VPATH = ../lib $(MODULEDIR)
SOURCE = $(wildcard $(addsuffix /*.c ,$(MODULEDIR)))
OBJECT = $(notdir $(subst .c,.o,$(SOURCE)))


.c.o:
	@echo Compiling $@ ...
	@echo Compiling $(INCLUDES) ...
	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
	
#$(OBJECT):%.o:%.c
#	@echo Compiling $@ ...
#	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@	
	
all:$(OBJECT)
	@$(AR) $(ARFLAGS) $(LIBNAME) $^


-include $(subst .o,.d,$(OBJECT))


clean :
	@echo Deleting files ...
	@rm -f *.o 
	@rm -f $(LIBNAME)



.PHONY : all clean $(SRC)

