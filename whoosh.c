#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_CMD_LINE_LENGTH 128

/* Have function for print error message */
void print_error_msg() {

  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));

}

void execute_cd(char *argv[], int size) {

  /* "When you run cd (without arguments), your shell
     should change the working directory to the path
     stored in the $HOME environment variable."
  */
  if (size == 1) {
    
    /* "Use the call getenv("HOME") in your source code
       to obtain this value" */
    char* home_directory = getenv("HOME");
    
    if (chdir(home_directory) != 0)
      print_error_msg();
    
  }

  /* Have a specific directory to change to */
  /* "You do not have to support tilde (~). You should
     treat it like a common character, i.e., you should
     just pass the whole word (e.g. "~username") to
     chdir(), and chdir will return an error."
  */
  else if (size == 2) {

    if(chdir(argv[1]) != 0)
      print_error_msg();

  }

  /* Safety net for anything out of ordinary */
  else
    print_error_msg();

}

void execute_pwd(char* argv[], int size, int redirection) {
  
  /* pwd command only one argument. No redirection */
  if (!redirection && size != 1) {
    
    print_error_msg();
    return;

  }
  /* "Basically, when a user types pwd, you simply call
     getcwd(), and show the result."
  */
  char work_directory[500];
  if (getcwd(work_directory, 500) != NULL) {

    int work_directory_length = strlen(work_directory);
   
    work_directory[work_directory_length] = '\n';
    work_directory[work_directory_length + 1] = '\0';
    write(STDOUT_FILENO, work_directory, strlen(work_directory));

  }

  /* Once again, failsafe */
  else {

    print_error_msg();
    exit(1);

  }
}

void execute_ls(char *argv[], const char *ls) {

  execvp(ls, argv);

  /* "Remember that if execv() is successful, it will 
     not return; if it does return, there was an error 
     (e.g., the command does not exist)"
  */
  print_error_msg();
  exit(1);

}

void execute_path(char *argv[], int size) {

  if (size >= 2) {

    char path[500];
    int i = 1;

    strcpy(path, "$PATH");

    while (i < size) {

      strcpy(path, ":");
      strcpy(path, argv[i]);
      i++;

    }

    if (setenv("PATH", path, 1) != 0) {

      print_error_msg();
      exit(1);

    }
  }

  else {
    
    print_error_msg();
    exit(1);

  }
    
}

void command_interpreter(char *arguments[MAX_CMD_LINE_LENGTH * 10], 
			 int size, int background_proc,
			 int redirection) {

  int i;
  char *argv[size + 1];

  /* Convert arguments over to an argv type setup 
     (easier to pass to function call) 
  */
  for (i = 0; i < size; i++)
    argv[i] = arguments[i];

  argv[i] = NULL;

  //  printf("argv[0]: %s, argv[1]: %s, argv[2]: %s\n", argv[0], argv[1], argv[2]);
  fpos_t std_out;
  fpos_t std_err;
  
  fgetpos(stdout, &std_out);
  fgetpos(stderr, &std_err);
  
  int std_out_tmp = dup(fileno(stdout));
  int std_err_tmp = dup(fileno(stderr));

  if (redirection) {

    if ((strcmp(arguments[0], "exit") == 0) ||                
	(strcmp(arguments[0], "ls") == 0) ||                          
	(strcmp(arguments[0], "cd") == 0) ||                          
	(strcmp(arguments[0], "pwd") == 0) ||                         
	(strcmp(arguments[0], "path") == 0)) {                        
                                                                        
    }                                                                 
    
    else {                                                            
      print_error_msg();                                              
      return;                                                       
    }
      
    int j = size;

    while (j > 1) {

      if (strchr(argv[j - 1], '/') != NULL) {
      	print_error_msg();
	return;
      }
      
      j--;
    }
    
    char *std_out_file = strcpy(argv[i - 1], ".out");
    char *std_err_file = strcpy(argv[i - 1], ".err");

    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (access(std_out_file, F_OK) == -1) {
      print_error_msg();
      return;
    }

    if (access(std_err_file, F_OK) == -1) {

      print_error_msg();
      return;
    }

    int open_file_stdout = open(std_out_file,  O_CREAT | O_RDWR | O_TRUNC);

    int open_file_stderr = open(std_err_file, O_CREAT | O_RDWR | O_TRUNC);
    argv[i - 1] = NULL;

    if (open_file_stdout == -1) {
      print_error_msg();
      return;

    }

    if (open_file_stderr == -1) {

      print_error_msg();
      return;
    }
  }

  /* Change directory: "cd" */
  if (strcmp(arguments[0], "cd") == 0) {

    execute_cd(argv, size);

  }

  /* Exit: "exit" */
  else if (strcmp(arguments[0], "exit") == 0) {
    
    if (size > 1) {
      print_error_msg();
      exit(1);
    }
 
    exit(0);

  }

  /* Change $PATH env variable: "path" */
  else if (strcmp(arguments[0], "path") == 0) {

    execute_path(argv, size);

  }

  /* Print working directory: "pwd" */
  else if (strcmp(arguments[0], "pwd") == 0) {
    
    execute_pwd(argv, size, redirection);

  }

  /* Probably ls with/without redirection */
  else {

    /* Always fork */
    int pid = fork();

    /* If background & child */
    if (pid >= 0) {
     
      /* Child so do ls */
      if (pid == 0) {
	
	execute_ls(argv, arguments[0]);

      }

      /* Check if background (parent) */
      else {

	if (!background_proc) {
	  
	  wait(NULL);

	}
      
      }

    }
    
    /* Safety net */
    else {
      
      print_error_msg();
      exit(1);
      
    }

  }

  /* Reinstate everything */
  if (redirection) {

    fflush(stdout);
    fflush(stderr);

    dup2(std_out_tmp, fileno(stdout));
    dup2(std_err_tmp, fileno(stderr));

    close(std_out_tmp);
    close(std_err_tmp);

    clearerr(stdout);
    clearerr(stderr);

    fsetpos(stdout, &std_out);
    fsetpos(stderr, &std_err);

  }

}

