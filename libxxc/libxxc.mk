# ++C Runtime Library
# Licensed under the MIT License

XXC_SRC_NAMES := app argparse stream FileStream FSPath MemoryStream socket \
                 Map Runnable Thread ProcessStream Process
XXC_SRCS := $(addprefix libxxc/src/,$(addsuffix .ppc,$(XXC_SRC_NAMES)))
XXC_OBJS := $(call objs,$(XXC_SRCS))

LIBXXC_A  := $(BUILD)/libxxc.a
LIBXXC_SO := $(BUILD)/libxxc.so

$(LIBXXC_A): $(XXC_OBJS)
	@echo "  AR      $@"
	$(Q)rm -f $@
	$(Q)$(AR) rcs $@ $^

$(LIBXXC_SO): $(XXC_OBJS) $(LIBMINICRT_SO)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS_BASE) -shared -o $@ $(XXC_OBJS) -L$(BUILD) -lminicrt $(EXTRA_LIBS)

.PHONY: install-libs-libxxc install-dev-libxxc
install-libs-libxxc: $(LIBXXC_SO)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 755 $(LIBXXC_SO) $(DESTDIR)$(PREFIX)/lib/

install-dev-libxxc: $(LIBXXC_A)
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/xxc
	install -m 644 $(LIBXXC_A) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libxxc/include/*.pph $(DESTDIR)$(PREFIX)/include/xxc/
