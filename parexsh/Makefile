include ../mk/c_c++rules.mk
include ../mk/noimplicit.mk
include ../mk/flags.mk

SRCS= \
  src/parexsh.cc \

CXXFLAGS+=-I../libparex/include -g
LDFLAGS+=-L$(CURDIR)/../libparex -Wl,-R$(CURDIR)/../libparex -lparex

OBJS=$(SRCS:.cc=.o)
DEPS=$(SRCS:.cc=.d)

parexsh: $(OBJS)
	$(SHOW.ld)
	$(LINK.ld)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) parexsh

sinclude $(DEPS)
