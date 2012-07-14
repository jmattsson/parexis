# Build rules
#
# Inspired by the Dr Dobbs "Dependency Management" article from 2006, but
# fixed up to handle multi-line dependencies and always use wildcarding.
# In short, we generate the .d files as a side effect of actual compilation,
# and then post-process them into using the $(wildcard ...) make function to
# avoid make barfing when a source or header file is deleted. There is no
# explicit rule for generating .d files, which avoids make restarting itself.
#
# Remaining issue: .d files do not automatically disappear if their .c file
# is deleted. Suggestions on how to fix that are welcome.
#

# Some useful ANSI escape sequences for prettifying the output.
ANSI_BOLD =[1m
ANSI_UNDERLINE=[4m
ANSI_REVERSE=[7m
ANSI_BLACK=[30m
ANSI_RED=[31m
ANSI_GREEN=[32m
ANSI_YELLOW=[33m
ANSI_BLUE=[34m
ANSI_WHITE=[37m
ANSI_OFF=[0m

# By default we don't show command lines, but can be changed with 'make Q='
Q=@

# We normally want a condensed output of what's going on instead of the hugely
# long gcc lines.  For convenient reuse and consistency, we can use the
# $(SHOW.xx) variables defined here.
SHOW.c =@echo "$(ANSI_REVERSE)[   cc]$(ANSI_OFF) $@"
SHOW.cc=@echo "$(ANSI_REVERSE)[  c++]$(ANSI_OFF) $@"
SHOW.so=@echo "$(ANSI_REVERSE)[share]$(ANSI_OFF) $@"
SHOW.ld=@echo "$(ANSI_REVERSE)[ link]$(ANSI_OFF) $@"

# We reuse the normal COMPILE.c variable to mean slightly more than it normally
# does, and include the target and sources here. This should be fine considering
# we've got a fully custom makefile setup and don't use any built-in rules.
# We use -MMD instead of -MD since we have full control over the system headers
# and expect them to change only infrequently.
COMPILE.c =$Q$(CC) $(ACTUAL_CFLAGS) -MMD -c -o $@ $<
COMPILE.cc=$Q$(CXX) $(ACTUAL_CXXFLAGS) -MMD -c -o $@ $<

# Dependency fix-ups, arguably should be part of the COMPILE.x variables.
# Whether the sed should be $Q or @ is also debatable. It's ugly, so I prefer
# to keep it hidden.
# Essentially: find the target line (target.o: deps...) and inject "$(wildcard "
#  then for the last line also append a ")". This happily handles both the
#  simple case of everything on a single line, and also when dependencies span
#  span multiple lines.
DEPEND.c =@sed -i -e 's,$@[ :]*\(.*\),$@ : $$\(wildcard \1,g' -e '$$ s/\(.*\)/\1\)/' $*.d
DEPEND.cc=$(DEPEND.c)

# Linking of shared libs and binaries.
LINK.so=$Q$(CXX) $(ACTUAL_LDFLAGS) -shared -o $@ $^
LINK.ld=$Q$(CXX) $(ACTUAL_LDFLAGS) -o $@ $(filter-out %.so, $^)

# Pattern rules for C, C++, shared libs (and .executables)
.PRECIOUS: %.o
%.o: %.c
	$(SHOW.c)
	$(COMPILE.c)
	$(DEPEND.c)

%.o: %.cc
	$(SHOW.cc)
	$(COMPILE.cc)
	$(DEPEND.cc)

%.so:
	$(SHOW.so)
	$(LINK.so)
	$Qchmod -x $@

# This really is just a placeholder/example - I'm not seriously suggesting we
# name binaries with .exe on the end :)
%.exe:
	$(SHOW.ld)
	$(LINK.ld)
