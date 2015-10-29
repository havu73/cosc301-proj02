#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#define PROMPT "prompt(lowercase) >>> "
#define SEQUENTIAL 0 
#define PARALLEL 1
#define NOEXIT 0
#define EXIT 1
#define CONTINUE 1
#define DISCONTINUE 0
#define RUNNING 1
#define PAUSED 0
struct stat;
struct pollfd;
struct _process{
	pid_t pid;
	char ** command;
	int state;
	struct _process* next;
};
typedef struct _process process;

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}
process * add_process(process * head, pid_t pid, 
						char ** command, int state) {
	 if (head==NULL){
        head= (process *)malloc(sizeof(process));
        head->command=command;
        //int * id = malloc(sizeof(int));
        //*id=pid;
        head -> pid=pid;
        head ->state= state;
        head->next=NULL;
        return head;
    }else{
        process * newhead=(process *)malloc(sizeof(process));
        newhead->command=command;
        newhead->pid=pid;
        newhead->state=state;
        newhead->next=head;
        return newhead;
    }
}

process * delete_process (process* head, pid_t pid){
	if (head->pid==pid){
		process * tmp=head;
		head=head->next;
		free_tokens(tmp->command);
		free(tmp);
	}
	else{
		process * tmp=head;
		while (tmp->next!=NULL){
			process * before=tmp;
			tmp=tmp->next;
			if (tmp->pid==pid){
				before->next=tmp->next;
				free_tokens(tmp->command);
				free(tmp);
				break;
			}
		}
	}
	return head;
}

process * find_pid(process * head, pid_t pid){
	if (head!=NULL && head->pid==pid){
		return head;
	}
	else if(head==NULL){
		return NULL;
	}
	else{
		process * tmp=head;
		while (tmp->next!=NULL){
			tmp=tmp->next;
			if (tmp->pid==pid){
				return tmp;
			}
		}
		return NULL;
	}
}

void print_process (process * head){
	if (head== NULL){
		printf("There are no processes running\n");
	}
	else {
		process * tmp=head;
		while (tmp!=NULL){
			char cmdprint[128];
			int icommand=0;
			int iprint=0;
			while ((tmp->command)[icommand]!=NULL){
				for (int i=0;i<strlen((tmp->command)[icommand]);i++){
					cmdprint[iprint]=(tmp->command)[icommand][i];
					iprint++;
				}
				cmdprint[iprint]=' ';
				iprint++;
				icommand++;
			}
			cmdprint[iprint]='\0';
			if (tmp->state==PAUSED){
				printf("Process ID: %d, Command: %s, State: paused\n",tmp->pid,cmdprint);
			}else{
				printf("Process ID: %d, Command: %s, State: running\n",tmp->pid,cmdprint);
			}
			tmp=tmp->next;
		}
	}
}

char** tokenify(const char *s, const char * delimiter) {
    // your code here
    //char* s1=(char*)malloc(sizeof(char)*(strlen(s)+1));
    char * s1=strdup(s);
    char* token;
    int size=0;
    for (token=strtok(s1,delimiter);token!=NULL;token=strtok(NULL,delimiter)){
        size++;
    }
    char** result;
    result=(char**)malloc(sizeof(char*)*(size+1));
    int i=0;
    //char* s2=(char*)malloc(sizeof(char)*(strlen(s)+1));
    char * s2=strdup(s);
    char* token2;
    for (token2=strtok(s2,delimiter);token2!=NULL;token2=strtok(NULL,delimiter)){
        result[i]=strdup(token2);
        i++;
    }
    result[i]=NULL;
    free(s1);
    free(s2);
    return result;
}


void print_tokens(char ** tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}


