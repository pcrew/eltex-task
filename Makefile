
TOPDIR := $(shell /bin/pwd)
INCDIR := $(TOPDIR)/include

CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
#CFLAGS += -Wall -Os -I$(INCDIR) -D_FILE_OFFSET_BITS=64
CFLAGS += -Wall -Os -I$(INCDIR) -g -D_FILE_OFFSET_BITS=64 

export CC STRIP CFLAGS TOPDIR INCDIR

all:
	@$(MAKE) -C utils
	@$(MAKE) -C api 
	@$(MAKE) -C reactor
	@$(MAKE) -C programs

clean:
	@rm -v `find . -name *.o`
	@rm -v 'find . -name *.sw?'
	@rm -v 'find . -name *.sw?'

cleanall:
	@rm -vf `find . -name *.o`
	@rm -vf `find . -name *.so`
	@rm -vf `find . -name *.sw?`
	@rm -vf `find . -name *.sw?`
	@rm -vf *.deb

	@$(MAKE) cleanall -C utils
	@$(MAKE) cleanall -C api
	@$(MAKE) cleanall -C reactor
	@$(MAKE) cleanall -C programs
