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
#define PROMPT "prompt (lowercase)>>> "
#define SEQUENTIAL 0 
#define PARALLEL 1
#define NOEXIT 0
#define EXIT 1
struct stat;
/*
char ** separate_command(char * buffer){
		int i=0;
		int command=1;
		while (i<strlen(buffer)&&buffer[i]!='#'){
			if (buffer[i]==';'){
				command++;
			}
			i++;
		}
		i=0;
		char ** result= (char**)malloc((command+1)*sizeof(char*));
		//checked: printf("%d\n",command+1);
		result[command]=NULL;
		//checked:  printf("%s\n",result[command]);
		char str[128];
		int strindex=0;
		int index=0;
		while (i<strlen(buffer)&& buffer[i]!='#'){
			if (buffer[i]!=';'){
				//checked:  printf("%c\n",buffer[i]);
				str[strindex]=buffer[i];
				strindex++;
			}else{
				str[strindex]='\0';
				//checked: printf("%s\n",str);
				strindex=0;
				result[index]=strdup(str);
				str[0]='\0';
				index++;
			}
			i++;
		}
		str[strindex]='\0';
		result[index]=strdup(str);
		result[index+1]=NULL;
		//checked: printf("%s\n", result[index+1]);
		return result;
}
void free_command(char ** command){
	int i=0;
	while (command[i]!=NULL){
		free(command[i]);
		i++;
	}
	//printf ("Done with free\n");
	free(command);
	printf("Done with free\n");
}
int main(int argc, char **argv) {
	printf ("prompt>>  ");
	fflush(stdout);
	char buffer[1024];
	while (fgets(buffer,1024,stdin)!=NULL){
		char ** result=separate_command(buffer);
		int i=0;
		while (result[i]!=NULL){
			printf("%s\n",result[i]);
			i++;
		}
		free_command(result);
	}
    return 0;
}

*/

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


void print_tokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
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
			if (one_command[1]==NULL){
				printf("Current mode is SEQUENTIAL\n");
				free_tokens(one_command);
				i++;
				continue;
			}else if (strcmp(one_command[1],"p")==0 || strcmp(one_command[1],"parallel")==0){
				printf("Mode changed to PARALLEL after all execution of current command\n");
				free_tokens(one_command);
				i++;
				continue;
			}else if(strcmp(one_command[1],"s")==0 || strcmp(one_command[1],"sequential")==0){
				printf("Already in SEQUENTIAL mode\n");
				free_tokens(one_command);
				i++;
				continue;
			}else{
				printf("Invalid command: mode %s\n",one_command[1]);
				free_tokens(one_command);
				i++;
				continue;
			}
		}
		/////EXIT
		else if (strcmp(one_command[0],"exit")==0){
			printf("Exit after execution of the command\n");
			ex=EXIT;
		////JOBS
		}else if (strcmp(one_command[0],"jobs")==0){
			printf("No other jobs running in sequential mode\n");
			free_tokens(one_command);
			i++;
			continue;
		////PAUSE
		}else if (strcmp(one_command[0],"pause")==0){
			printf("Pause is not a valid command in sequential mode\n");
			free_tokens(one_command);
			i++;
			continue;
		////RESUME
		}else if (strcmp(one_command[0],"resume")==0){
			printf("Resume is not a valid command in sequential mode\n");
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
				}else{
					int status;
					pid_t p;
					p=wait(&status);
				}
				//print_tokens(one_command);
			}
		}
		///////FREE one_command/////
		free_tokens(one_command);
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
	
	int mode=SEQUENTIAL;
	while(mode==SEQUENTIAL){
		commands=get_commands();
		sequential(&mode,commands);
		free_tokens(commands);
	}
	
	/*
	char ** dir_list=load_directories();
	char * stop;
	char buffer [1024];
	printf("%s",PROMPT);
	int mode=SEQUENTIAL;
	while (fgets(buffer,1024,stdin)!=NULL){
		buffer[strlen(buffer)-1]='\0';
		stop=strchr(buffer,'#');
		if (stop!=NULL){
			*stop='\0';
		}
		char ** commands=tokenify(buffer,";");
		int i=0;
		while (commands[i]!=NULL){
			char ** one_command=tokenify(commands[i]," \n\t");
			////////CHECK FOR SPECIAL COMMANDS///////
			
			////////CHECK VALIDITY OF COMMAND//////
			struct stat statresult;
			int rv=stat(one_command[0],&statresult);
			if (rv<0){
				rv=command_check(one_command,dir_list);
				if (rv>=0){
					print_tokens(one_command);
				}
				else{
					printf("Invalid command. Try again\n");
				}
			}
			else{
				print_tokens(one_command);
			}
			i++;
			free_tokens(one_command);
		}
		free_tokens(commands);
		printf("%s",PROMPT);
	}
	
	free_directories(dir_list);
    return 0;
    */
}

