# ++C Runtime Library
# Licensed under the MIT License

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
  ifeq ($(origin AR),default)
    AR := $(or $(shell command -v llvm-ar 2>/dev/null),ar)
  endif
else
  IS_CLANG := 0
  ifeq ($(origin AR),default)
    AR := $(or $(shell command -v gcc-ar 2>/dev/null),ar)
  endif
endif

STD ?= gnu2x
OPT ?= -O3

FREESTANDING = -ffreestanding -fno-builtin -fno-stack-protector -ftls-model=initial-exec
ifeq ($(ARCH),aarch64)
  FREESTANDING += -mno-outline-atomics
endif

CFLAGS_BASE  = -std=$(STD) $(OPT) -fPIC $(FREESTANDING) -Wall -Wno-unused-function
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

CFLAGS_ALL  = $(CFLAGS_BASE) $(INCLUDES) $(DEFS) $(CFLAGS)
ASFLAGS_ALL = -fPIC $(INCLUDES) $(ASFLAGS)

EXTRA_LIBS =
ifeq ($(PLATDIR),linux)
  ifneq ($(ARCH),x86_64)
    EXTRA_LIBS = -lgcc
  endif
else ifeq ($(ARCH),aarch64)
  BUILTINS_LIB ?= compiler_rt
  EXTRA_LIBS = -l$(BUILTINS_LIB)
endif
