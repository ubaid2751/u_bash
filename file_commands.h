#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

void ubash_copy_cmd(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUF_SIZE];
    ssize_t bytes_read, bytes_written;

    src_fd = open(source, O_RDONLY);
    if (src_fd < 0) {
        perror("Error opening source file");
        exit(EXIT_FAILURE);
    }

    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Error opening destination file");
        close(src_fd);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error writing to destination file");
            close(src_fd);
            close(dest_fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read < 0) {
        perror("Error reading source file");
    }

    close(src_fd);
    close(dest_fd);
}