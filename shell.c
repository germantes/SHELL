/**
 * Linux Job Control Shell Project
 *
 * Operating Systems
 * Grados Ing. Informatica & Software
 * Dept. de Arquitectura de Computadores - UMA
 *
 * Some code adapted from "OS Concepts Essentials", Silberschatz et al.
 *
 * To compile and run the program:
 *   $ gcc shell.c job_control.c -o shell
 *   $ ./shell
 *	(then type ^D to exit program)
 **/

#include "job_control.h" /* Remember to compile with module job_control.c */

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough */

job *my_job_list;

/**
 * Parse redirections operators '<' '>' once args structure has been built
 * Call immediately after get_command()
 * get_command(...);
 * char *file_in, *file_out;
 * parse_redirections(args, &file_in, &file_out);
 *
 * For a valid redirection, a blank space is required before and after
 * redirection operators '<' or '>'
 **/

void sigchld_handler(int num_sig)
{
	block_SIGCHLD();
	int status, info;
	job_iterator iter = get_iterator(my_job_list);
	pid_t pid_wait;
	enum status status_res;
	while (has_next(iter))
	{
		job *the_job = next(iter);

		pid_wait = waitpid(the_job->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);
		if (pid_wait == the_job->pgid)
		{
			status_res = analyze_status(status, &info);
			printf("Background pid: %d, command: %s, %s, info: %d\n", the_job->pgid, the_job->command, status_strings[status_res], info);
			if (status_res == SUSPENDED)
			{
				the_job->state = STOPPED;
			}
			else if (status_res == CONTINUED)
			{
				the_job->state = BACKGROUND;
			}
			else
			{
				/* status_res == SIGNALED || status_res == EXITED */
				delete_job(my_job_list, the_job);
			}
		}
		else if (pid_wait == -1)
			perror("Wait error from sigchld_handler");
	}
	unblock_SIGCHLD();
}
/**
 * MAIN
 **/
