LDFLAGS += -lalpm
CFLAGS ?= -g -Wall -Werror
CFLAGS += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)
CFLAGS += $(shell pkg-config --cflags libxml-2.0)
LDFLAGS += $(shell pkg-config --libs libxml-2.0)
mkiso: mkiso.o xml.o
