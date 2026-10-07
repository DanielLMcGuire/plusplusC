PLAT_C_SRCS := platform/$(PLATDIR)/syscall.c platform/shared/tls.c platform/shared/alloc.c
PLAT_S_SRCS := platform/$(PLATDIR)/syscall_$(ARCH).s platform/$(PLATDIR)/thread_$(ARCH).s
PLAT_OBJS   := $(call objs,$(PLAT_C_SRCS) $(PLAT_S_SRCS))

CRT0_SRC := platform/$(PLATDIR)/crt0_$(ARCH).s
CRT0_OBJ := $(BUILD)/crt0.o

PLAT_HEADERS := platform/xxc_sys.h platform/sys_$(PLATDIR).h \
                platform/sys_alloc.h platform/sys_thread.h

$(CRT0_OBJ): $(CRT0_SRC)
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	$(Q)$(CC) $(ASFLAGS_ALL) -c $< -o $@

.PHONY: install-dev-platform
install-dev-platform: $(CRT0_OBJ)
	install -d $(DESTDIR)$(PREFIX)/lib/minicrt $(DESTDIR)$(PREFIX)/include/minicrt
	install -m 644 $(CRT0_OBJ) $(DESTDIR)$(PREFIX)/lib/minicrt/crt0.o
	install -m 644 $(PLAT_HEADERS) $(DESTDIR)$(PREFIX)/include/minicrt/
