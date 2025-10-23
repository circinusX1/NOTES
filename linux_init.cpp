#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// This program is designed to be compiled and linked as /sbin/init (PID 1).
// It performs a disk clone operation and then attempts to start a shell.

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    // --- 1. Disk Cloning Operation ---
    printf("Minimal Init (PID 1) starting disk clone operation...\n");
    printf("Cloning /dev/mmcblkp2 to /dev/mmcblkp1 using dd.\n");
    printf("This may take some time. Please wait for completion message.\n");

    // Fork a child process to execute the dd command
    pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork failed");
        // Fall through to start shell anyway
    } else if (pid == 0) {
        // Child process: Execute the dd command
        
        // Command: dd if=/dev/mmcblkp2 of=/dev/mmcblkp1 bs=4M
        // Note: 'status=progress' is often unavailable in minimal init environments.
        // We use execlp for a direct execution without invoking a shell interpreter first.
        // We use 4M block size for better performance.
        
        execlp("/bin/dd", "dd", 
               "if=/dev/mmcblkp2", 
               "of=/dev/mmcblkp1", 
               "bs=4M", 
               (char *)NULL);
               
        // If execlp returns, it means an error occurred (e.g., dd not found)
        perror("dd execution failed");
        exit(EXIT_FAILURE);
        
    } else {
        // Parent process (PID 1): Wait for the child (dd process) to finish
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
        }
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("\nCloning completed successfully (dd exit code 0).\n");
        } else {
            fprintf(stderr, "\nCloning failed or completed with non-zero status (%d).\n", WEXITSTATUS(status));
        }
    }

    // --- 2. Fallback to Shell ---
    // The init process (PID 1) MUST NOT exit, so we replace it with a shell.
    printf("Disk clone complete. Replacing init (PID 1) with /bin/sh...\n");
    
    // Attempt to execute a shell. We use execl to replace the current process.
    execl("/bin/sh", "sh", (char *)NULL);
    
    // If the shell launch fails (e.g., /bin/sh not found)
    perror("Failed to execute /bin/sh. Kernel panic imminent.");
    
    // In a real scenario, an infinite loop would be here if the final exec failed, 
    // to prevent the kernel panic, but logging the failure is more helpful.
    
    // Since PID 1 is designed to run forever, we enter an infinite loop if everything else fails.
    while(1) {
        sleep(1);
    }
    
    // Should never be reached.
    return EXIT_FAILURE; 
}
