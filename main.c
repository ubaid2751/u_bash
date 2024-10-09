#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<ncurses.h>
#include<unistd.h>
#include<errno.h>
#include<assert.h>
#include<sys/wait.h>
#include"file_commands.h"

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[ubash]$ "
#define ENTER 10
#define DOWN_ARROW 258
#define UP_ARROW 259

#define DATA_START_CAPACITY 128

#define ASSERT(cond, ...) \
        do { \
            if (!(cond)) { \
                fprintf(stderr, "%s:%d: ASSERTION FAILED: ", __FILE__, __LINE__); \
                fprintf(stderr, __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                exit(1); \
            } \
        } while (0)

#define DA_APPEND(da, item) do {                                                       \
    if ((da)->count >= (da)->capacity) {                                               \
        (da)->capacity = (da)->capacity == 0 ? DATA_START_CAPACITY : (da)->capacity*2; \
        void *new = calloc(((da)->capacity+1), sizeof(*(da)->data));                   \
        ASSERT(new,"outta ram");                                                       \
        if ((da)->data != NULL)                                                        \
            memcpy(new, (da)->data, (da)->count);                                      \
        free((da)->data);                                                              \
        (da)->data = new;                                                              \
    }                                                                                  \
    (da)->data[(da)->count++] = (item);                                                \
} while (0)

#define CHECK_CLEAR(command, line) \
    ((command.count == 5 && strncmp((command).data, "clear", 5) == 0) ? \
    (clear(), (*(line) = 0), move(*(line), 0), refresh(), (command) = (String){0}, true) : false)
// 
// #define CHECK_EXIT(command, line) \
	// ((command.count >= 4 && strncmp((command).data, "exit", 4) == 0) ? true : false)

typedef struct {
	char *data;
	size_t count;
	size_t capacity;
} String;

typedef struct {
	String *data;
	size_t count;
	size_t capacity;
} Strings;

char *str_to_cstr(String str) {
	char *cstr = malloc(sizeof(char) * str.count + 1);
	memcpy(cstr, str.data, sizeof(char) * str.count);
	cstr[str.count] = '\0';
	return cstr;
}

char **parse_command(char *command) {
	char *cur = strtok(command, " ");
	if(cur == NULL) {
		return NULL;
	}
	size_t args_s = 8;
	char **args = malloc(sizeof(char*)*args_s);
    if (args == NULL) {
        return NULL;
    }
	size_t args_cur = 0;
	while(cur != NULL) {
		if(args_cur+2 >= args_s) {
			args_s *= 2;
			args = realloc(args, sizeof(char*)*args_s);
		}

		while(command[0] != '\0') command++;
		command++;
		assert(command);
		if(command[0] == '\'') {
			command++;
			args[args_cur++] = cur;
			cur = command;
			args[args_cur++] = cur;
			while(command[0] != '\'' && command[0] != '\0') command++;
			if(command[0] == '\0') break;
			command[0] = '\0';
			command++;
			cur = strtok(command, " ");
			continue;
		}
		
		args[args_cur++] = cur;
		cur = strtok(NULL, " ");
	}

	args[args_cur] = NULL;

	return args;
}

void execute_command(char **args, size_t *line) {
	int filedes[2];
	char buf[4096] = {0};
	if(pipe(filedes) < 0) {
		mvprintw(*line, 0, "Error: %s\n", strerror(errno));
		return;
	}

	int pid = fork();
	int status;
	if(pid < 0) {
		mvprintw(*line, 0, "Error: %s\n", strerror(errno));
		return;
	}
	else if(!pid) {
		close(filedes[0]);
		if(dup2(filedes[1], STDOUT_FILENO) < 0) {
			printf("Error: %s\n", strerror(errno));
			return;
		}
		close(filedes[1]);

		if(execvp(args[0], args) < 0) {
			printw("Error: %s", strerror(errno));
		}
		exit(1);
	}
	else {
		close(filedes[1]);
		
		int nbytyes = 0;
		while((nbytyes = read(filedes[0], buf, sizeof(buf) - 1)) != 0) {
			mvprintw(*line, 0, "%s", buf);
			for (size_t i = 0; buf[i] != '\0'; i++) {
				if(buf[i] == '\n') (*line)++;
			}
			refresh();
			memset(buf, 0, sizeof(buf));
		}
		
		pid_t wpid = waitpid(pid, &status, 0);
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
		(void)wpid;

		close(filedes[0]);
		refresh();
	}
}

void handle_command(char **args, size_t *line) {
	*line += 1;

	if(*args == NULL) {
		mvprintw(*line, 0, "Error: No command entered\n");
		return;
	}
	if(strcmp(args[0], "exit") == 0) {
		int exit_code = 0;
		if(args[1] != NULL) {
			exit_code = strtol(args[1], NULL, 10);
		}

		endwin();
		printf("exit\n");
		exit(exit_code);
	}
	else if(strcmp(args[0], "cd") == 0) {
		char *dir = "~";
		if(args[1] != NULL) {
			dir = args[1];
		}

		if(chdir(dir) < 0) {
			mvprintw(*line, 0, "'%s': %s\n", dir, strerror(errno));
			*line += 1;
		}
	}
	else {
		execute_command(args, line);
	}
}

int main() {
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	bool QUIT = false;

	int ch;
	size_t line = 0;
	size_t command_max = 0;
	size_t command_pos = 0;

	size_t height = 0, width = 0;
	(void)height;

	String command = {0};
	Strings command_his = {0};

	while(!QUIT) {
		getmaxyx(stdscr, height, width);
		mvprintw(line, 0, SHELL);
		mvprintw(line, sizeof(SHELL) - 1, "%.*s", (int)command.count, command.data);
		
		// move(line, sizeof(SHELL) - 1 + command_pos);

		ch = getch();
		switch(ch) {
			case ctrl('q'):
				QUIT = true;
				break;

			case ENTER:
				line++;
				
				char **args = NULL;
				if (command.count > 0) {
					args = parse_command(str_to_cstr(command));
				}

				mvprintw(line, command.count, "\n\r");
			
				if (CHECK_CLEAR(command, &line)) break;
				// if (CHECK_EXIT(command, &line)) QUIT=true;

				if(args != NULL) {
					handle_command(args, &line);
					DA_APPEND(&command_his, command);
					if(command_his.count > command_max) command_max = command_his.count;
				}
				command = (String){0};
				command_pos = 0;
				break;
			
			case KEY_BACKSPACE:
				if(command.count > 0) {
					command.count--;
					move(line, sizeof(SHELL) - 1 + command.count);
					clrtoeol();
				}
				break;

			case UP_ARROW:
				if(command_his.count > 0) {
					move(line, sizeof(SHELL) - 1);
					clrtoeol();

					command_his.count--;
					command = command_his.data[command_his.count];
				}
				break;

			case DOWN_ARROW:
				if(command_his.count < command_max) {
					move(line, sizeof(SHELL) - 1);
					clrtoeol();

					command_his.count++;
					command = command_his.data[command_his.count];
				}
				break;

			default:
				DA_APPEND(&command, ch);
				break;
		}
		refresh();
	}

	endwin();

	for (size_t i = 0; i < command_his.count; i++) {
		printf("%.*s\n", (int)command_his.data[i].count, command_his.data[i].data);
		free(command_his.data[i].data);
	}
	
	return 0;
}