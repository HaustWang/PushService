#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/file.h>

#include "comm_server.h"
#include "base.h"

volatile int stop = 0;
volatile int loadcmd = 0;
char **saved_argv = NULL;
static char *arg_start;
static char *arg_end;
static char *env_start;
static int backgd_mode = 0;

char *prog_name = NULL;
char *current_dir = NULL;
int lock_fd = -1;

volatile int app_set_rlimit = 0; // 应用程序是否自己设置rlimit

static void SigTermHandler(int signo)
{
	stop = 1;
    UNUSED(signo);
}

static void SigUsr1Handler(int signo)
{
	loadcmd = 1;
    UNUSED(signo);
}

static void dup_argv(int argc, char **argv)
{
	saved_argv = (char **)malloc (sizeof (char *) * (argc + 1));
	if (!saved_argv)
		return;
	saved_argv[argc] = NULL;
	while (--argc >= 0)
    {
		saved_argv[argc] = strdup (argv[argc]);
	}
}

static void free_argv(void)
{
	char **argv;
	for (argv = saved_argv; *argv; argv++)
		free (*argv);
	free (saved_argv);
	saved_argv = NULL;
}

int InitSignal()
{
	struct sigaction sa;
	sigset_t sset;

	memset(&sa, 0, sizeof(sa));

	// signals to term
	sa.sa_handler = SigTermHandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	// signals to loadcmd
	sa.sa_handler = SigUsr1Handler;
	sigaction(SIGUSR1, &sa, NULL);

	// signals to ignore
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	// other signals
	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);

	return 0;
}

int DaemonStart(int argc, char** argv, int run_daemon)
{
	arg_start = argv[0];
	arg_end = argv[argc-1] + strlen (argv[argc - 1]) + 1;
	env_start = environ[0];

	dup_argv(argc, argv);

	if (run_daemon)
	{
		daemon (1, 1);
		backgd_mode = 1;
	}
	return 0;
}

void DaemonStop(void)
{
	free_argv ();
	free (prog_name);
	free (current_dir);
	prog_name = NULL;
	current_dir = NULL;
}

void DaemonSetTitle(const char* fmt, ...)
{
	char title [64];
	int i, tlen;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf (title, sizeof (title) - 1, fmt, ap);
	va_end (ap);

	tlen = strlen (title) + 1;
	if (arg_end - arg_start < tlen && env_start == arg_end)
	{
		char *env_end = env_start;
		for (i=0; environ[i]; i++)
		{
			if(env_end == environ[i])
			{
				env_end = environ[i] + strlen (environ[i]) + 1;
				environ[i] = strdup(environ[i]);
			}
			else
			{
				break;
			}
		}
		arg_end = env_end;
		env_start = NULL;
	}

	i = arg_end - arg_start;
	if (tlen == i)
	{
		strcpy (arg_start, title);
	}
	else if (tlen < i)
	{
		strcpy (arg_start, title);
		memset (arg_start + tlen, 0, i - tlen);
	}
	else
	{
		stpncpy(arg_start, title, i - 1)[0] = '\0';
	}
}


void InitArgs(int argc, char **argv)
{
	prog_name = strdup(argv[0]);
	current_dir = strdup(get_current_dir_name());
	UNUSED(argc);
}


void SingleInstance(const char* prog_name)
{
	// 保证只有一个实例在运行
	int lock_fd = open(prog_name, O_RDONLY);
	if (lock_fd < 0)
	{
		printf("Open lock file %s failed, server init failed! %s\n", prog_name, strerror(errno));
		exit(1);
	}
	if (::flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
		printf("%s is already running!\n", prog_name);
		exit(1);
	}
	close(lock_fd);
}


int SetNonBlock(int sock_fd)
{
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags < 0)
    {
        perror("get sock_fd flags fail ");
        return -1;
    }
    if (fcntl(sock_fd, F_SETFL, flags|O_NONBLOCK) < 0)
    {
        perror("set sock_fd flags ");
        return -2;
    }

    return sock_fd;
}
