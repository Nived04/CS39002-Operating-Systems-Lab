## Overview: 
Creating multiple processes using C and exploring ways to make them communicate with each other.

### fork(): 
    A separate execution line (child process) is made that executes alongside the main (parent) process.
    Return value: -1 for errors, 0 to the new process, and process ID of the new process to the old process.

Use of fflush(stdout):
    The child and the parent processes print text so fast to the std output that the buffer actually collects 
    all the text and displays it together. To prevent this from happening and letting printf print exactly when it
    is called, it is important to use fflush(stdout), because it will clear the buffer every time 

### wait():
    A function that tells a process to stop execution until one of its child processes exits.
    Return value: (pid_t)-1 for errors, or the pid of the child process.
    - Case 1: Multiple child process, single wait call -> arbitrary child PID returned.
    - Case 2: No child processes, returns -1 (doesn't block execution)