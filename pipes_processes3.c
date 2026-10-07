#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * Executes the pipeline: "cat scores | grep <argument> | sort"
 * 
 * Process Hierarchy:
 * - P1 (Parent): Executes "cat scores"
 * - P2 (Child of P1): Executes "grep <argument>"
 * - P3 (Child of P2 / Child's Child): Executes "sort"
 */

int main(int argc, char **argv)
{
    // 1. Command-Line Argument Verification
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <search_term>\n", argv[0]);
        return 1;
    }

    int pipefd1[2];
    int pipefd2[2];
    pid_t pid1, pid2;

    char *cat_args[]  = {"cat", "scores", NULL};
    char *grep_args[] = {"grep", argv[1], NULL};
    char *sort_args[] = {"sort", NULL};

    // Create Pipe 1 (P1 -> P2)
    if (pipe(pipefd1) == -1) {
        perror("Pipe 1 creation failed");
        return 1;
    }

    pid1 = fork();

    if (pid1 < 0) {
        perror("Fork 1 failed");
        close(pipefd1[0]);
        close(pipefd1[1]);
        return 1;
    }

    if (pid1 == 0) {
        // --- Child Process P2 (grep) ---

        // Create Pipe 2 (P2 -> P3)
        if (pipe(pipefd2) == -1) {
            perror("Pipe 2 creation failed");
            close(pipefd1[0]);
            close(pipefd1[1]);
            exit(1);
        }

        pid2 = fork();

        if (pid2 < 0) {
            perror("Fork 2 failed");
            close(pipefd1[0]);
            close(pipefd1[1]);
            close(pipefd2[0]);
            close(pipefd2[1]);
            exit(1);
        }

        if (pid2 == 0) {
            // --- Child's Child Process P3 (sort) ---
            
            // Redirect stdin to read end of Pipe 2
            if (dup2(pipefd2[0], STDIN_FILENO) == -1) {
                perror("dup2 failed in sort process");
                close(pipefd1[0]); close(pipefd1[1]);
                close(pipefd2[0]); close(pipefd2[1]);
                exit(1);
            }

            // Close all unused pipe descriptors in P3
            close(pipefd1[0]);
            close(pipefd1[1]);
            close(pipefd2[0]);
            close(pipefd2[1]);

            // Execute "sort"
            execvp("sort", sort_args);
            perror("execvp sort failed");
            exit(1);
        } else {
            // --- Child Process P2 (grep <arg>) ---

            // Redirect stdin to read end of Pipe 1
            if (dup2(pipefd1[0], STDIN_FILENO) == -1) {
                perror("dup2 stdin failed in grep process");
                close(pipefd1[0]); close(pipefd1[1]);
                close(pipefd2[0]); close(pipefd2[1]);
                exit(1);
            }

            // Redirect stdout to write end of Pipe 2
            if (dup2(pipefd2[1], STDOUT_FILENO) == -1) {
                perror("dup2 stdout failed in grep process");
                close(pipefd1[0]); close(pipefd1[1]);
                close(pipefd2[0]); close(pipefd2[1]);
                exit(1);
            }

            // Close all pipe descriptors in P2 before executing/waiting
            close(pipefd1[0]);
            close(pipefd1[1]);
            close(pipefd2[0]);
            close(pipefd2[1]);

            // Execute "grep <arg>"
            execvp("grep", grep_args);
            perror("execvp grep failed");
            
            // If execvp fails, ensure child P3 doesn't stay as a zombie
            waitpid(pid2, NULL, 0);
            exit(1);
        }
    } else {
        // --- Parent Process P1 (cat scores) ---

        // Redirect stdout to write end of Pipe 1
        if (dup2(pipefd1[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout failed in cat process");
            close(pipefd1[0]);
            close(pipefd1[1]);
            waitpid(pid1, NULL, 0);
            return 1;
        }

        // Close both pipe descriptors in P1
        close(pipefd1[0]);
        close(pipefd1[1]);

        // Execute "cat scores"
        execvp("cat", cat_args);
        perror("execvp cat failed");

        // Clean up child process if execvp fails
        waitpid(pid1, NULL, 0);
        return 1;
    }

    return 0;
}