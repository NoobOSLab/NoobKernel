#include <mm/kalloc.h>
#include <mm/pm.h>
#include <mm/slab.h>
#include <mm/buddy.h>
#include <misc/string.h>
#include <misc/printf.h>
#include <misc/errno.h>

void *kalloc_page() {
	void *page = buddy_alloc(PAGE_SIZE);
	if (page != NULL) {
		return page;
	}
	page = page_alloc(0);
	return page;
}

int kfree_page(void *addr) {
	struct page *page = addr2page(addr);
	if (page == NULL)
		return -EINVAL;
	if (page->flags & PM_BUDDY)
		return buddy_free(addr);
	else
		return page_free(addr);
	;
}

void *kmalloc(size_t size) {
	if (size == 0) {
		return NULL; // 不允许分配0字节
	}
	if (size <= (1 << SLAB_MAX_ORDER)) {
		return slab_alloc(size);
	} else if (size <= (PAGE_SIZE << BUDDY_MAX_ORDER)) {
		if (size <= PAGE_SIZE) {
			size = PAGE_SIZE; // 对于小于等于一页的请求，分配一页
		}
		return buddy_alloc(
			size); // 对于大于一页的请求，使用buddy系统分配
	}
	return NULL; // 超过buddy系统最大分配限制
}

void *kzalloc(size_t size) {
	void *ptr = kmalloc(size);
	if (ptr) {
		memset(ptr, 0, size); // 清零分配的内存
	}
	return ptr;
}

void *kcalloc(size_t nitems, size_t size) {
	return kmalloc(nitems * size); // 简单实现，直接调用kmalloc
}

void *realloc(void *ptr, size_t size) {
	int ret = kfree(ptr);
	if (ret < 0) {
		return NULL; // 释放失败
	}
	return kmalloc(size); // 重新分配内存
}

int kfree(void *addr) {
	struct page *page = addr2page((void *)PAGE_ALIGN_DOWN((uintptr_t)addr));
	if (page == NULL)
		return -EINVAL;
	if (page->flags & PM_SLAB)
		return slab_free(addr);
	else if (page->flags & PM_BUDDY)
		return buddy_free(addr);
	else
		return -EINVAL;
}

char *alloc_str(const char *str) {
	if (str == NULL) {
		return NULL;
	}
	size_t len = strlen(str);
	char *new_str = kmalloc(len + 1);
	if (new_str == NULL) {
		return NULL;
	}
	strncpy(new_str, str, len + 1);
	return new_str;
}

void kalloc_test() {
	printf("开始内存分配器压力测试...\n");

// 测试1：多个页面的分配和释放
#define PAGE_COUNT 10
	void *pages[PAGE_COUNT];
	printf("\n1. 测试连续分配和释放%d个页面\n", PAGE_COUNT);
	for (int i = 0; i < PAGE_COUNT; i++) {
		pages[i] = kalloc_page();
		if (pages[i]) {
			printf("成功分配第%d个页面: %p\n", i + 1, pages[i]);
		} else {
			printf("分配第%d个页面失败\n", i + 1);
		}
	}
	for (int i = 0; i < PAGE_COUNT; i++) {
		if (pages[i]) {
			kfree_page(pages[i]);
			printf("释放第%d个页面: %p\n", i + 1, pages[i]);
		}
	}

	// 测试2：不同大小的内存分配
	printf("\n2. 测试不同大小的内存分配\n");
	static const size_t test_sizes[] = {
		16, // 小内存块
		256, // 中等内存块
		1024, // 1KB
		4096, // 一页大小
		4097, // 略大于一页
		8192, // 2页
		1024 * 1024 // 1MB
	};
	void *ptrs[ARRAY_SIZE(test_sizes)] = { NULL };

	for (size_t i = 0; i < ARRAY_SIZE(test_sizes); i++) {
		ptrs[i] = kmalloc(test_sizes[i]);
		if (ptrs[i]) {
			// 写入测试，验证内存可用性
			memset(ptrs[i], 0xAA, test_sizes[i]);
			printf("成功分配 %u 字节: %p\n", test_sizes[i],
			       ptrs[i]);
		} else {
			printf("分配 %u 字节失败\n", test_sizes[i]);
		}
	}

	// 释放所有分配的内存
	for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]);
	     i++) {
		if (ptrs[i]) {
			kfree(ptrs[i]);
			printf("释放 %u 字节内存: %p\n", test_sizes[i],
			       ptrs[i]);
		}
	}

	// 测试3：边界条件测试
	printf("\n3. 测试边界条件\n");
	void *zero_size = kmalloc(0);
	printf("分配0字节返回: %p\n", zero_size);

	void *large_size = kmalloc(1ULL << 31); // 尝试分配2GB内存
	printf("尝试分配2GB内存返回: %p\n", large_size);
	if (large_size)
		kfree(large_size);

	// 测试4：重复分配释放测试
	printf("\n4. 重复分配释放测试\n");
