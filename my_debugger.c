/* Code sample: using ptrace for simple tracing of a child process.
**
** Note: this was originally developed for a 32-bit x86 Linux system; some
** changes may be required to port to x86-64.
**
** Eli Bendersky (http://eli.thegreenplace.net)
** This code is in the public domain.
*/
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

pid_t run_target_with_args(const char* programname, char* const argv[])
{
	pid_t pid;
	
	pid = fork();
	
    if (pid > 0) {
		return pid;
		
    } else if (pid == 0) {
		/* Allow tracing of this process */
		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			perror("ptrace");
			exit(1);
		}
		/* Replace this process's image with the given program */
        execv(programname, argv);
        printf("FAIL!\n");
        exit(0);

		
	} else {
		// fork error
		perror("fork");
        exit(1);
    }
}

void run_redirection_debugger(pid_t child_pid, int fd,unsigned long start_addr, int copy)
{
    int wait_status, iwrote = 0;

    /* Wait for child to stop on its first instruction */
    wait(&wait_status);
    struct user_regs_struct regs;
    unsigned long data, data2, data_trap, data2_trap, ret_addr, orig_fd;

    if(!WIFEXITED(wait_status)){
        /* Get the fisrt line in the function */
        data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)start_addr, NULL);

        /* Write the trap instruction 'int 3' into the address */
        data_trap = (data & 0xFFFFFFFFFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data_trap); 
        
        /* Run until the function is called */
        ptrace(PTRACE_CONT, child_pid, 0, 0);  
        wait(&wait_status);   
    }

    while(!WIFEXITED(wait_status)){
        /* See where child is now */
        ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
        
        /* Get the function return addres */
        ret_addr = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)(regs.rsp), NULL);
        
        /* Put a breakpoint at the return address of the function */
        data2 = ptrace(PTRACE_PEEKTEXT, child_pid, (void*) ret_addr, NULL);
        data2_trap = (data2 & 0xFFFFFFFFFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2_trap); 
        
        /* Remove the breakpoint in foo by restoring the previous data */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

        /* Execute one line of code */
        ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        wait(&wait_status);
        /* Put back the breakpoint */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data_trap); 
        
        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);

        while(1){
            iwrote = 0;
            /* The child can continue running now, stop only at syscall or ret */
            if(ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0){
                perror("ptrace1");
                return;
            }
            wait(&wait_status);

            /* Get the current regs struct */
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);

            /* Break if we returned from the function */
            if(regs.rip - 1 == ret_addr) break;
            if(regs.orig_rax == 1){
                write(fd, "PRF:: ", 6);
                orig_fd = regs.rdi;
                regs.rdi = fd;
                ptrace(PTRACE_SETREGS, child_pid, 0, &regs);
                iwrote = 1;
            }

            /* Run system call and stop */
            if(ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0){
                perror("ptrace2");
                return;
            }
            wait(&wait_status);

            if(iwrote == 1 && copy == 1){
                iwrote = 0;
                regs.rdi = orig_fd;
                regs.rip -= 2;
                regs.rax = 1;
                ptrace(PTRACE_SETREGS, child_pid, 0, &regs);                
                
                ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
                wait(&wait_status); 
                
                ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
                wait(&wait_status); 
            }
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
            
            /* Break if we returned from the function */
            if(regs.rip - 1 == ret_addr) break;

        }
        /* Remove the breakpoint in foo return address by restoring the previous data(2) */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

        /* Continue running the child process until next break or exit*/
        ptrace(PTRACE_CONT, child_pid, NULL, NULL);
        wait(&wait_status);
    }
    

}
int main(int argc, char** argv)
{
    unsigned long addr = strtol(argv[1], NULL, 16);
    int copy = (*argv[2] == 'c');
    char* fname = argv[3];
    char* args[argc-3];
    for(int i = 4; i < argc; i++){
        args[i-4] = argv[i];
    }
    argv[argc-4] = NULL; 
    int fd = open(fname, O_CREAT | O_WRONLY , 0777);

    pid_t c_pid = run_target_with_args(args[0], args);
    run_redirection_debugger(c_pid, fd, addr, copy);

    close(fd);
    return 0;
}
