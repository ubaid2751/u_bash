#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

void ubash_copy_cmd(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUF_SIZE];
    ssize_t bytes_read, bytes_written;
    printf("\n");

    src_fd = open(source, O_RDONLY);
    if (src_fd < 0) {
        perror("Error opening source file");
        exit(EXIT_FAILURE);
    }

    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Error opening destination file");
        close(src_fd);
        return;
    }

    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error writing to destination file");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (bytes_read < 0) {
        perror("Error reading source file");
    }

    close(src_fd);
    close(dest_fd);
}

void ubash_cat_cmd(const char *source, size_t *line) {
    int src_fd;
    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    int col = 0;

    src_fd = open(source, O_RDONLY);
    if(src_fd < 0) {
        mvprintw((*line)++, 0, "Error opening source file: %s\n", source);
        return;
    }

    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                (*line)++;
                col = 0;
            } else {
                mvprintw(*line, col++, "%c", buffer[i]);
            }
        }
    }

    if (bytes_read < 0) {
        mvprintw((*line)++, 0, "Error reading source file: %s\n", source);
    }
    (*line)++;

    close(src_fd);
    refresh();
}