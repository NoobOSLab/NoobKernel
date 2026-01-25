#pragma once

#include <hal/device_types.h>

#define PLIC_DEF(addr, len)                                      \
	const struct plic_raw PLIC = {addr, len};

#define VIRTIO_MMIO_DEF(no, addr, len, irqno)                                      \
	const struct virtio_mmio_raw VIRTIO_MMIO_##no = {addr, len, irqno};

#define UART_DEF(no, addr, len, irqno)                                      \
	const struct uart_raw UART_##no = {addr, len, irqno};