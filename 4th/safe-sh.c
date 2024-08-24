#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *FILE1 = "/etc/passwd";
static const char *FILE2 = "r.txt";

/*
 * Implement the function of command
 * ```bash grep -v usr < FILE1```
 * while writing to the pipe
 */
void subroutine11(int pipefd[2]) {
  close(pipefd[0]);
  if (dup2(pipefd[1], 1) == -1) {
    perror("dup2");
    exit(EXIT_FAILURE);
  }
  close(pipefd[1]);

  int fd = open(FILE1, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  if (dup2(fd, 0) == -1) {
    perror("dup2");
    exit(EXIT_FAILURE);
  }
  close(fd);

  execlp("grep", "grep", "-v", "usr", (const char *)NULL);
}

/*
 * Implement the function of command
 * ```bash wc -l > FILE2```
 * while reading from the pipe
 */
void subroutine12(int pipefd[2]) {
  close(pipefd[1]);
  if (dup2(pipefd[0], 0) == -1) {
    perror("dup2");
    exit(EXIT_FAILURE);
  }
  close(pipefd[0]);

  int fd = open(FILE2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  if (dup2(fd, 1) == -1) {
    perror("dup2");
    exit(EXIT_FAILURE);
  }
  close(fd);

  execlp("wc", "wc", "-l", (const char *)NULL);
}

/*
 * Implement the function of command
 * ```bash grep -v usr < FILE1 | wc –l > FILE2```
 */
void subroutine1(void) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    subroutine11(pipefd); // grep -v usr < FILE1 |
  } else {
    subroutine12(pipefd); // | wc -l > FILE2
  }
}

/*
 * Implement the function of command
 * ```bash cat FILE2```
 */
void subroutine2(void) {
  execlp("cat", "cat", FILE2, (const char *)NULL);
}

/*
 * Implement the function of command
 * ```bash grep -v usr < FILE1 | wc –l > FILE2; cat FILE2```
 */
int main(void) {
  int pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    subroutine1(); // grep -v usr < FILE1 | wc –l > FILE2
  } else {
    int status;
    wait(&status);
    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
      exit(EXIT_FAILURE);
    }

    subroutine2(); // cat FILE2
  }
}
