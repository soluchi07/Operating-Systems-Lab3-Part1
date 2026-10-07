// C program demonstrating 2-way IPC using fork() and pipe()
#define _DEFAULT_SOURCE
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h> 

int main() 
{ 
    // fd1: P1 -> P2
    // fd2: P2 -> P1
    int fd1[2]; 
    int fd2[2]; 

    char fixed_str_p2[] = "howard.edu";
    char fixed_str_p1[] = "gobison.org";
    char p1_initial_input[100]; 
    char p2_extra_input[100];
    char concat_str[200];
    pid_t p; 

    // Create IPC Pipes
    if (pipe(fd1) == -1) 
    { 
        perror("Pipe 1 creation failed"); 
        return 1; 
    } 
    if (pipe(fd2) == -1) 
    { 
        perror("Pipe 2 creation failed"); 
        return 1; 
    } 

    // Prompt P1 user input
    printf("Input: ");
    if (scanf("%99s", p1_initial_input) != 1) {
        fprintf(stderr, "Failed to read input\n");
        return 1;
    }
    
    p = fork(); 

    if (p < 0) 
    { 
        perror("Fork failed"); 
        return 1; 
    } 

    // =======================================================
    // Parent Process (P1)
    // =======================================================
    else if (p > 0) 
    { 
        close(fd1[0]);  // Close unused read end of Pipe 1
        close(fd2[1]);  // Close unused write end of Pipe 2

        // Send user string to P2 via Pipe 1
        if (write(fd1[1], p1_initial_input, strlen(p1_initial_input) + 1) < 0) {
            perror("P1 Write to Pipe 1 failed");
            return 1;
        }
        close(fd1[1]); // Close write end to signal EOF to P2

        // Receive modified string back from P2 via Pipe 2
        if (read(fd2[0], concat_str, sizeof(concat_str)) < 0) {
            perror("P1 Read from Pipe 2 failed");
            return 1;
        }
        close(fd2[0]); // Finished reading from Pipe 2

        // Reap child process
        wait(NULL); 

        // Append final suffix in P1
        strcat(concat_str, fixed_str_p1);

        // P1 Final Output
        printf("Output: %s\n", concat_str);
    } 

    // =======================================================
    // Child Process (P2)
    // =======================================================
    else
    { 
        close(fd1[1]); // Close unused write end of Pipe 1
        close(fd2[0]); // Close unused read end of Pipe 2

        // Read initial string from P1
        if (read(fd1[0], concat_str, sizeof(concat_str)) < 0) {
            perror("P2 Read from Pipe 1 failed");
            exit(1);
        }
        close(fd1[0]); // Finished reading from Pipe 1

        // Step 1: Append "howard.edu" and display P2 output
        strcat(concat_str, fixed_str_p2);
        printf("Output: %s\n", concat_str);

        // Step 2: Prompt for P2 extra string input
        printf("Input: ");
        if (scanf("%99s", p2_extra_input) != 1) {
            fprintf(stderr, "Failed to read second input\n");
            exit(1);
        }

        // Append P2 second string
        strcat(concat_str, p2_extra_input);

        // Send updated string back to P1 via Pipe 2
        if (write(fd2[1], concat_str, strlen(concat_str) + 1) < 0) {
            perror("P2 Write to Pipe 2 failed");
            exit(1);
        }
        close(fd2[1]); // Finished writing to Pipe 2

        exit(0); 
    } 

    return 0;
}