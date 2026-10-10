# ++C Runtime Library
# Licensed under the MIT License

CRT_SRC_NAMES := alloc atexit str mem sig sock mutex thread cond sem clock \
                 dstr iarr parr map format cio path proc env
CRT_SRCS := $(addprefix libminicrt/src/,$(addsuffix .c,$(CRT_SRC_NAMES)))
CRT1_SRC := libminicrt/src/crt1.c

LIBMINICRT_OBJS := $(call objs,$(CRT_SRCS) $(CRT1_SRC)) $(PLAT_OBJS)

LIBMINICRT_A  := $(BUILD)/libminicrt.a
LIBMINICRT_SO := $(BUILD)/libminicrt.so

$(LIBMINICRT_A): $(LIBMINICRT_OBJS)
	@echo "  AR      $@"
	$(Q)rm -f $@
	$(Q)$(AR) rcs $@ $^

$(LIBMINICRT_SO): $(LIBMINICRT_OBJS)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS_BASE) -shared -o $@ $^ $(EXTRA_LIBS)

.PHONY: install-libs-libminicrt install-dev-libminicrt
install-libs-libminicrt: $(LIBMINICRT_SO)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 755 $(LIBMINICRT_SO) $(DESTDIR)$(PREFIX)/lib/

install-dev-libminicrt: $(LIBMINICRT_A)
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/minicrt
	install -m 644 $(LIBMINICRT_A) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libminicrt/include/*.h $(DESTDIR)$(PREFIX)/include/minicrt/
