
.PHONY: clean cleanall

SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
PROGRAM = test-v1
LDFLAGS = -ldl -lm -lpthread

UTILS  =
UTILS += $(TOPDIR)/reactor/*.o
UTILS += $(TOPDIR)/utils/*.o
UTILS += $(TOPDIR)/api/*.o

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(UTILS)
	$(CC) -rdynamic $(OBJECTS) $(UTILS) $(ENGINE) -o $@ $(LDFLAGS)

clean:
	@rm -fv $(OBJECTS)

cleanall:
	@rm -fv $(OBJECTS) $(PROGRAM)
