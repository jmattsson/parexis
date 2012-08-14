include ../mk/noimplicit.mk
include ../mk/flags.mk
include ../mk/c_c++rules.mk

SRCS= \
	src/PXChannel.cc \
	src/PXDriver.cc \
	src/PXPrinter.cc \
	src/PXIO.cc \
	src/PXFileIO.cc \
	src/PXSerialIO.cc \
	src/PXProcessIO.cc \
	src/PXInterleavedPrinter.cc \

OBJS=$(SRCS:.cc=.o)
DEPS=$(SRCS:.cc=.d)

CXXFLAGS+=-g
LDFLAGS+=-lpcre

libparex.so: $(OBJS)
	$(SHOW.so)
	$(LINK.so)

PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) libparex.so

sinclude $(DEPS)