#define REPEAT_COUNT 1000
#define ALLOC_SIZE 1024
	for (int i = 0; i < REPEAT_COUNT; i++) {
		void *ptr = kmalloc(ALLOC_SIZE);
		if (ptr) {
			kfree(ptr);
			if (i % 100 == 0) {
				printf("完成第%d次分配释放循环\n", i);
			}
		} else {
			printf("第%d次分配失败\n", i);
			break;
		}
	}

	// 测试5：结构体指针分配和访问测试
	printf("\n5. 结构体指针分配和访问测试\n");

	// 定义测试用结构体
	struct test_struct {
		int id;
		char name[32];
		struct test_struct *next; // 修改为具体类型而不是void*
		long data[4];
	};

// 分配结构体数组
#define STRUCT_COUNT 5
	struct test_struct *structs[STRUCT_COUNT] = { NULL }; // 初始化为NULL

	// 分配和初始化结构体
	for (int i = 0; i < STRUCT_COUNT; i++) {
		structs[i] = kmalloc(sizeof(struct test_struct));
		if (structs[i]) {
			// 先清零整个结构体
			memset(structs[i], 0, sizeof(struct test_struct));

			// 初始化各个字段
			structs[i]->id = i + 1;
			snprintf(structs[i]->name, sizeof(structs[i]->name),
				 "struct_%d", i + 1);

			// 链接到下一个结构体
			if (i < STRUCT_COUNT - 1) {
				// 等待下一个结构体分配完成后再链接
				structs[i]->next = NULL;
			} else {
				structs[i]->next = NULL; // 最后一个节点
			}

			// 初始化数据数组
			for (int j = 0; j < 4; j++) {
				structs[i]->data[j] = i * 100 + j;
			}

			printf("成功分配结构体 %d: %p\n", i + 1,
			       (void *)structs[i]);
			printf("  id=%d, name=%s\n", structs[i]->id,
			       structs[i]->name);
		} else {
			printf("分配结构体 %d 失败\n", i + 1);
			break;
		}
	}

	// 分配完成后再建立链接
	for (int i = 0; i < STRUCT_COUNT - 1; i++) {
		if (structs[i] && structs[i + 1]) {
			structs[i]->next = structs[i + 1];
		}
	}

	// 验证链表遍历
	printf("\n验证结构体链表遍历:\n");
	struct test_struct *current = structs[0];
	int count = 0;
	while (current && count < STRUCT_COUNT) {
		printf("访问结构体 %d: id=%d, name=%s\n", count + 1,
		       current->id, current->name);
		// 验证数据数组
		printf("  data: [%d, %d, %d, %d]\n", current->data[0],
		       current->data[1], current->data[2], current->data[3]);

		current = current->next;
		count++;
	}

	// 释放结构体
	for (int i = STRUCT_COUNT - 1; i >= 0; i--) { // 从后向前释放
		if (structs[i]) {
			printf("释放结构体 %d: %p\n", i + 1,
			       (void *)structs[i]);
			kfree(structs[i]);
			structs[i] = NULL;
		}
	}

	// 测试6：页大小内存的读写测试
	printf("\n6. 页大小内存读写测试\n");

	// 分配一页大小的内存
	void *page_mem = kmalloc(PAGE_SIZE);
	if (!page_mem) {
		printf("页大小内存分配失败\n");
	} else {
		printf("成功分配页大小内存: %p\n", page_mem);

		// 按字节写入测试
		printf("写入测试 - 按字节填充...\n");
		uint8_t *byte_ptr = (uint8_t *)page_mem;
		for (size_t i = 0; i < PAGE_SIZE; i++) {
			byte_ptr[i] = i & 0xFF;
		}

		// 按字写入测试
		printf("写入测试 - 按字(4字节)填充...\n");
		uint32_t *word_ptr = (uint32_t *)page_mem;
		for (size_t i = 0; i < PAGE_SIZE / 4; i++) {
			word_ptr[i] = (i * 0x01010101);
		}

		// 验证内容
		printf("读取验证...\n");
		int errors = 0;
		for (size_t i = 0; i < PAGE_SIZE / 4; i++) {
			uint32_t expected = i * 0x01010101;
			uint32_t actual = word_ptr[i];
			if (expected != actual) { // 使用临时变量使逻辑更清晰
				printf("验证错误 位置 %u: 期望值=0x%08x, 实际值=0x%08x\n",
				       i, expected, actual);
				errors++;
				if (errors >= 5) {
					printf("错误太多，停止显示...\n");
					break;
				}
			}
		}

		if (errors == 0) {
			printf("内存读写验证成功!\n");
		} else {
			printf("发现 %d 个验证错误\n", errors);
		}

		// 边界访问测试
		printf("测试页面边界访问...\n");
		byte_ptr[0] = 0x55; // 首字节
		byte_ptr[PAGE_SIZE - 1] = 0xAA; // 末字节
		printf("边界访问验证: 首字节=0x%x, 末字节=0x%x\n", byte_ptr[0],
		       byte_ptr[PAGE_SIZE - 1]);

		// 释放内存
		kfree(page_mem);
		printf("页大小内存释放完成\n");
	}

	printf("\n内存分配器压力测试完成\n");
}