int main(int argc, char const *argv[]) {

  FILE *input_arg = NULL;
  
  // Check if incorrect number of arguments
  if (argc < 1) {

    print_error_msg();
    exit(1);

  }

  // Possibly have exit, pwd, cd, path
  if (argc == 2) {

    /* "Redirction is relatively easy to implement
       For example, to redirect standard output to a
       File, just use close() on stdout, and then
       open() on a file."
    */
    close(fileno(stdin));
    input_arg = fopen(argv[1], "r");

    /* "If the output file is not specified (e.g. the 
       user types ls >), you should print an error
       message and not run the program ls."
    */
    if (input_arg == NULL) {

      print_error_msg();
      exit(1);

    }

    dup2(fileno(input_arg), STDIN_FILENO);

  }

  int to_infinity_and_beyond = 1;

  /* Loop forever (mimic shell) */
  while (to_infinity_and_beyond) {

    char output[MAX_CMD_LINE_LENGTH] = ""; /* Line to output to STDOUT */

    /* All commands are 2+ so keep printing whoosh */
    //if (argc < 2)
    strcat(output, "whoosh> ");

    /* Made this arbitraily big */
    char this_input_line[MAX_CMD_LINE_LENGTH * 10];

    write(STDOUT_FILENO, output, strlen(output));

    /* Loop through all input */
    if (fgets(this_input_line, (MAX_CMD_LINE_LENGTH * 10), 
	      stdin) != NULL) {

      if (argc == 2)
	write(STDOUT_FILENO, this_input_line, strlen(this_input_line));

      /* "For the following situation, you should print the 
	 error message to stderr and CONTINUE processing:
	 A very long command line (over 128 bytes)
      */
      if ((strlen(this_input_line) - 1) > MAX_CMD_LINE_LENGTH) {

	print_error_msg();
	continue;

      }
      
      char *new_line = this_input_line;

      /* Trim newline character */
      char *trim_newline = strtok(this_input_line, "\n");

      if (trim_newline == NULL)
	continue;

      /* Trim whitespace at beginning and end */
      char *trim_wspace = trim_newline;
      char *end = trim_wspace + strlen(trim_wspace) - 1;
      
      while (isspace(*trim_wspace))
	trim_wspace++;

      /* Line is all spaces */
      if (*trim_wspace == 0)
	continue;
      
      while(end > trim_wspace && isspace(*end)) 
	end--;
     
      int background_proc = 0; 
      int redirection = 0;

      /* Redirection: Is there a '>' character? */
      char *rarrow = strchr(trim_wspace, '>');

      /* If there is redirection */
      if (rarrow != NULL) {

	*(rarrow++) = ' ';
	redirection = 1;

	char* multi_space = strtok(rarrow, " ");

	if (multi_space == NULL) {

	    print_error_msg();
	    continue;

	}

	int count_spaces = 0;

	while (multi_space != NULL) {
	
	  if (strcmp(multi_space, "&") != 0)
	    count_spaces++;

	  multi_space = strtok(NULL, " ");
	
	}

	if (count_spaces > 1) {

	  print_error_msg();
	  continue;

	}
	
      }

      /* No redirection */
      else
	redirection = 0;
      
      /* Begin parsing argument(s) to pass to interpreter */
      char *arguments[MAX_CMD_LINE_LENGTH * 2];
      char space[] = " ";
      char tab[] = "\t";
      char *space_ret, *tab_ret;
      char *parse_arguments = strtok_r(trim_wspace, space, &space_ret);
 
      int size = 0;

      while (parse_arguments != NULL) {
      
	char *temp;

	/* Tab? */
	tab_ret = strtok_r(parse_arguments, tab, &temp);
	
	while (tab_ret != NULL) {
	
	  arguments[size++] = tab_ret;
	  tab_ret = strtok_r(NULL, tab, &temp);
	
	}
	
	parse_arguments = strtok_r(NULL, space, &space_ret);
      
      }

      command_interpreter(arguments, size, background_proc, redirection);
    }

    else
      break;

  }

  return 0;
}
