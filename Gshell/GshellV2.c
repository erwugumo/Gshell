/*
*author: gumo time: 2020/6/17
*A little shell
*Internam Command:
*cs: change the command prompt
*cd: change work dirctory
*&: create pure background process 
*jobs: show the background process
* > < : redirct output and input
*fg: put a background process in the foreground and continue
*bg: make a background process continue 
*ctrl+z: make a foreground process stop
*ctrl+c: make a foreground process exit
*Most External Command 
*/
#include "apue.h"
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#define MAX_CHILD_NUM 64
#define NONE "\033[m"
#define LIGHT_BLUE "\033[1;34m"
#define NAME_COLOR "\033[44;37m"
#define PATH_COLOR "\033[42;37m"
#define TIME_COLOR "\033[47;30m"
#define RED "\033[0;32;31m"
#define GREEN "\033[0;32;32m"
#define WHITE "\033[0;32;37m"

char	my_sig='$';
pid_t	pid,shellid;
int		redirct_input(char* file);
int		redirct_output(char* file);
char	user_name[MAXLINE];
char	path_buf[MAXLINE];
void	pppr();
int		my_cd(char* arg,char* path_buf);
char	my_cs(char* arg);
int		child_id[MAX_CHILD_NUM]={0};
int		background_stopped_id[MAX_CHILD_NUM]={0};
int		my_getid(int pid,int *child_id);
int		my_jobs();
int		my_bg(char* arg);
int		my_fg(char* arg);
static void	sig_chld(int);
int		cur_proc=0;
volatile sig_atomic_t f_end=0,pid_chld=-1,pid_backstop=-1,pid_backstop_pre=-1,f_fg=0,f_bg=0,exit_type=-1;
int		f_redirctin=0,f_redirctout=0,f_background=0,f_foreground=0,f_backstop=0,f_backcont=0;
int		fg_pid=-1;
int		bg_pid[MAX_CHILD_NUM]={0};

