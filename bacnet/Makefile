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

# configuration
# If BACNET_DEFINES has not already been set, configure to your needs here
MY_BACNET_DEFINES = -DPRINT_ENABLED=1
MY_BACNET_DEFINES += -DBACAPP_ALL
MY_BACNET_DEFINES += -DBACFILE
MY_BACNET_DEFINES += -DINTRINSIC_REPORTING
MY_BACNET_DEFINES += -DBACNET_PROPERTY_LISTS=1
BACNET_DEFINES ?= $(MY_BACNET_DEFINES)

#BACDL_DEFINE=-DBACDL_ETHERNET=1
#BACDL_DEFINE=-DBACDL_ARCNET=1
BACDL_DEFINE = -DBACDL_MSTP=1
BACDL_DEFINE += -DBACDL_BIP=1

# Declare your level of BBMD support
BBMD_DEFINE ?=-DBBMD_ENABLED=1
#BBMD_DEFINE ?= -DBBMD_ENABLED=0
#BBMD_DEFINE ?= -DBBMD_CLIENT_ENABLED

# Passing parameters via command line
MAKE_DEFINE ?=

# Define WEAK_FUNC for [...somebody help here; I can't find any uses of it]
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE) $(BBMD_DEFINE) -DWEAK_FUNC=
DEFINES += $(MAKE_DEFINE)

# BACnet Ports Directory
BACNET_PORT ?= linux

# Default compiler settings
OPTIMIZATION = -Os
DEBUGGING =
WARNINGS = -Wall -Wmissing-prototypes
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
DEFINES += -DBIP_DEBUG
endif
endif
#CFLAGS  = $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(STANDARDS) $(INCLUDES) $(DEFINES)

BACNET_SERVER = ./server
BACNET_CLIENT = ./client
BACNET_PORT_DIR = ./linux
BACNET_OBJECT = ./object
BACNET_HANDLER = ./handler
BACNET_CORE = ./src
BACNET_INCLUDE = ./include
# compiler configuration
#STANDARDS = -std=c99
INCLUDE1 = -I$(BACNET_PORT_DIR) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER)
INCLUDE2 = -I$(BACNET_INCLUDE)
INCLUDES = $(INCLUDE1) $(INCLUDE2)
INCLUDES += -I../modbus/include -I../driver/include -I../protocol/include -I../102dpa/include
LIBNAME = ../lib/libbacnet.a
# target
TARGET = bacnet
LIBRARY = ../lib/lib$(TARGET).a

CORE_SRC = \
	$(BACNET_CORE)/apdu.c \
	$(BACNET_CORE)/npdu.c \
	$(BACNET_CORE)/bacdcode.c \
	$(BACNET_CORE)/bacint.c \
	$(BACNET_CORE)/bacreal.c \
	$(BACNET_CORE)/bacstr.c \
	$(BACNET_CORE)/bacapp.c \
	$(BACNET_CORE)/bacprop.c \
	$(BACNET_CORE)/bactext.c \
	$(BACNET_CORE)/bactimevalue.c \
	$(BACNET_CORE)/datetime.c \
	$(BACNET_CORE)/indtext.c \
	$(BACNET_CORE)/key.c \
	$(BACNET_CORE)/keylist.c \
	$(BACNET_CORE)/proplist.c \
	$(BACNET_CORE)/debug.c \
	$(BACNET_CORE)/bigend.c \
	$(BACNET_CORE)/arf.c \
	$(BACNET_CORE)/awf.c \
	$(BACNET_CORE)/cov.c \
	$(BACNET_CORE)/dcc.c \
	$(BACNET_CORE)/iam.c \
	$(BACNET_CORE)/ihave.c \
	$(BACNET_CORE)/rd.c \
	$(BACNET_CORE)/rp.c \
	$(BACNET_CORE)/rpm.c \
	$(BACNET_CORE)/timesync.c \
	$(BACNET_CORE)/whohas.c \
	$(BACNET_CORE)/whois.c \
	$(BACNET_CORE)/wp.c \
	$(BACNET_CORE)/wpm.c \
	$(BACNET_CORE)/abort.c \
	$(BACNET_CORE)/reject.c \
	$(BACNET_CORE)/bacerror.c \
	$(BACNET_CORE)/ptransfer.c \
	$(BACNET_CORE)/memcopy.c \
	$(BACNET_CORE)/filename.c \
	$(BACNET_CORE)/tsm.c \
	$(BACNET_CORE)/bacaddr.c \
	$(BACNET_CORE)/address.c \
	$(BACNET_CORE)/bacdevobjpropref.c \
	$(BACNET_CORE)/bacpropstates.c \
	$(BACNET_CORE)/alarm_ack.c \
	$(BACNET_CORE)/event.c \
	$(BACNET_CORE)/getevent.c \
	$(BACNET_CORE)/get_alarm_sum.c \
	$(BACNET_CORE)/readrange.c \
	$(BACNET_CORE)/timestamp.c \
	$(BACNET_CORE)/version.c