char ** load_directories(){
	FILE * dir_file=fopen("./shell-config","r");
	char next[128];
	int count=0;
	while(fgets(next,128,dir_file)!=NULL){
		count++;
		//next[0]='\0';
	}
	fclose(dir_file);
	char ** result=(char **)malloc(sizeof(char*)*(count+1));
	result[count]=NULL;
	dir_file=fopen("./shell-config","r");
	int i=0;
	while (fgets(next,128,dir_file)!=NULL){
		next[strlen(next)-1]='\0';
		result[i]=strdup(next);
		//next[0]='\0';
		i++;
	}
	fclose(dir_file);
	return result;
}
void free_directories(char **dir_list){
	int i=0;
	while (dir_list[i]!=NULL){
		free(dir_list[i]);
		i++;
	}
	free(dir_list[i]);
	free(dir_list);
}
int command_check(char ** one_command, char ** dir_list){
	char temp[128];
	int i=0;
	int rv;
	while (dir_list[i]!=NULL){
		strcpy(temp,dir_list[i]);
		temp[strlen(dir_list[i])]='/';
		for (int j=0;j<strlen(one_command[0]);j++){
			temp[strlen(dir_list[i])+1+j]=one_command[0][j];
		}
		temp[strlen(dir_list[i])+strlen(one_command[0])+1]='\0';
		struct stat statresult;
		rv=stat(temp,&statresult);
		if (rv>=0){
			//there exists such a program
			char * new=strdup(temp);
			char* tmp=one_command[0];
			one_command[0]=new;
			free(tmp);
			break;
		}
		i++;
	}
	return rv;
}
char ** get_commands(){
	char * stop;
	printf("%s",PROMPT);
	char buffer[1024];
	fgets(buffer,1024,stdin);
	buffer[strlen(buffer)-1]='\0';
	stop=strchr(buffer,'#');
	if (stop!=NULL){
		*stop='\0';
	}
	char ** commands=tokenify(buffer,";");
	while(commands[0]==NULL){
		printf("Please type in your commands\n");
		printf("%s",PROMPT);
		fgets(buffer,1024,stdin);
		buffer[strlen(buffer)-1]='\0';
		stop=strchr(buffer,'#');
		if (stop!=NULL){
			*stop='\0';
		}
		commands=tokenify(buffer,";");
	}
	return commands;
}

////////MODE//////////
void mode_proc(char ** one_command, int * mode){
	if (*mode==SEQUENTIAL){
		if (one_command[1]==NULL){
			printf("Current mode: SEQUENTIAL\n");
		}
		else if (strcmp(one_command[1],"p")==0 || strcmp(one_command[1],"parallel")==0){
			printf("Mode will change to PARALLEL after execution of the last process\n");
			*mode=PARALLEL;
		}
		else if (strcmp(one_command[1],"s")==0 || strcmp(one_command[1],"sequential")==0){
			printf("Already in SEQUENTIAL mode\n");
		}
		else{
			printf ("Invalid command: mode %s\n",one_command[1]);
		}
	}
	else{
		if (one_command[1]==NULL){
			printf("Current mode: PARALLEL\n");
		}
		else if (strcmp(one_command[1],"p")==0 || strcmp(one_command[1],"parallel")==0){
			printf("Already in PARALLEL mode\n");
		}
		else if (strcmp(one_command[1],"s")==0 || strcmp(one_command[1],"sequential")==0){
			printf("Mode will change to SEQUENTIAL after execution of the last process\n");
			*mode=SEQUENTIAL;
		}
		else{
			printf ("Invalid command: mode %s\n",one_command[1]);
		}
	}
	free_tokens(one_command);
}

//////EXIT///////
void exit_proc(char ** one_command,int * ex){
	if(one_command[1]==NULL){
		*ex=EXIT;
		printf("Exit after execution off all commands\n");
		free_tokens(one_command);
	}
	else{
		printf("Invalid command: exit %s\n",one_command[1]);
	}
}

//////////////JOBS/////////////
void jobs_proc(char** one_command,int *mode, process * head){
	if (*mode==SEQUENTIAL){
		printf("No other jobs running in sequential mode\n");
	}else{
		if (one_command[1]==NULL){
			printf("List of current jobs: \n");
			print_process(head);
		}else{
			printf("Invalid command: jobs %s\n",one_command[1]);
		}
	}
	free_tokens(one_command);
}

