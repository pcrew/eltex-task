
.PHONY: clean cleanall

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -fPIC

UTILS = $(TOPDIR)/utils/logging.o

all: $(OBJECTS)

clean:
	@rm -fv $(OBJECTS)
cleanall:
	@rm -fv $(OBJECTS)