int
main(void)
{
	char	buf[MAXLINE];
	struct	passwd* c_user=NULL;
	char 	arg[6][MAXLINE];
	int		red_inloc=-1,red_outloc=-1;

	struct sigaction act_chld;
	act_chld.sa_handler=sig_chld;
	act_chld.sa_flags=SA_RESTART;
	sigaction(SIGCHLD,&act_chld,NULL);
	sigemptyset(&act_chld.sa_mask);

	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);

	shellid=tcgetpgrp(0);

	c_user=getpwuid(getuid());
	strcpy(user_name,c_user->pw_name);
	getcwd(path_buf,MAXLINE);
	pppr();

	while (1) 
	{
		while(fgets(buf,MAXLINE,stdin)==NULL);
		/* init */
		pid=-1;
		f_redirctin=0;
		f_redirctout=0;
		f_background=0;
		f_foreground=0;
		f_backstop=0;
		f_backcont=0;
		f_end=1;
		f_bg=0;
		f_fg=0;
		red_inloc=MAXLINE;
		red_outloc=MAXLINE;
		if(buf[0]=='\n')
		{
			pppr();
			continue;
		}
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0; /* replace newline with null */
		memset(arg,0,MAXLINE*6);
		/* analyse command */
                int i = 0,l = 0,c = 0;
		int fd[2];
		if(pipe(fd)<0)
			printf("Create Pipe Error.\n");
                for(i=0;i<strlen(buf);i++)
                {
                        if(buf[i]!=0)
                        {
                                if(buf[i]==' ')
                                {
                                        arg[l][c]=0;
                                        l++;
                                        c = 0;
                                        continue;
                                }
				if(buf[i]=='<')
				{
					f_redirctin=1;
					red_inloc=l;
					if(buf[i+1]==' ')
						i++;
					continue;
				}
				if(buf[i]=='>')
				{
					f_redirctout=1;
					red_outloc=l;
					if(buf[i+1]==' ')
						i++;
					continue;
				}
                                arg[l][c]=buf[i];
                                c++;
                        }
                }
		/* my own function */
		if(!strcmp(arg[0],"cd"))
		{
	  		my_cd(arg[1],path_buf);
			pppr();
			continue;
                }
                else if(!strcmp(arg[0],"exit"))
                {
			printf("Bye~\n");
			fflush(stdout);
                        exit(0);
                }
		else if(!strcmp(arg[0],"cs"))
		{
			if(f_redirctin==1||f_redirctout==1)
				printf("Error,the signal could not be \"<\" or \">\"!\n");
			else
				my_sig=my_cs(arg[1]);
			pppr();
			continue;
		}
		else if(!strcmp(arg[0],"&"))
		{
			f_background=1;
		}
		else if(!strcmp(arg[0],"jobs"))
		{
			my_jobs();
			pppr();
			continue;
		}
		else if(!strcmp(arg[0],"bg"))
		{	
			f_backcont=1;
			f_end=0;
			f_bg=1;
			my_bg(arg[1]);
			pppr();
			continue;
		}
		else if(!strcmp(arg[0],"fg"))
		{
			f_foreground=1;
			f_fg=1;
			fg_pid=atoi(arg[1]);
			cur_proc=my_getid(atoi(arg[1]),child_id);
			int i;
			for(i=0;i<MAX_CHILD_NUM;i++)
			{
				if(atoi(arg[1])==bg_pid[i])
				{
					bg_pid[i]=0;
					break;
				}
			}
			if(my_fg(arg[1])<0)
			{
				pppr();
				continue;
			}
		}
		if(f_foreground==0&&f_backcont==0)
		{
			/* other command */
			pid=fork();
			cur_proc=my_getid(pid,child_id);
			if(f_background==1&&pid>0) /* background */
			{
				close(fd[1]);
				setpgid(pid,pid);
			}
			if(f_background==0&&pid>0)
			{
				setpgid(pid,pid);
				tcsetpgrp(0,child_id[cur_proc]);
			}
			if (pid < 0) 
				err_sys("fork error");
			else if (pid == 0) /* child1 */ 
			{	
				setpgid(child_id[cur_proc],child_id[cur_proc]);
				if(f_background==0)
					tcsetpgrp(0,child_id[cur_proc]);
				if(f_background==1)
				{
					close(fd[0]); /* pipe write in child1 */
					int ppid;
					ppid=fork(); /* double fork */
					if(ppid<0)
					{
						err_sys("fork error");
						exit(127);
					}
					else if(ppid>0) /* child1 */
					{       
						char ppid_buf[20]={0};
						sprintf(ppid_buf,"%d",ppid);
						write(fd[1],ppid_buf,20);
						exit(0);
					}
				}
				/* if background child1 else child2 */
				int Is_red=1;
				int Is_background=1;
				if(f_redirctin==0&&f_redirctout==0)
					Is_red=0;
				if(f_background==0)
					Is_background=0;
				if(f_redirctin==1)
					if(redirct_input(arg[red_inloc])<0)
						exit(127);
				if(f_redirctout==1)
					if(redirct_output(arg[red_outloc])<0)
						exit(127);
				int loc_min=l;
				/* if redirct command cut */
				if(Is_red==1)
				{
					if(red_inloc<=red_outloc)
						loc_min=red_inloc;
					else
						loc_min=red_outloc;
				}
				l=loc_min-Is_red;
				if(l == 0)
					execlp(arg[0+Is_background],arg[0+Is_background],(char*)0);
				else if(l == 1)
					execlp(arg[0+Is_background],arg[0+Is_background],arg[1+Is_background],(char*)0);
				else if(l == 2)
					execlp(arg[0+Is_background],arg[0+Is_background],arg[1+Is_background],arg[2+Is_background],(char*)0);
				else if(l == 3)
					execlp(arg[0+Is_background],arg[0+Is_background],arg[1+Is_background],arg[2+Is_background],arg[3+Is_background],(char*)0);
				else if(l == 4)
					execlp(arg[0+Is_background],arg[0+Is_background],arg[1+Is_background],arg[2+Is_background],arg[3+Is_background],arg[4+Is_background],(char*)0);
				else if(l == 5)
					execlp(arg[0+Is_background],arg[0+Is_background],arg[1+Is_background],arg[2+Is_background],arg[3+Is_background],arg[4+Is_background],arg[5+Is_background],(char*)0);	
				err_ret("couldn't execute: %s\n", buf);
				fflush(stdout);
				fflush(stderr);
				exit(127);
			}
		}
		/* parent */
		if(f_background==1)
		{
			char pid_buff[20]={0};
			read(fd[0],pid_buff,20);
			my_getid(atoi(pid_buff),child_id);
			printf("pid=%d is running.\n",atoi(pid_buff));
			int i;
			for(i=0;i<MAX_CHILD_NUM;i++)
			{
				if(bg_pid[i]==0)
				{
					bg_pid[i]=atoi(pid_buff);
					break;
				}
			}
		}
		while(1)
		{
			if(exit_type>=0)
			{
				if(exit_type==0) /* exit */
				{
					int i;
					for(i=0;i<MAX_CHILD_NUM;i++)
					{
						if(pid_chld==bg_pid[i])
						{
							bg_pid[i]=0;
							exit_type=-1;
							break;
						}
					}
					if(i<MAX_CHILD_NUM)
						continue;
					else
					{
						if(f_background==1) /* & mini proc exit */
						{
							exit_type=-1;
							break;
						}
						int i;
						for(i=0;i<MAX_CHILD_NUM;i++)
						{
							if(pid_chld==child_id[i])
							{
								child_id[i]=0; /* normal */
								break;
							}
						}
						exit_type=-1;
						if(i<MAX_CHILD_NUM)
							break;
					}
				}
				else if(exit_type==1)/* ctrl + z */
				{
					exit_type=-1;
					int i;
					for(i=0;i<MAX_CHILD_NUM;i++)
					{
						if(bg_pid[i]==0)
						{
							bg_pid[i]=pid_chld;
							exit_type=-1;
							break;
						}
					}
					break;
				}
				else if(exit_type==2)	/* bg or fg continue */
				{
					int i;
					for(i=0;i<MAX_CHILD_NUM;i++)
					{
						if(pid_chld==bg_pid[i])
						{
							exit_type=-1;
							break;
						}
					}
					if(pid_chld==fg_pid)
						exit_type=-1;
				}
				else if(exit_type==3)
				{
					exit_type=-1;
					break;
				}
			}
		}
		tcsetpgrp(0,shellid);
		pppr();
	}
	exit(0);
}
void
sig_chld(int signo)
{
	int exit_pid,status;
	exit_pid=waitpid(-1,&status,0|WUNTRACED|WCONTINUED);
	if(exit_pid<0)
		perror("wait");
	if(WIFEXITED(status))
	{
		/*printf("pid=%d exited\n",exit_pid);*/
		exit_type=0;
	}
	else if(WIFSTOPPED(status))/* ctrl + z */
	{
		printf("pid=%d stopped\n",exit_pid);
		exit_type=1;
	}
	else if(WIFCONTINUED(status))/* bg or fg */
	{
		printf("pid=%d continued\n",exit_pid);
		exit_type=2;
	}
	else if(WIFSIGNALED(status))/* ctrl + c */
	{
		printf("pid=%d exitted\n",exit_pid);
		exit_type=3;
	}
	pid_chld=exit_pid;
}

