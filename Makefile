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

CC      ?= cc
BUILD   ?= build
PREFIX  ?= /usr/local
DESTDIR ?=
LTO     ?= 1
USE_SHARED ?= 0
V       ?= 0

ifeq ($(V),0)
  Q = @
else
  Q =
endif

OS := $(shell uname -s)
ifeq ($(OS),Linux)
  PLATDIR := linux
else ifeq ($(OS),FreeBSD)
  PLATDIR := freebsd
else
  $(error ++C Makefile only supports Linux and FreeBSD (got: $(OS)); use CMake or Makefile.uefi)
endif

MACHINE := $(shell uname -m)
ifneq ($(filter x86_64 amd64 AMD64,$(MACHINE)),)
  ARCH := x86_64
else ifneq ($(filter aarch64 arm64,$(MACHINE)),)
  ARCH := aarch64
else ifneq ($(filter i386 i486 i586 i686,$(MACHINE)),)
  ARCH := i386
else
  $(error ++C: unsupported architecture: $(MACHINE))
endif

ifeq ($(PLATDIR)$(ARCH),freebsdi386)
  $(error ++C: FreeBSD supports x86_64 and aarch64 only)
endif

CC_VERSION := $(shell $(CC) --version 2>/dev/null)
ifneq ($(findstring clang,$(CC_VERSION)),)
  IS_CLANG := 1
  AR ?= llvm-ar
  ifeq ($(origin AR),default)
    AR := $(or $(shell command -v llvm-ar 2>/dev/null),ar)
  endif
else
  IS_CLANG := 0
  ifeq ($(origin AR),default)
    AR := $(or $(shell command -v gcc-ar 2>/dev/null),ar)
  endif
endif

STD      ?= gnu2x
OPT      ?= -O3

FREESTANDING = -ffreestanding -fno-builtin -fno-stack-protector -ftls-model=initial-exec
ifeq ($(ARCH),aarch64)
  FREESTANDING += -mno-outline-atomics
endif

CFLAGS_BASE = -std=$(STD) $(OPT) -fPIC $(FREESTANDING) -Wall -Wno-unused-function
LDFLAGS_BASE = $(OPT) -nostdlib -fno-builtin
ifeq ($(LTO),1)
  ifeq ($(IS_CLANG),1)
    LTO_FLAG = -flto
  else
    LTO_FLAG = -flto=auto
  endif
  CFLAGS_BASE  += $(LTO_FLAG)
  LDFLAGS_BASE += $(LTO_FLAG)
endif

INCLUDES = -Ilibminicrt/include -Ilibminicrt/internal -Ilibxxc/include -Iplatform
DEFS     = -D_CORECRT_BUILD

EXTRA_LIBS =
ifeq ($(PLATDIR),linux)
  ifneq ($(ARCH),x86_64)
    EXTRA_LIBS = -lgcc
  endif
else ifeq ($(ARCH),aarch64)
  BUILTINS_LIB ?= compiler_rt
  EXTRA_LIBS = -l$(BUILTINS_LIB)
endif

CFLAGS_ALL = $(CFLAGS_BASE) $(INCLUDES) $(DEFS) $(CFLAGS)
ASFLAGS_ALL   = -fPIC $(INCLUDES) $(ASFLAGS)


CRT_SRC_NAMES = alloc atexit str mem sig sock mutex thread cond sem clock \
                dstr iarr parr map format cio
XXC_SRC_NAMES = app argparse stream FileStream FSPath MemoryStream socket \
                Map Runnable Thread

PLAT_C_SRCS  = platform/$(PLATDIR)/syscall.c platform/shared/tls.c platform/shared/alloc.c
PLAT_S_SRCS  = platform/$(PLATDIR)/syscall_$(ARCH).s platform/$(PLATDIR)/thread_$(ARCH).s
CRT0_SRC     = platform/$(PLATDIR)/crt0_$(ARCH).s

CRT_SRCS  = $(addprefix libminicrt/src/,$(addsuffix .c,$(CRT_SRC_NAMES)))
CRT1_SRC  = libminicrt/src/crt1.c
XXC_SRCS  = $(addprefix libxxc/src/,$(addsuffix .ppc,$(XXC_SRC_NAMES)))

objs = $(patsubst %.ppc,$(BUILD)/obj/%.o,$(patsubst %.s,$(BUILD)/obj/%.o,$(patsubst %.c,$(BUILD)/obj/%.o,$(1))))

SYS_OBJS  = $(call objs,$(PLAT_C_SRCS) $(PLAT_S_SRCS))
CRT_OBJS  = $(call objs,$(CRT_SRCS))
XXC_OBJS  = $(call objs,$(XXC_SRCS))
CRT1_OBJ  = $(call objs,$(CRT1_SRC))
CRT0_OBJ         = $(BUILD)/crt0.o

