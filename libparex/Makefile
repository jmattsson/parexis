include ../mk/noimplicit.mk
include ../mk/flags.mk
include ../mk/c_c++rules.mk

SRCS= \
	src/PXChannel.cc \
	src/PXDriver.cc \
	src/PXIO.cc \
	src/PXFileIO.cc \

OBJS=$(SRCS:.cc=.o)
DEPS=$(SRCS:.cc=.d)

LDFLAGS+=-lpcre

libparex.so: $(OBJS)
	$(SHOW.so)
	$(LINK.so)

PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) libparex.so

sinclude $(DEPS)
