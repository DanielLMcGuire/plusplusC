###################################################################
# targets:
#   make              build libminicrt.a, libxxc.a, tests and demos
#   make libs         static libraries only
#   make shared       also build libminicrt.so and libxxc.so
#   make test         build and run the test executables
#   make demo         build the demo executables (app, socket)
#   make install      install everything (PREFIX, DESTDIR)
#   make install-libs shared libraries only
#   make install-dev  static libraries, crt0.o and headers only
#   make clean
#
# variables:
#   CC=clang          compiler (default: cc)
#   BUILD=build       output directory
#   LTO=0             disable -flto
#   USE_SHARED=1      link tests/demos against the shared libraries
#   PREFIX=/usr/local install prefix
#   DESTDIR=          staging root for packaging
#   V=1               show full compiler commands

CC         ?= cc
BUILD      ?= build
PREFIX     ?= /usr/local
DESTDIR    ?=
LTO        ?= 1
USE_SHARED ?= 0
V          ?= 0

include mk/config.mk
include mk/rules.mk
include platform/platform.mk
include libminicrt/libminicrt.mk
include libxxc/libxxc.mk

TESTS = test leak_test
DEMOS = app socket

test_SRC      = test/test.ppc
leak_test_SRC = test/leak.ppc
app_SRC       = examples/app-demo.ppc
socket_SRC    = examples/socket-demo.ppc

PROGRAMS   = $(TESTS) $(DEMOS)
EXEC_STAMP = $(BUILD)/.exec-config

ifeq ($(USE_SHARED),1)
  EXEC_DEFS     = -DXXC_USING_SHARED_LIB
  EXEC_LIBS     = -L$(BUILD) -lxxc -lminicrt $(EXTRA_LIBS) -Wl,-rpath,'$$ORIGIN'
  EXEC_LINK     = $(LDFLAGS_BASE)
  EXEC_DEPS     = $(LIBMINICRT_SO) $(LIBXXC_SO)
  SHARED_WANTED = 1
else
  EXEC_DEFS     =
  EXEC_LIBS     = -Wl,--start-group $(LIBXXC_A) $(LIBMINICRT_A) -Wl,--end-group $(EXTRA_LIBS)
  EXEC_LINK     = $(LDFLAGS_BASE) -static -no-pie
  EXEC_DEPS     = $(LIBMINICRT_A) $(LIBXXC_A)
endif

$(shell mkdir -p $(BUILD); echo "USE_SHARED=$(USE_SHARED) LTO=$(LTO) CC=$(CC)" | cmp -s - $(EXEC_STAMP) || echo "USE_SHARED=$(USE_SHARED) LTO=$(LTO) CC=$(CC)" > $(EXEC_STAMP))

define PROGRAM_RULE
$$(BUILD)/prog/$(1).o: $$($(1)_SRC) $$(EXEC_STAMP)
	@mkdir -p $$(dir $$@)
	@echo "  CC      $$<"
	$$(Q)$$(CC) $$(CFLAGS_ALL) $$(EXEC_DEFS) -x c -MMD -MP -c $$< -o $$@

$$(BUILD)/$(1): $$(BUILD)/prog/$(1).o $$(CRT0_OBJ) $$(EXEC_DEPS) $$(EXEC_STAMP)
	@echo "  LINK    $$@"
	$$(Q)$$(CC) $$(EXEC_LINK) -o $$@ $$(BUILD)/prog/$(1).o $$(CRT0_OBJ) $$(EXEC_LIBS)
endef
$(foreach p,$(PROGRAMS),$(eval $(call PROGRAM_RULE,$(p))))

.PHONY: all libs shared tests demo test check install install-libs install-dev uninstall clean help
.DEFAULT_GOAL := all

all: libs $(if $(SHARED_WANTED),shared) tests demo

libs: $(LIBMINICRT_A) $(LIBXXC_A) $(CRT0_OBJ)

shared: $(LIBMINICRT_SO) $(LIBXXC_SO)

tests: $(addprefix $(BUILD)/,$(TESTS))

demo: $(addprefix $(BUILD)/,$(DEMOS))

test check: tests
	$(Q)set -e; for t in $(TESTS); do \
	    echo "==> $$t"; ./$(BUILD)/$$t; \
	done

install: install-libs install-dev

install-libs: install-libs-libminicrt install-libs-libxxc

install-dev: install-dev-platform install-dev-libminicrt install-dev-libxxc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libminicrt.a $(DESTDIR)$(PREFIX)/lib/libxxc.a \
	      $(DESTDIR)$(PREFIX)/lib/libminicrt.so $(DESTDIR)$(PREFIX)/lib/libxxc.so
	rm -rf $(DESTDIR)$(PREFIX)/lib/minicrt $(DESTDIR)$(PREFIX)/include/minicrt \
	       $(DESTDIR)$(PREFIX)/include/xxc

clean:
	rm -rf $(BUILD)

help:
	@sed -n '2,/^$$/p' Makefile | sed 's/^# \{0,1\}//'

$(BUILD)/%.d: ;
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
