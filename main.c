#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<ncurses.h>
#include<unistd.h>
#include<errno.h>
#include<sys/wait.h>

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
    if ((da)->count >= (da)->capacity) {                                                 \
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

#define CHECK_EXIT(command, line) \
	((command.count >= 4 && strncmp((command).data, "exit", 4) == 0) ? true : false)

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

void handle_command(char *file, char **args) {
	int pid = fork();
	int status;

	if(pid < 0) {
		printw("Error %s", strerror(errno));
		return;
	}
	else if(pid == 0) {
		if(execvp(args[0], args) < 0) {
			printw("error %s", strerror(errno));
		}
	}
	else {
		pid_t wpid = waitpid(pid, &status, 0);

		(void)wpid;
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
	}
}

char *str_to_cstr(String command) {
	
}

char **parse_command(char *command) {
	char *cur = strtok(command, " ");
	size_t args_s = 8;
	char **args = malloc(sizeof(char*)*args_s);
	size_t args_cur = 0;

	while(*cur != '\0') {
		if(args_cur >= args_s) {
			args_s *= 2;
			args = realloc(args, sizeof(char*)*args_s);
		}
		args[args_cur++] = cur;
		printf("%s\n", cur);
		cur = strtok(command, "\0");
	}

	for (size_t i = 0; i < args_cur; i++) {
		printf("%s\n", args[i]);
	}
	exit(1);
	return args;
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

	String command = {0};
	Strings command_his = {0};

	while(!QUIT) {
		mvprintw(line, 0, SHELL);
		mvprintw(line, sizeof(SHELL) - 1, "%.*s", (int)command.count, command.data);
		ch = getch();
		switch(ch) {
			case ctrl('q'):
				QUIT = true;
				break;

			case KEY_ENTER: 
				break;

			case ENTER:
				line++;
				
				if (CHECK_CLEAR(command, &line)) break;
				if (CHECK_EXIT(command, &line)) QUIT=true;

				char **args = parse_command(str_to_cstr(command));
				handle_command(args[0], args);
				line++;
				DA_APPEND(&command_his, command);
				if(command_his.count > command_max) command_max = command_his.count;
				command = (String){0};
				break;

			case UP_ARROW:
				if(command_his.count != 0) {
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
