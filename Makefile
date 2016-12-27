#shell 函数运行pwd命令获取当前文件所在的目录，并返回给PWD常量
PWD = $(shell pwd)

#编译相关的常量定义
#CROSS_COMPILE=arm-none-linux-gnueabi-
CROSS_COMPILE=/opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar 
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip
CXX=$(CROSS_COMPILE)g++
export CROSS_COMPILE CC AR LD

ENNLIB = $(PWD)/lib

#输出目录变量
OUTPUTDIR=$(PWD)/output
OUTOBJDIR=$(OUTPUTDIR)/obj
OUTBINDIR=$(OUTPUTDIR)/bin
OUTLIBDIR=$(OUTPUTDIR)/lib

#安装目录常量定义
#INSTALLDIR=/home/ives/freescale/dataconcentrator/rootfs_qt/rootfs/home
#UBISDIR=/home/ives/freescale/dataconcentrator/rootfs_qt/rootfs

#INSTALLDIR=/home/wanglongchang/freescale/dataconcentrator/rootfs/home
#UBISDIR=/home/wanglongchang/freescale/dataconcentrator/rootfs

INSTALLDIR=/work/imx6/rootfs/home
UBISDIR=/work/imx6/rootfs/home

#jzq软件输出目录，CFG_FLAGS是gcc编译参数
ENNOBJ = $(OUTBINDIR)/ennjzq
CFG_FLAGS+= -g -D_GNU_SOURCE


# Users can override those variables from the command line.
CFG_FLAGS  +=  -Wall -fno-strict-aliasing -std=gnu99 -g -O2  
CFG_FLAGS  +=  -I./modbus/include -I./102dpa/include -I./driver/include -I./protocol/include -I/usr/local/sqlite/include -I./bacnet/include

#库目录地址常量   生成的库放到ENNLIB定义的目录路下面
ENNLIBS = $(ENNLIB)/libapp.a \
					$(ENNLIB)/libdriver.a \
					$(ENNLIB)/lib102.a \
					$(ENNLIB)/libprotocol.a	\
					$(ENNLIB)/libbacnet.a 
					
					
COMPILE = $(CC) $(CFG_FLAGS) -Wall -O2 -fPIC -o "$(OUTOBJDIR)/$(*F).o" -c "$<"
#COMPILE = $(CC) $(CFG_FLAGS) -Wall -fPIC -o "$(OUTOBJDIR)/$(*F).o" -c "$<"

#当前目录下的子目录的常量定义，在这个地方进行扩展，独立模块放到一个目录下面
DRIVER = ./driver
MODBUS = ./modbus
102DPA = ./102dpa
PROTOCOL = ./protocol
#wanglongchang  added
BACNET = ./bacnet

#把模块的目录赋值给 MODULES 模块 wanglongchang added  $(BACNET)
MODULES = $(MODBUS) $(102DPA) $(DRIVER) $(PROTOCOL) $(BACNET)
#MODULES = $(MODBUS)

CLEANMODULE = $(addsuffix _RM, $(MODULES))

#加入src目录下的makefile文件，编译的时候会在此处展开
include $(PWD)/src/Makefile

all:$(MODULES) $(OUTOBJDIR) $(ENNAPP_OBJ)
#$(EXEC):$(MODULES) $(OUTOBJDIR) $(ENNAPP_OBJ)
#	@echo $(ENNLIB)
#	@echo $(ENNLIBS)
#	$(CC) $(CFG_FLAGS) $(ENNAPP_OBJ) $(ENNLIBS) -o $(ENNOBJ) -L/usr/local/sqlite/lib -lsqlite3 -lpthread -lrt -Wl -lm --hash-style=sysv

	$(CC) $(CFG_FLAGS) $(ENNAPP_OBJ) $(ENNLIBS) -o $(ENNOBJ) -L/usr/local/sqlite/lib -lsqlite3 -lpthread -lrt -Wl -lm
#	$(CC) $(CFG_FLAGS) -lpthread -lrt -Wl -lm -o $(ENNOBJ) $(ENNAPP_OBJ) $(ENNLIBS) 
#	$(STRIP) $(CYHI)

#生成各种目录
$(OUTOBJDIR):
	@echo make dir in outbojdir...
	mkdir -p "$(OUTPUTDIR)"
	mkdir -p "$(OUTLIBDIR)"
	mkdir -p "$(OUTOBJDIR)"
	mkdir -p "$(OUTBINDIR)"

clean:$(CLEANMODULE) 
	@rm -rf $(OUTPUTDIR)
.PHONY: all clean $(MODULES) $(CLEANMODULE)

#进入各个子目录执行相应的makefile文件
$(MODULES):
	@echo now jump into next directory....
	$(MAKE) --directory=$@
	
$(CLEANMODULE):
	$(MAKE) --directory=$(subst _RM, ,$@) clean

install:
#rm -fr ../img/hicyhaisiapp
#cp $(OUTPUTDIR)/bin/hicyhaisiapp ../img/ 
	cp -rf ./output/bin/ennjzq $(INSTALLDIR)
	chmod 777 $(INSTALLDIR)/ennjzq
rom:
#	mkfs.jffs2 -e 16KiB -r ./output/bin -o ../cystapp.jffs2 -n
	mkfs.ubifs -r $(UBISDIR) -m 2048 -e 129024 -c 400 -o rootfs.ubifs
