/*
*文件名称: comm_server.h
*文件标识: 
*摘    要: 进程要使用的一类通用函数
*
*当前版本: 1.0
*/

#ifndef _COMM_SERVER_H_
#define _COMM_SERVER_H_


volatile extern int stop;
volatile extern int loadcmd;

extern char *prog_name;
extern char *current_dir;

void DaemonStop(void); 
int DaemonStart(int argc, char** argv, int run_daemon);
void DaemonSetTitle(const char* fmt, ...);

void InitArgs(int argc, char **argv);

int InitSignal();

void SingleInstance(const char* prog_name);
int SetNonBlock(int sock_fd);
#endif

