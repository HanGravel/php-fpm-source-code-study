
	/* $Id: fpm_cleanup.c,v 1.8 2008/05/24 17:38:47 anight Exp $ */
	/* (c) 2007,2008 Andrei Nigmatulin */

#include "fpm_config.h"

#include <stdlib.h>

#include "fpm_arrays.h"
#include "fpm_cleanup.h"

struct cleanup_s {
	int type;
	void (*cleanup)(int, void *);
	void *arg;
};

static struct fpm_array_s cleanups = { .sz = sizeof(struct cleanup_s) };
/* 全局变量cleanups是结构体cleanup_s的内存池 */

/* 向cleanups中添加函数。这些函数将在最后统一调用。可以理解为注册析构函数 */
int fpm_cleanup_add(int type, void (*cleanup)(int, void *), void *arg) /* {{{ */
{
	struct cleanup_s *c;

        /* 从内存池cleanups中取出第一个可用内存块 */
	c = fpm_array_push(&cleanups);

	if (!c) {
		return -1;
	}

        /* 为c赋值 */
	c->type = type;
	c->cleanup = cleanup;
	c->arg = arg;

	return 0;
}
/* }}} */

void fpm_cleanups_run(int type) /* {{{ */
{
	struct cleanup_s *c = fpm_array_item_last(&cleanups);
	int cl = cleanups.used;

	for ( ; cl--; c--) {
		if (c->type & type) {
			c->cleanup(type, c->arg);
		}
	}

	fpm_array_free(&cleanups);
}
/* }}} */