int my_bg(char* arg)
{
	
	int t_pid=atoi(arg);
	char filename[256]={0};
	sprintf(filename,"/proc/%d/status",t_pid);
	FILE* fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("pid error.\n");
		return(-1);
	}
	else
	{
		if(kill(t_pid,SIGCONT)<0)
		{
			printf("kill error\n");
			return(-1);
		}
	}
	printf("pid=%d in the background continuing.\n",t_pid);
	return(0);
}

int my_fg(char* arg)
{
	int t_pid=atoi(arg);
	char filename[256]={0};
	sprintf(filename,"/proc/%d/status",t_pid);
	FILE* fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("pid error.\n");
		return(-1);
	}
	else
	{
		printf("pid=%d is in the front.\n",t_pid);
		tcsetpgrp(0,t_pid);
		if(kill(t_pid,SIGCONT))
		{
			printf("kill error.\n");
			return(-1);
		}
		return(0);
	}
}

int
redirct_input(char* file)
{
	int fd;
	if(access(file,F_OK)!=0)
	{
		printf("Input Error.\n");
		fflush(stdout);
		return(-1);
	}
	fd=open(file,O_RDONLY,0644);
	dup2(fd,0);
	return(0);
}

int
redirct_output(char* file)
{
	int fd;
	fd=open(file,O_WRONLY|O_TRUNC,0664);
	if(access(file,F_OK)!=0)
	{
		printf("Output Error.\n");
		fflush(stdout);
		return(-1);
	}
	dup2(fd,1);
	return(0);
}