HANDLER_SRC = \
	$(BACNET_HANDLER)/dlenv.c \
	$(BACNET_HANDLER)/txbuf.c \
	$(BACNET_HANDLER)/noserv.c \
	$(BACNET_HANDLER)/h_npdu.c \
	$(BACNET_HANDLER)/h_whois.c \
	$(BACNET_HANDLER)/h_iam.c  \
	$(BACNET_HANDLER)/h_rp.c \
	$(BACNET_HANDLER)/h_rp_a.c \
	$(BACNET_HANDLER)/h_rpm.c \
	$(BACNET_HANDLER)/h_rpm_a.c \
	$(BACNET_HANDLER)/h_rr.c \
	$(BACNET_HANDLER)/h_wp.c  \
	$(BACNET_HANDLER)/h_wpm.c \
	$(BACNET_HANDLER)/h_alarm_ack.c  \
	$(BACNET_HANDLER)/h_arf.c  \
	$(BACNET_HANDLER)/h_arf_a.c  \
	$(BACNET_HANDLER)/h_awf.c  \
	$(BACNET_HANDLER)/h_rd.c  \
	$(BACNET_HANDLER)/h_dcc.c  \
	$(BACNET_HANDLER)/h_ts.c  \
	$(BACNET_HANDLER)/h_whohas.c  \
	$(BACNET_HANDLER)/h_ihave.c  \
	$(BACNET_HANDLER)/h_cov.c  \
	$(BACNET_HANDLER)/h_ccov.c  \
	$(BACNET_HANDLER)/h_ucov.c  \
	$(BACNET_HANDLER)/h_getevent.c  \
	$(BACNET_HANDLER)/h_gas_a.c  \
	$(BACNET_HANDLER)/h_get_alarm_sum.c  \
	$(BACNET_HANDLER)/h_pt.c  \
	$(BACNET_HANDLER)/h_pt_a.c  \
	$(BACNET_HANDLER)/h_upt.c  \
	$(BACNET_HANDLER)/s_arfs.c \
	$(BACNET_HANDLER)/s_awfs.c \
	$(BACNET_HANDLER)/s_dcc.c \
	$(BACNET_HANDLER)/s_ihave.c \
	$(BACNET_HANDLER)/s_iam.c  \
	$(BACNET_HANDLER)/s_cov.c  \
	$(BACNET_HANDLER)/s_ptransfer.c \
	$(BACNET_HANDLER)/s_rd.c \
	$(BACNET_HANDLER)/s_rp.c  \
	$(BACNET_HANDLER)/s_rpm.c  \
	$(BACNET_HANDLER)/s_ts.c \
	$(BACNET_HANDLER)/s_cevent.c  \
	$(BACNET_HANDLER)/s_router.c  \
	$(BACNET_HANDLER)/s_uevent.c  \
	$(BACNET_HANDLER)/s_whohas.c \
	$(BACNET_HANDLER)/s_whois.c  \
	$(BACNET_HANDLER)/s_wpm.c  \
	$(BACNET_HANDLER)/s_upt.c \
	$(BACNET_HANDLER)/s_wp.c
	
