# ++C Runtime Library
# Licensed under the MIT License

objs = $(patsubst %.ppc,$(BUILD)/obj/%.o,$(patsubst %.s,$(BUILD)/obj/%.o,$(patsubst %.c,$(BUILD)/obj/%.o,$(1))))

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
