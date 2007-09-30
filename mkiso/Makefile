CFLAGS ?= -Wall -Werror -g -O2 -pipe
LDFLAGS += -lpacman
CFLAGS += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)
CFLAGS += $(shell pkg-config --cflags libxml-2.0)
LDFLAGS += $(shell pkg-config --libs libxml-2.0)

OBJECTS = mkiso.o xml.o boot.o menu.o

mkiso: $(OBJECTS)

clean:
	rm -f $(OBJECTS) mkiso