OBJECT_SRC = \
	$(BACNET_OBJECT)/device.c \
	$(BACNET_OBJECT)/ai.c \
	$(BACNET_OBJECT)/ao.c \
	$(BACNET_OBJECT)/av.c \
	$(BACNET_OBJECT)/bi.c \
	$(BACNET_OBJECT)/bo.c \
	$(BACNET_OBJECT)/bv.c \
	$(BACNET_OBJECT)/csv.c \
	$(BACNET_OBJECT)/lc.c \
	$(BACNET_OBJECT)/lsp.c \
	$(BACNET_OBJECT)/ms-input.c \
	$(BACNET_OBJECT)/mso.c \
	$(BACNET_OBJECT)/msv.c \
	$(BACNET_OBJECT)/piv.c \
	$(BACNET_OBJECT)/nc.c  \
	$(BACNET_OBJECT)/trendlog.c \
	$(BACNET_OBJECT)/schedule.c \
	$(BACNET_OBJECT)/bacfile.c \
	$(BACNET_OBJECT)/device-client.c

SERVER_SRC = $(BACNET_SERVER)/server.c
CLIENT_SRC = $(BACNET_CLIENT)/client.c

PORT_ARCNET_SRC = \
	$(BACNET_PORT_DIR)/arcnet.c

PORT_MSTP_SRC = \
	$(BACNET_PORT_DIR)/rs485.c \
	$(BACNET_PORT_DIR)/dlmstp.c \
	$(BACNET_PORT_DIR)/timer.c \
	$(BACNET_CORE)/ringbuf.c \
	$(BACNET_CORE)/fifo.c \
	$(BACNET_CORE)/mstp.c \
	$(BACNET_CORE)/mstptext.c \
	$(BACNET_CORE)/crc.c \

PORT_ETHERNET_SRC = \
	$(BACNET_PORT_DIR)/ethernet.c

PORT_BIP_SRC = \
	$(BACNET_PORT_DIR)/bip-init.c \
	$(BACNET_CORE)/bvlc.c \
	$(BACNET_CORE)/bip.c

PORT_ALL_SRC = \
	$(BACNET_PORT_DIR)/arcnet.c \
	$(BACNET_PORT_DIR)/dlmstp.c \
	$(BACNET_PORT_DIR)/rs485.c \
	$(BACNET_PORT_DIR)/timer.c \
	$(BACNET_CORE)/ringbuf.c \
	$(BACNET_CORE)/fifo.c \
	$(BACNET_CORE)/mstp.c \
	$(BACNET_CORE)/crc.c \
	$(BACNET_PORT_DIR)/ethernet.c \
	$(BACNET_PORT_DIR)/bip-init.c \
	$(BACNET_CORE)/bvlc.c \
	$(BACNET_CORE)/bip.c

#ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
#PORT_SRC = ${PORT_BIP_SRC}
#endif
#ifeq (${BACDL_DEFINE},-DBACDL_MSTP=1)
#PORT_SRC = ${PORT_MSTP_SRC}
#endif
#ifeq (${BACDL_DEFINE},-DBACDL_ARCNET=1)
#PORT_SRC = ${PORT_ARCNET_SRC}
#endif
#ifeq (${BACDL_DEFINE},-DBACDL_ETHERNET=1)
#PORT_SRC = ${PORT_ETHERNET_SRC}
#endif
#ifdef BACDL_ALL
#PORT_SRC = ${PORT_ALL_SRC}
#endif

SRCS = ${CORE_SRC} ${HANDLER_SRC} ${OBJECT_SRC} ${PORT_BIP_SRC} ${PORT_MSTP_SRC} ${SERVER_SRC} ${CLIENT_SRC}

OBJS = ${SRCS:.c=.o}

# use local includes, but other values from calling Makefile
#CFLAGS  = $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(STANDARDS) $(INCLUDES) $(DEFINES)
	
all: $(LIBRARY)

lib: $(LIBRARY)

$(LIBRARY): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)

.c.o:
	${CC} -c $(CFLAGS) $(INCLUDES) $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	@echo Deleting files ...
	@rm -rf core $(OBJS) 
	@rm -f $(LIBRARY)

include: .depend
