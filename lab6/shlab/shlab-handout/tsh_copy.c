/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* build-in cmd */
#define QUIT "quit"
#define JOBS "jobs"
#define BGCMD "bg"
#define FGCMD "fg"
/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);  // TODO: TO RESOURCE THE CHILD PROCESS
int builtin_cmd(char **argv);  // FINISHED
void do_bgfg(char **argv); // FINISHED
void waitfg(pid_t pid);  // FINISHED

void sigchld_handler(int sig);   // FINISHED
void sigtstp_handler(int sig);  // FINISHED
void sigint_handler(int sig);  // FINISHED

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}


/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    int  bg;  // 是否运行在后台
    int build_in;  // 是否是内建命令
    int pid;  // 子进程的 pid
    // 1. 解析命令行参数
    char* argv[MAXLINE];  //  用于记录命令行参数
    bg = parseline(cmdline , argv); // argv 中记录着参数,并且之后的 & 已经被去除了
    // 2. 判断是否是内置命令
    build_in = builtin_cmd(argv);
    // 3. 如果是内置命令执行内置参数
    if(build_in) {
        if(strcmp(argv[0] , QUIT) == 0) {
            // 执行 quit 命令
            // 回收所有的子进程
            // 发送 kill 给所有子进程
            for(int i = 0 ; i < MAXJOBS ; i ++) {
                if(jobs[i].pid != 0) {
                    kill(jobs[i].pid , SIGKILL);
                }
            }
            exit(0);  // 注意回收子进程
        } else if(strcmp(argv[0] , JOBS) == 0) {
            // 执行 jobs 命令,列出所有后台执行的进程
            listjobs(jobs);
            return ;
        } else if(strcmp(argv[0] , BGCMD) == 0 || strcmp(argv[0] , FGCMD) == 0) {
            // 执行 bg job 命令  TODO: 完成bg,fg命令
            do_bgfg(argv);
            return ;
        }
    }
    // 4. 否则 Fork 子进程并且执行相应的程序
    // 4.0 Fork 之前首先阻塞信号,防止子进程直接结束了,加入被回收的进程到 jobs 中
    // 4.0.1 初始化信号集合
    sigset_t new_set , old_set , mask_all;  
    sigfillset(&mask_all);
    sigaddset(&new_set , SIGCHLD);
    // 4.0.2 阻塞信号
    sigprocmask(SIG_BLOCK , &new_set , &old_set);
    
    if((pid = fork()) == 0) {
        // 4.1 首先设置进程组 id
        sigprocmask(SIG_SETMASK , &old_set , NULL);
        setpgid(0 , 0);  // 表示把子进程组的 ID 设置为自己的 PID
        // 4.2 执行第三方程序, TODO: 加入后台进程
        // 注意放弃对于信号的阻塞
        execve(argv[0] , argv , environ);
    } else if(pid > 0) {
        // 5. 父进程需要把子进程加入到jobs中
        if(bg) {
            sigprocmask(SIG_BLOCK , &mask_all  , NULL);
            addjob(jobs , pid , BG , cmdline);
            sigprocmask(SIG_SETMASK , &old_set , NULL);  // 放弃阻塞
            printf("[%d] (%d) %s" , pid2jid(pid) , pid , cmdline);
            // 打印后台进程的信息
        } else {
            sigprocmask(SIG_BLOCK , &mask_all , NULL);
            addjob(jobs , pid , FG , cmdline);
            sigprocmask(SIG_SETMASK , &old_set , NULL); // 放弃阻塞
            // 6. 阻塞等待前台进程完成
            waitfg(pid);
        }
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 *  
 *  return 1 if the process should be excuted in the background
 *  return 0 if the process should be excuted in the foreground
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    // 1. 判断是否是内置命令
    char* cmd_name = argv[0];
    if(strcmp(cmd_name , QUIT) == 0) return 1;
    if(strcmp(cmd_name , JOBS) == 0) return 1;
    if(strcmp(cmd_name , BGCMD) == 0) return 1;
    if(strcmp(cmd_name , FGCMD) == 0) return 1;
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    // 1. 首先获取到 JID
    char* choice = argv[0];
    char* jid_str = argv[1];
    int  jid;
    if(jid_str[0] == '%') {
        jid = jid_str[1] - 48;
    } else {
        jid = pid2jid(atoi(jid_str));
    }
    // 2. 获取到 job
    struct job_t* job = getjobjid(jobs , jid);
    if(job == NULL) {
        // %3: No such job
        printf("%s: No such job\n" , argv[1]);
        return ;
    }
    if(strcmp(choice , BGCMD) == 0) {
        // 3. 移动进程到后台执行
        job -> state = BG;
    } else if(strcmp(choice , FGCMD) == 0) {
        // 4. 移动进程到前台执行
        job -> state = FG;
        waitfg(job -> pid);
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
    作用: 等待前台进程执行完毕,也就是发送 SIGCHLD 指令
 */
void waitfg(pid_t pid)
{
    // 利用轮询的方式,检测 jobs 中是否存在相应的 pid 的进程即可
    // 每一次也只有一个前台进程
    sigset_t mask;
    sigemptyset(&mask);
    while(fgpid(jobs) != 0) {
        sigsuspend(&mask);  // 接受所有的信号
    }
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 *   作用: 接收到所有的 SIGCHLD 信号回收停止的子进程,但是不要等待已经结束的子进程
 */
void sigchld_handler(int sig) 
{
    // 回收子进程
    int olderrno = errno;  // 表示记录原来的 errno
    int pid , status;
    sigset_t new_set , old_set;
    sigfillset(&new_set);
    while((pid = waitpid(-1 , &status , 0)) > 0) {
        // 首先判断退出状态
        // 阻塞所有信号
        sigprocmask(SIG_BLOCK , &new_set , &old_set);
        if(WIFSIGNALED(status)) {
            // 表示是由一个没有捕获的信号导致退出的
            printf("Job [%d] (%d) terminated by signal %d\n" , pid2jid(pid) , pid , WTERMSIG(status));
        }
        // 进行一系列的操作
        deletejob(jobs , pid);
        // 取消信号的阻塞
        sigprocmask(SIG_SETMASK , &old_set , NULL);
    }
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 *  作用: 捕捉 SIGINT信号,并且发送给前台进程
 */
void sigint_handler(int sig) 
{
    // 发送给前台进程(也就是前台进程组)
    int fpid;
    // 为了防止发送信号给前台进程的过程中被其他的信号干扰这里阻塞信号
    sigset_t new_set  , old_set;
    sigfillset(&new_set);
    sigprocmask(SIG_BLOCK , &new_set , &old_set);
    if((fpid = fgpid(jobs)) > 0) {
        kill(-fpid , SIGINT);
    }
    sigprocmask(SIG_SETMASK , &old_set , NULL);
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 * 
 *  作用: 捕捉SIGTSIP信号并且发送给前台进程
 */
void sigtstp_handler(int sig) 
{
    // 1. 发送信号给前台进程
    int fpid;
    sigset_t new_set , old_set;
    sigfillset(&new_set);
    sigprocmask(SIG_BLOCK , &new_set , &old_set);
    if((fpid = fgpid(jobs)) > 0) {
        kill(-fpid , SIGTSTP);
    }
    sigprocmask(SIG_SETMASK , &new_set , &old_set);
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



