
	/* $Id: fpm_arrays.h,v 1.2 2008/05/24 17:38:47 anight Exp $ */
	/* (c) 2007,2008 Andrei Nigmatulin */

#ifndef FPM_ARRAYS_H
#define FPM_ARRAYS_H 1

#include <stdlib.h>
#include <string.h>

struct fpm_array_s {
	void *data;
	size_t sz;
	size_t used;
	size_t allocated;
};

/* struct fpm_array_s 是一个内存池
 * void *data: 代表 指向连续内存中第一个结构体 的指针
 * size_t sz: 代表 内存池所储存的结构体 的大小（sizeof）
 * size_t used: 代表 内存池已使用的内存分块的个数
 * size_t allocated: 代表
 */

static inline struct fpm_array_s *fpm_array_init(struct fpm_array_s *a, unsigned int sz, unsigned int initial_num) /* {{{ */
{
	void *allocated = 0;

	if (!a) {
		a = malloc(sizeof(struct fpm_array_s));

		if (!a) {
			return 0;
		}

		allocated = a;
	}

	a->sz = sz;

	a->data = calloc(sz, initial_num);

	if (!a->data) {
		free(allocated);
		return 0;
	}

	a->allocated = initial_num;
	a->used = 0;

	return a;
}
/* }}} */

static inline void *fpm_array_item(struct fpm_array_s *a, unsigned int n) /* {{{ */
{
	char *ret;

	ret = (char *) a->data + a->sz * n;

	return ret;
}
/* }}} */

static inline void *fpm_array_item_last(struct fpm_array_s *a) /* {{{ */
{
	return fpm_array_item(a, a->used - 1);
}
/* }}} */

static inline int fpm_array_item_remove(struct fpm_array_s *a, unsigned int n) /* {{{ */
{
	int ret = -1;

	if (n < a->used - 1) {
		void *last = fpm_array_item(a, a->used - 1);
		void *to_remove = fpm_array_item(a, n);

		memcpy(to_remove, last, a->sz);

		ret = n;
	}

	--a->used;

	return ret;
}
/* }}} */

/* 返回内存池中首个未使用内存分块的指针
 * 
 * 注意：如果内存池a已满或者未初始化(a->used == a->allocated),则该函数会为内存池申请新的空间
 */
static inline void *fpm_array_push(struct fpm_array_s *a) /* {{{ */
{
	void *ret;

	if (a->used == a->allocated) {
		size_t new_allocated = a->allocated ? a->allocated * 2 : 20;
                /* realloc: 重新分配给定的内存区域。如果a->data为NULL，则行为相当于malloc.
                 * 第一次分配20倍于struct size的内存，之后每次翻倍
                 */
		void *new_ptr = realloc(a->data, a->sz * new_allocated);

                /* 分配内存失败，返回0*/
		if (!new_ptr) {
			return 0;
		}

                /*把指针赋值给a->data,把内存倍数赋值给a->allocated*/
		a->data = new_ptr;
		a->allocated = new_allocated;
	}

        /* 得到内存池a的首个未使用内存分块的指针*/
	ret = fpm_array_item(a, a->used);

	++a->used;

	return ret;
}
/* }}} */

static inline void fpm_array_free(struct fpm_array_s *a) /* {{{ */
{
	free(a->data);
	a->data = 0;
	a->sz = 0;
	a->used = a->allocated = 0;
}
/* }}} */

#endif