void
pppr()
{
	time_t t;
	time(&t);
	struct tm *lt;
	lt=localtime(&t);
	if(lt->tm_min>=10&&lt->tm_hour>=10)
		printf("%s %s v2 %s %s %s %d:%d %s%c%s ",NAME_COLOR,user_name,PATH_COLOR,path_buf,TIME_COLOR,lt->tm_hour,lt->tm_min,WHITE,my_sig,NONE);
	else if(lt->tm_min<10&&lt->tm_hour>=10)
		printf("%s %s v2 %s %s %s %d:0%d %s%c%s ",NAME_COLOR,user_name,PATH_COLOR,path_buf,TIME_COLOR,lt->tm_hour,lt->tm_min,WHITE,my_sig,NONE);
	else if(lt->tm_min>=10&&lt->tm_hour<10)
		printf("%s %s v2 %s %s %s 0%d:%d %s%c%s ",NAME_COLOR,user_name,PATH_COLOR,path_buf,TIME_COLOR,lt->tm_hour,lt->tm_min,WHITE,my_sig,NONE);
	else
		printf("%s %s v2 %s %s %s 0%d:0%d %s%c%s ",NAME_COLOR,user_name,PATH_COLOR,path_buf,TIME_COLOR,lt->tm_hour,lt->tm_min,WHITE,my_sig,NONE);
	fflush(stdout);
}

int
my_cd(char* arg,char* path_buf)
{
	if(!strcmp(arg,"~"))
	{
		if(strcmp(user_name,"root"))
			sprintf(arg,"/home/%s",user_name);
		else
			arg="/";
	}
	if(chdir(arg)<0)
	{
		 perror("cd");
		 return(-1);
	}
	else
	{
		 getcwd(path_buf,MAXLINE);
		 return(0);
	}
}

char
my_cs(char* arg)
{
	if(strlen(arg)>1)
	{
		printf("Error,the signal is too long!\n");
		return(my_sig);
	}
	else if(!strcmp(arg,"<")||!strcmp(arg,">"))
	{
		printf("Error,the signal could not be \"<\" or \">\"!\n");
		return(my_sig);
	}
	printf("Now the signal is %c.\n",arg[0]);
	return(arg[0]);
}

int
my_getid(int pid, int* child_id)
{
	int i;
	for(i=0;i<MAX_CHILD_NUM;i++)
	{
		if(child_id[i]==pid)
			return(i);
		if(child_id[i]==0)
		{
			child_id[i]=pid;
			return(i);
		}
		else
		{
			char filename[256]={0};
			sprintf(filename,"/proc/%d/status",child_id[i]);
			FILE* fp=fopen(filename,"r");
			if(fp==NULL)
			{
				child_id[i]=pid;
				return(i);
			}
		}
	}
	return(-1);
}

int
my_jobs()
{
	int i;
	int Is_empty=0;
	for(i=0;i<MAX_CHILD_NUM;i++)
	{
		char buf1[256],buf2[256],buf3[256];
		if(child_id[i]>0)
		{
			char filename[256]={0};
			sprintf(filename,"/proc/%d/status",child_id[i]);
			FILE* fp=fopen(filename,"r");
			if(fp!=NULL)
			{
				if(fgets(buf1,255,fp)==NULL)
				{
					fclose(fp);
					continue;
				}
				else if (buf1[strlen(buf1) - 1] == '\n')
					buf1[strlen(buf1) - 1] = 0; /* replace newline with null */
				if(fgets(buf2,255,fp)==NULL)
				{
					fclose(fp);
					continue;
				}
				if(fgets(buf3,255,fp)==NULL)
				{
					fclose(fp);
					continue;
				}
				else if (buf3[strlen(buf3) - 1] == '\n')
					buf3[strlen(buf3) - 1] = 0; /* replace newline with null */
				Is_empty=1;
				printf("pid=	%d %s %s\n",child_id[i],buf1,buf3);
				fclose(fp);
			}
		}
	}
	if(Is_empty==0)
		printf("Background is empty.\n");
	return(0);
}
