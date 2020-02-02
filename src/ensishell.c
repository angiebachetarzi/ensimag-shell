/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

	/* Remove this line when using parsecmd as it will free it */
	free(line);
	
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}

//structure of list of processes in background
struct proc_jobs {
	pid_t pid;	//pid of the command
	char *cmd; //command to be stored
	int index; //index of process in list of jobs
	struct proc_jobs *next;
};

struct proc_jobs* list_bg_processes;

int add_process_to_jobs(pid_t pid_bg, char *cmd_bg) {
	struct proc_jobs *bg_element = malloc(sizeof (struct proc_jobs));

	bg_element -> pid = pid_bg;
	bg_element -> cmd = malloc((sizeof(cmd_bg) + 1) * sizeof(char));
	strcpy(bg_element -> cmd, cmd_bg);
	bg_element -> next = NULL;

	//if list is empty
	if (list_bg_processes == NULL) {
		bg_element -> index = 1;
		list_bg_processes = bg_element;
	} else {
		//loop through list to get last element
		struct proc_jobs *tmp_element = list_bg_processes;
		while (tmp_element -> next != NULL) {
			(tmp_element -> index)++;
			tmp_element = tmp_element -> next;
		}
		bg_element -> index = (tmp_element -> index) + 1;
		tmp_element -> next = bg_element;
	}
	return bg_element -> index;
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		int i;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {
		  
			terminate(0);
		}
		

		
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		//handle the case where you just press enter (without a command)
		if (l -> seq[0] == NULL) {
			continue;
		}

		//identifying the command jobs in the command line
		if (strcmp(l -> seq[0][0], "jobs") == 0) {
			//loop through list to and display processes
			struct proc_jobs *tmp_element = list_bg_processes;
			while (tmp_element != NULL) {
				printf("[%d] %d, cmd : %s\n", tmp_element -> index, tmp_element -> pid, tmp_element -> cmd);
				tmp_element = tmp_element -> next;
			}
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) //printf("background (&)\n");

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			pid_t pid1, pid2;;
			int status;
			pid1 = fork();
			switch (pid1) {
				case -1 : 
					perror("error in fork\n");
					break;
				case 0 :
				//case we have one more command
				//example : |
				if (l -> seq[1] != NULL) {
					pid2 = fork();
					switch (pid2) {
						
					}
				}
					execvp(cmd[0], cmd);
					break;
				default :
				if (!l -> bg) {
					waitpid(pid1, &status, 0);
				} else {
					//create a new process and add it to a list of processes
					int index = add_process_to_jobs(pid1, *cmd);
					printf("[%d] %d\n", index, pid1);
				}
					break;

			}
		}
	
	}

}
