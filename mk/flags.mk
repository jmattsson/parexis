
# Common stuff
CFLAGS_COMMON=-fPIC
CXXFLAGS_COMMON=$(CFLAGS_COMMON) -fuse-cxa-atexit -fvisibility-inlines-hidden -std=gnu++0x

# Warnings (see http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
CFLAGS_WARN=-Wall -Wextra -Wformat-y2k -Wformat-security -Wformat-nonliteral -Winit-self -Wswitch-default -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op
CXXFLAGS_WARN=$(CFLAGS_WARN) -Weffc++ -Woverloaded-virtual

# Optimisations
CFLAGS_OPT=-O3
CXXFLAGS_OPT=$(CFLAGS_OPT)

# Includes
CFLAGS_INCLUDE=-I./include
CXXFLAGS_INCLUDE=$(CFLAGS_INCLUDE)

# It all comes together in the ACTUAL_xxx variables, which are the ones
# referenced by the build rules. No direct editing past this point unless to
# include new categories :)
ACTUAL_CFLAGS=$(CFLAGS_COMMON) $(CFLAGS_WARN) $(CFLAGS_OPT) $(CFLAGS) $(CFLAGS_INCLUDE)
ACTUAL_CXXFLAGS=$(CXXFLAGS_COMMON) $(CXXFLAGS_WARN) $(CXXFLAGS_OPT) $(CXXFLAGS) $(CXXFLAGS_INCLUDE)
ACTUAL_LDFLAGS=$(LDFLAGS)