int main(void)
{
	char inputBuffer[MAX_LINE]; /* Buffer to hold the command entered */
	int background;				/* Equals 1 if a command is followed by '&' */
	char *args[MAX_LINE / 2];	/* Command line (of 256) has max of 128 arguments */
	/* Probably useful variables: */
	int pid_fork, pid_wait; /* PIDs for created and waited processes */
	int status;				/* Status returned by wait */
	enum status status_res; /* Status processed by analyze_status() */
	int info;				/* Info processed by analyze_status() */

	my_job_list = new_list("Job List");
	ignore_terminal_signals();
	signal(SIGCHLD, sigchld_handler);

	while (1) /* Program terminates normally inside get_command() after ^D is typed*/
	{
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background); /* Get next command */
		char *file_in, *file_out;
		parse_redirections(args, &file_in, &file_out);

		if (args[0] == NULL)
			continue; /* Do nothing if empty command */
		if (!strcmp(args[0], "cd"))
		{
			chdir(args[1]);
			if (args[1] == NULL)
			{
				printf("Directory not found");
			}
			else
			{
				printf("Directory changed to: %s\n", args[1]);
			}
		}
		else if (!strcmp(args[0], "exit"))
		{
			printf("Bye\n");
			exit(EXIT_SUCCESS);
		}
		else if (!strcmp(args[0], "jobs"))
		{
			if (empty_list(my_job_list))
			{
				printf("No background or suspended jobs.\n");
			}
			else
			{
				print_job_list(my_job_list);
			}
		}
		else if (!strcmp(args[0], "bg"))
		{
			int num = 1;
			if (args[1] != NULL)
			{
				num = atoi(args[1]);
			}
			job *the_job = get_item_bypos(my_job_list, num);
			if (the_job != NULL)
			{
				killpg(the_job->pgid, SIGCONT);
				the_job->state = CONTINUED;
			}
			else
			{
				printf("No valid job");
			}
		}
		else if (!strcmp(args[0], "fg"))
		{
			int num = 1;
			if (args[1] != NULL)
			{
				num = atoi(args[1]);
			}
			job *the_job = get_item_bypos(my_job_list, num);
			if (the_job != NULL)
			{
				// Give terminal to the job's process group
				set_terminal(the_job->pgid);

				// If the job is stopped, send SIGCONT to the process group
				if (the_job->state == STOPPED || the_job->state == SUSPENDED)
				{
					killpg(the_job->pgid, SIGCONT);
				}

				// Change job state to FOREGROUND
				the_job->state = FOREGROUND;

				int status, info;
				enum status status_res;
				// Wait for the job to finish or stop
				int pid_wait = waitpid(the_job->pgid, &status, WUNTRACED);
				set_terminal(getpid());
				if (pid_wait == the_job->pgid)
				{
					status_res = analyze_status(status, &info);
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", the_job->pgid, the_job->command, status_strings[status_res], info);
					if (status_res == SUSPENDED)
					{
						the_job->state = STOPPED;
					}
					else
					{
						// Remove job if exited or signaled
						delete_job(my_job_list, the_job);
					}
				}
				else if (pid_wait == -1)
				{
					perror("Wait error in fg");
				}
			}
			else
			{
				printf("No valid job\n");
			}
		}
		else
		{
			/** The steps are:
			 *	 (1) Fork a child process using fork()
			 *	 (2) The child process will invoke execvp()
			 * 	 (3) If background == 0, the parent will wait, otherwise continue
			 *	 (4) Shell shows a status message for processed command
			 * 	 (5) Loop returns to get_command() function
			 **/
			pid_fork = fork();
			if (pid_fork > 0)
			{ /* We are in the parent */
				new_process_group(pid_fork);
				if (!background) /* Foreground */
				{
					set_terminal(pid_fork);
					pid_wait = waitpid(pid_fork, &status, WUNTRACED);
					set_terminal(getpid());
					if (pid_wait == pid_fork)
					{
						status_res = analyze_status(status, &info);
						if (info != 1)
						{
							// printf("%s pid: %d, command: %s, %s, info: %d\n", state_strings[status_res], pid_fork, args[0], status_strings[status_res], info);
							printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_strings[status_res], info);
						}
						if (status_res == SUSPENDED)
						{
							block_SIGCHLD();
							add_job(my_job_list, new_job(pid_fork, args[0], SUSPENDED)); // In here I should put the job I'm currently on
							unblock_SIGCHLD();
						}
					}
					else if (pid_wait == -1)
					{
						perror("Wait error");
					}
				}
				else
				{ /* Background */
					printf("Background job pid: %d, command: %s\n", pid_fork, args[0]);
					block_SIGCHLD();
					add_job(my_job_list, new_job(pid_fork, args[0], BACKGROUND));
					unblock_SIGCHLD();
				}
			}
			else if (pid_fork == 0)
			{ /* We are in the child */
				new_process_group(getpid());
				if (!background)
					set_terminal(getpid());
				restore_terminal_signals();

				if (file_in != NULL)
				{
					FILE *open_file = fopen(file_in, "r");
					if (open_file == NULL)
					{
						fprintf(stderr, "failed to open the file: '%s'", file_in);
						perror("");
					}
					if (dup2(fileno(open_file), fileno(stdin)) == -1)
					{
						fprintf(stderr, "Error duplicating output file descriptor for '%s': ", file_in);
						perror("");
					}
					else
					{
						fclose(open_file);
					}
				}
				if (file_out != NULL)
				{
					FILE *open_file = fopen(file_out, "w");
					if (open_file == NULL)
					{
						fprintf(stderr, "failed to open the file: '%s'", file_out);
						perror("");
					}
					if (dup2(fileno(open_file), fileno(stdout)) == -1)
					{
						fprintf(stderr, "Error duplicating output file descriptor for '%s': ", file_out);
						perror("");
					}
					else
					{
						fclose(open_file);
					}
				}
				execvp(args[0], args); // if command doesn't exist, execvp keeps going, but another shell is created for the child
				printf("Error, command not found: %s\n", args[0]);
				exit(EXIT_FAILURE);
			}
			else
			{ /* There was an error */
				perror("Fork error");
			}
		}
	} /* End while */
}
