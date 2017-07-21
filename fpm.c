
	/* $Id: fpm.c,v 1.23 2008/07/20 16:38:31 anight Exp $ */
	/* (c) 2007,2008 Andrei Nigmatulin */

#include "fpm_config.h"

#include <stdlib.h> /* for exit */

#include "fpm.h"
#include "fpm_children.h"
#include "fpm_signals.h"
#include "fpm_env.h"
#include "fpm_events.h"
#include "fpm_cleanup.h"
#include "fpm_php.h"
#include "fpm_sockets.h"
#include "fpm_unix.h"
#include "fpm_process_ctl.h"
#include "fpm_conf.h"
#include "fpm_worker_pool.h"
#include "fpm_scoreboard.h"
#include "fpm_stdio.h"
#include "fpm_log.h"
#include "zlog.h"

/* 初始化全局变量fpm_globals */
struct fpm_globals_s fpm_globals = {
	.parent_pid = 0,
	.argc = 0,
	.argv = NULL,
	.config = NULL,
	.prefix = NULL,
	.pid = NULL,
	.running_children = 0,
	.error_log_fd = 0,
	.log_level = 0,
	.listening_socket = 0,
	.max_requests = 0,
	.is_child = 0,
	.test_successful = 0,
	.heartbeat = 0,
	.run_as_root = 0,
	.force_stderr = 0,
	.send_config_pipe = {0, 0},
};

/* fpm初始化 */
int fpm_init(int argc, char **argv, char *config, char *prefix, char *pid, int test_conf, int run_as_root, int force_daemon, int force_stderr) /* {{{ */
{
        /* 为全局变量fpm_globals赋值*/
	fpm_globals.argc = argc;
	fpm_globals.argv = argv;
	if (config && *config) {
		fpm_globals.config = strdup(config);
	}
	fpm_globals.prefix = prefix;
	fpm_globals.pid = pid;
	fpm_globals.run_as_root = run_as_root;
	fpm_globals.force_stderr = force_stderr;

        /* 各部件初始化 */
	if (0 > fpm_php_init_main()           ||    //注册一个析构函数
	    0 > fpm_stdio_init_main()         ||    //关闭标准输入和标准输出
	    0 > fpm_conf_init_main(test_conf, force_daemon) ||  //读取配置文件写成字符串，然后解析字符串赋值给结构体，再向LOG写当前配置，最后注册析构函数
	    0 > fpm_unix_init_main()          ||    //定义了该进程打开文件描述符的最大数量以及该进程core dump file的最大size，然后使进程daemonize。最后设置部分worker pool信息
	    0 > fpm_scoreboard_init_main()    ||    //为每个worker pool各自申请scoreboard并初始化
	    0 > fpm_pctl_init_main()          ||    //给全局变量static char **saved_argv赋值并注册析构
	    0 > fpm_env_init_main()           ||    //如果kv结构的value值为"$"，那么读取系统环境变量作为此值。如果kv结构中定义了USER或HOME，那么把wp中user或home释放掉并设为NULL
	    0 > fpm_signals_init_main()       ||    //更改指定信号的action，并创建了socketpair,当收到这些信号时，往其中一个socket写入指定的值。这个值代表收到了哪个信号
	    0 > fpm_children_init_main()      ||    //注册全局变量last_faults并初始化且注册析构函数。last_faults数组的元素是收到信号的时刻
	    0 > fpm_sockets_init_main()       ||    //根据www.conf中的listen为每个woker pool获得wp->listening_socket(一个fd)
	    0 > fpm_worker_pool_init_main()   ||    //注册worker pool的构析函数
	    0 > fpm_event_init_main()) {            //初始化每个worker pool wp->config->pm_max_children个数的子进程

		if (fpm_globals.test_successful) {
			exit(FPM_EXIT_OK);
		} else {
			zlog(ZLOG_ERROR, "FPM initialization failed");
			return -1;
		}
	}

        /*如果全局配置pid不为空，就创建一个文件，并向其写入主进程的pid*/
	if (0 > fpm_conf_write_pid()) {
		zlog(ZLOG_ERROR, "FPM initialization failed");
		return -1;
	}

        /*启动zlog，并把stderr定向到fpm_globals.error_log_fd */
	fpm_stdio_init_final();
	zlog(ZLOG_NOTICE, "fpm is running, pid %d", (int) fpm_globals.parent_pid);

	return 0;
}
/* }}} */

/*	children: return listening socket
	parent: never return */
int fpm_run(int *max_requests) /* {{{ */
{
	struct fpm_worker_pool_s *wp;

	/* create initial children in all pools */
	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {
		int is_parent;

		is_parent = fpm_children_create_initial(wp);

		if (!is_parent) {
			goto run_child;
		}

		/* handle error */
		if (is_parent == 2) {
			fpm_pctl(FPM_PCTL_STATE_TERMINATING, FPM_PCTL_ACTION_SET);
			fpm_event_loop(1);
		}
	}

	/* run event loop forever */
	fpm_event_loop(0);

run_child: /* only workers reach this point */

	fpm_cleanups_run(FPM_CLEANUP_CHILD);

	*max_requests = fpm_globals.max_requests;
	return fpm_globals.listening_socket;
}
/* }}} */