//////////////PAUSE/////////
void pause_proc(char** one_command,int * mode, process* head){
	if (*mode==SEQUENTIAL){
		printf("Pause is not a valid command in sequential mode\n");
	}
	else{
		if (one_command[1]==NULL){
			printf("Process ID not specified for pause\n");
		}else{
			int id =atoi(one_command[1]);
			process * found_process=find_pid(head,id);
			if (found_process==NULL){
				printf("Pause: Process not found : %d\n",id);
			}else{
				found_process->state=PAUSED;
				kill(id,SIGSTOP);
			}
		}
	}
	free_tokens(one_command);
}

//////////RESUME/////////
void resume_proc(char ** one_command, int * mode, process * head){
	if (*mode==SEQUENTIAL){
		printf("Resume is not a valid command in sequential mode\n");
	}
	else{
		if (one_command[1]==NULL){
			printf("Process ID not specified for pause\n");
		}else{
			int id =atoi(one_command[1]);
			process * found_process=find_pid(head,id);
			if (found_process==NULL){
				printf("Process not found : %d\n",id);
			}else{
				found_process->state=RUNNING;
				kill(id,SIGCONT);
			}
		}
	}
	free_tokens(one_command);
}

void parallel(int *mode){
	process * head=NULL;
	int cont=CONTINUE;
	char ** commands=get_commands();
	int ex=NOEXIT;
	char **dir_list=load_directories();
	while (cont==CONTINUE){
		int i=0;
		while (commands[i]!=NULL){
			char ** one_command=tokenify(commands[i]," \n\t");
			///////////MODE
			if (strcmp(one_command[0],"mode")==0){
				mode_proc(one_command, mode);
				if (*mode==SEQUENTIAL){
					cont=DISCONTINUE;
				}
				i++;
				continue;
			}
			///////////EXIT
			else if (strcmp(one_command[0],"exit")==0){
				exit_proc(one_command,&ex);
				if (ex==EXIT){
					cont=DISCONTINUE;
				}
				i++;
				continue;
			}
			////////////JOBS
			else if(strcmp(one_command[0],"jobs")==0){
				jobs_proc(one_command,mode,head);
				i++; 
				continue;
			}
			/////////////PAUSE
			else if (strcmp(one_command[0],"pause")==0){
				pause_proc(one_command,mode,head);
				i++;
				continue;
			}
			////////////RESUME
			else if (strcmp(one_command[0],"resume")==0){
				resume_proc(one_command, mode, head);
				i++;
				continue;
			}
			////NULL
			if (one_command[0]==NULL){
				free_tokens(one_command);
				i++;
				continue;
			}
			//////////ELSE
			//////////CHECK COMMAND VALIDITY
			struct stat statresult;
			int rv=stat(one_command[0],&statresult);
			if (rv<0){
				rv=command_check(one_command,dir_list);
				if (rv>=0){
					pid_t pid=fork();
					if (pid==0){//child process
						int exe=execv(one_command[0],one_command);
						if (exe<0){
							printf("Command failed. First word of command: %s",one_command[0]);
						}
					}else if(pid>0){//parents process
						head=add_process(head,pid,one_command,RUNNING);
					}else{
						printf("Fork failed. Command: %s\n",one_command[0]);
					}
				}else if (rv<0){
					printf("Invalid command. First command word: %s.\n", one_command[0]);
				}
			}
			else{
				pid_t pid=fork();
				if (pid==0){
					int exe=execv(one_command[0],one_command);
					if (exe<0){
						printf("Command failed. First word of command: %s\n",one_command[0]);
					}
				}else if (pid>0){
					head=add_process(head,pid,one_command,RUNNING);
				}else{
					printf("Fork failed. Could not create child process. Command: %s\n",one_command[0]);
				}
			}
			i++;
		}
		free_tokens(commands);
		if (cont==DISCONTINUE){
			//no checking for user input, only wait
			while (head !=NULL){
				int status;
				int id=wait(&status);
				if (id>0){
					process * found_process=find_pid(head,id);
					printf("Process terminated: ID: %d ;Command: %s\n",found_process->pid,found_process->command[0]);
					head=delete_process(head,id);
				}
			}
			free_directories(dir_list);
		}else{
			//check for user input, and then wait for any process if they terminated
			int status;
			struct pollfd pfd[1];
			pfd[0].fd=0;
			pfd[0].events=POLLIN;
			pfd[0].revents=0;
			while(1){
				int rv=poll(&pfd[0],1,10000);
				if (rv==0){//time out
					pid_t terminated=waitpid(-1,&status,WNOHANG);
					if (terminated>0){
						process * found_process=find_pid(head,terminated);
						printf("Process terminated: ID: %d ;Command: %s\n",found_process->pid,found_process->command[0]);
						head=delete_process(head,terminated);
					}
				}else if(rv<0){
					perror("poll");
				}else if (rv>0){
					char buffer[1024];
					read(pfd[0].fd, buffer, sizeof(buffer));
					buffer[strlen(buffer)-1] = '\0';
					char *comment = NULL;
					if ((comment = strchr(buffer, '#')) != NULL) {
						*comment = '\0';
					}
				    commands = tokenify(buffer, ";");
				    i = 0;
					break;
				}
			}
		}
	}
	if(ex==EXIT){
		exit(1);
	}
}
void sequential(int * mode,char ** commands){//if exit, need to free commands
											//otherwise, no need
	int ex=NOEXIT;
	int i=0;
	char ** dir_list=load_directories();
	while (commands[i]!=NULL){
		char ** one_command=tokenify(commands[i]," \n\t");
		//////////CHECK SPECIAL COMMAND/////////
		/////MODE
		if (strcmp(one_command[0],"mode")==0){
			mode_proc(one_command,mode);
			i++;
			continue;
		}
		/////EXIT
		else if (strcmp(one_command[0],"exit")==0){
			exit_proc(one_command,&ex);
			i++;
			continue;
		////JOBS
		}else if (strcmp(one_command[0],"jobs")==0){
			jobs_proc(one_command,mode,NULL);
			i++;
			continue;
		////PAUSE
		}else if (strcmp(one_command[0],"pause")==0){
			pause_proc(one_command,mode,NULL);
			i++;
			continue;
		////RESUME
		}else if (strcmp(one_command[0],"resume")==0){
			resume_proc(one_command,mode,NULL);
			i++;
			continue;
		}
		////NULL
		if (one_command[0]==NULL){
			free_tokens(one_command);
			i++;
			continue;
		}
		/////ELSE
		else{
			//////////CHECK COMMAND VALIDITY///////
			struct stat statresult;
			int rv=stat(one_command[0],&statresult);
			if (rv<0){
				rv=command_check(one_command,dir_list);
				if (rv>=0){
					pid_t pid=fork();
					if (pid==0){
						int exe=execv(one_command[0],one_command);
						if (exe<0){
							printf("Command failed. First word of command: %s",one_command[0]);
						}
					}else{
						int status;
						pid_t p;
						p=wait(&status);
					}
					//print_tokens(one_command);
				}else if (rv<0){
					printf("Invalid command. First command word: %s.\n", one_command[0]);
				}
			}
			else{
				pid_t pid=fork();
				if (pid==0){
					int exe=execv(one_command[0],one_command);
					if (exe<0){
						printf("Command failed. First word of command: %s\n",one_command[0]);
					}
				}else if (pid>0){
					int status;
					pid_t p;
					p=wait(&status);
				}else{
					printf("Fork failed. Could not create child process. Command: %s\n",one_command[0]);
				}
				//print_tokens(one_command);
			}
			///////FREE one_command/////
			free_tokens(one_command);
		}

		/////i++////
		i++;
	}
	///////IF EXIT///////
	if (ex==EXIT){
		free_directories(dir_list);
		free_tokens(commands);
		exit(EXIT_SUCCESS);
	}
	
	free_directories(dir_list);
}
int main(int argc, char **argv) {
	char ** commands;
	//char** commands=get_commands();
	//printf("%s\n",commands[0]);
	//free_tokens(commands);
	while (1){
	int mode=SEQUENTIAL;
	while(mode==SEQUENTIAL){
		commands=get_commands();
		sequential(&mode,commands);
		free_tokens(commands);
	}
	while (mode==PARALLEL){
		parallel(&mode);
	}
}
}