LIBMINICRT_A  = $(BUILD)/libminicrt.a
LIBXXC_A      = $(BUILD)/libxxc.a
LIBMINICRT_SO = $(BUILD)/libminicrt.so
LIBXXC_SO     = $(BUILD)/libxxc.so

TESTS = test leak_test
DEMOS = app socket

test_SRC      = test/test.ppc
leak_test_SRC = test/leak.ppc
app_SRC       = examples/app-demo.ppc
socket_SRC    = examples/socket-demo.ppc

PROGRAMS = $(TESTS) $(DEMOS)
EXEC_STAMP = $(BUILD)/.exec-config

ifeq ($(USE_SHARED),1)
  EXEC_VARIANT = shared
  EXEC_DEFS    = -DXXC_USING_SHARED_LIB
  EXEC_LIBS    = -L$(BUILD) -lxxc -lminicrt $(EXTRA_LIBS) -Wl,-rpath,'$$ORIGIN'
  EXEC_LINK    = $(LDFLAGS_BASE)
  EXEC_DEPS    = $(LIBMINICRT_SO) $(LIBXXC_SO)
  SHARED_WANTED = 1
else
  EXEC_VARIANT = static
  EXEC_DEFS    =
  EXEC_LIBS    = -Wl,--start-group $(LIBXXC_A) $(LIBMINICRT_A) -Wl,--end-group $(EXTRA_LIBS)
  EXEC_LINK    = $(LDFLAGS_BASE) -static -no-pie
  EXEC_DEPS    = $(LIBMINICRT_A) $(LIBXXC_A)
endif

PROG_OBJ = $(BUILD)/prog/$(1).o
PROG_BIN = $(BUILD)/$(1)

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

help:
	@sed -n '2,/^$$/p' Makefile | sed 's/^# \{0,1\}//'

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS_ALL) -MMD -MP -c $< -o $@

$(BUILD)/obj/%.o: %.ppc
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS_ALL) -x c -MMD -MP -c $< -o $@

$(BUILD)/obj/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	$(Q)$(CC) $(ASFLAGS_ALL) -c $< -o $@

$(CRT0_OBJ): $(CRT0_SRC)
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	$(Q)$(CC) $(ASFLAGS_ALL) -c $< -o $@

$(LIBMINICRT_A): $(CRT_OBJS) $(SYS_OBJS) $(CRT1_OBJ)
	@echo "  AR      $@"
	$(Q)rm -f $@
	$(Q)$(AR) rcs $@ $^

$(LIBXXC_A): $(XXC_OBJS)
	@echo "  AR      $@"
	$(Q)rm -f $@
	$(Q)$(AR) rcs $@ $^

$(LIBMINICRT_SO): $(CRT_OBJS) $(SYS_OBJS) $(CRT1_OBJ)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS_BASE) -shared -o $@ $^ $(EXTRA_LIBS)

$(LIBXXC_SO): $(XXC_OBJS) $(LIBMINICRT_SO)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS_BASE) -shared -o $@ $(XXC_OBJS) -L$(BUILD) -lminicrt $(EXTRA_LIBS)

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

install: install-libs install-dev

install-libs: shared
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 755 $(LIBMINICRT_SO) $(LIBXXC_SO) $(DESTDIR)$(PREFIX)/lib/

install-dev: libs
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/lib/minicrt
	install -d $(DESTDIR)$(PREFIX)/include/minicrt $(DESTDIR)$(PREFIX)/include/xxc
	install -m 644 $(LIBMINICRT_A) $(LIBXXC_A) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(CRT0_OBJ) $(DESTDIR)$(PREFIX)/lib/minicrt/crt0.o
	install -m 644 libminicrt/include/*.h $(DESTDIR)$(PREFIX)/include/minicrt/
	install -m 644 platform/xxc_sys.h platform/sys_$(PLATDIR).h platform/sys_alloc.h \
	    platform/sys_thread.h $(DESTDIR)$(PREFIX)/include/minicrt/
	install -m 644 libxxc/include/*.pph $(DESTDIR)$(PREFIX)/include/xxc/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libminicrt.a $(DESTDIR)$(PREFIX)/lib/libxxc.a \
	      $(DESTDIR)$(PREFIX)/lib/libminicrt.so $(DESTDIR)$(PREFIX)/lib/libxxc.so
	rm -rf $(DESTDIR)$(PREFIX)/lib/minicrt $(DESTDIR)$(PREFIX)/include/minicrt \
	       $(DESTDIR)$(PREFIX)/include/xxc

clean:
	rm -rf $(BUILD)

$(BUILD)/%.d: ;
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
