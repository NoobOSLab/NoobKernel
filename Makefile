.PHONY: all clean user run debug test .FORCE docs

# 工具链设置
TOOL_PREFIX = riscv64-unknown-elf-
CC = $(TOOL_PREFIX)gcc
AS = $(TOOL_PREFIX)gcc
LD = $(TOOL_PREFIX)ld
OBJCOPY = $(TOOL_PREFIX)objcopy
OBJDUMP = $(TOOL_PREFIX)objdump
GDB = /opt/riscv-gdb/bin/$(TOOL_PREFIX)gdb
PY = python3
CP = cp
QEMU = qemu-system-riscv64

# 编译参数设置
ARCH ?= QEMU
LOG ?= INFO
INIT_PROC ?= usershell
SRC_DIR := src
BUILD_DIR := build
ARCH_DIR := $(BUILD_DIR)/$(ARCH)
INCLUDES := include/
MODULES := misc mm hal trap task sync async ipc fs
TARGET := $(ARCH_DIR)/kernel
ASM := $(ARCH_DIR)/kernel.asm
SYM := $(ARCH_DIR)/kernel.sym

C_FLAGS := -Wall -Werror -O -O0 -fno-omit-frame-pointer -ggdb -std=gnu11
C_FLAGS += -Wno-unused-variable -Wno-unused-function
C_FLAGS += -MMD -MP
C_FLAGS += -c
C_FLAGS += -mcmodel=medany
C_FLAGS += -ffreestanding -fno-common -nostdlib -mno-relax -nostdinc
C_FLAGS += -I $(INCLUDES)
C_FLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
C_FLAGS += -D $(ARCH)
C_FLAGS += -D LOG_LEVEL_$(LOG)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
C_FLAGS += -fno-pie -no-pie
endif

LD_FLAGS := -z max-page-size=4096
LD_FLAGS += -T scripts/kernel.ld

# 运行参数设置
GDB_PORT = 15234
QEMU_FLAGS = \
	-nographic \
	-machine virt \
	-m 512M \
	-kernel $(TARGET)
QEMUGDB = -gdb tcp::$(GDB_PORT)

# 源文件列表
C_SRCS := $(SRC_DIR)/main.c
AS_SRCS := $(SRC_DIR)/entry.S

define include_module
    LOCAL_C_SRCS :=
	LOCAL_AS_SRCS :=
	LOCAL_C_SRCS_$(ARCH) :=
	LOCAL_AS_SRCS_$(ARCH) :=
	$(eval include $(SRC_DIR)/$(1)/Makefile)
	$(eval LOCAL_C_SRCS += $(LOCAL_C_SRCS_$(ARCH)))
	$(eval LOCAL_AS_SRCS += $(LOCAL_AS_SRCS_$(ARCH)))
	C_SRCS += $(addprefix $(SRC_DIR)/$(1)/, $(LOCAL_C_SRCS))
	AS_SRCS += $(addprefix $(SRC_DIR)/$(1)/, $(LOCAL_AS_SRCS))
endef

$(foreach m,$(MODULES),$(eval $(call include_module,$(m))))

# 目标文件列表
C_OBJS := $(patsubst $(SRC_DIR)/%,$(ARCH_DIR)/%,$(C_SRCS:.c=.o))
AS_OBJS := $(patsubst $(SRC_DIR)/%,$(ARCH_DIR)/%,$(AS_SRCS:.S=.o))

# 依赖关系文件列表
DEPS := $(C_OBJS:.o=.d) $(AS_OBJS:.o=.d)

# 空目标
.FORCE:

all: $(TARGET) $(ASM) $(SYM)

-include $(DEPS)

$(TARGET): $(AS_OBJS) $(C_OBJS)
	@echo "LD      $@"
	@$(LD) $^ -o $@ $(LD_FLAGS)

$(ARCH_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "CC      $<"
	@$(CC) $(C_FLAGS) $< -o $@

$(ARCH_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	@echo "AS      $<"
	@$(AS) $(C_FLAGS) $< -o $@

$(ASM): $(TARGET)
	@echo "OBJDUMP $@"
	@$(OBJDUMP) -S $(TARGET) > $(ASM)

$(SYM): $(TARGET)
	@echo "OBJDUMP $@"
	@$(OBJDUMP) -t $(TARGET) | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(SYM)

clean:
	@echo "CLEAN   build"
	@rm -rf $(BUILD_DIR)

run: $(TARGET)
	@echo "RUN     QEMU (virt)"
	@$(QEMU) $(QEMU_FLAGS)

debug: $(TARGET)
	@echo "DEBUG   QEMU (gdb)"
	$(QEMU) $(QEMU_FLAGS) -S $(QEMUGDB) &
	@while ! nc -zv localhost $(GDB_PORT) 2>/dev/null; do sleep 0.5; done
	@echo "GDB     connecting..."
	@$(GDB)
