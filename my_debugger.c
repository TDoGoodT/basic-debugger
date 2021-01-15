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
		assert(programname == argv[0]);
        execvp(programname, argv);
		
	} else {
		// fork error
		perror("fork");
        exit(1);
    }
}

void run_redirection_debugger(pid_t child_pid, int fd, long start_addr, bool copy)
{
    int wait_status;

    /* Wait for child to stop on its first instruction */
    wait(&wait_status);
    struct user_regs_struct regs;
    unsigned long data;
    unsigned long data_trap;

    if(!WIFEXITED(wait_status)){
        data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)start_addr, NULL);

        /* Write the trap instruction 'int 3' into the address */
        data_trap = (data & 0xFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data_trap); 
        
        /* Run until foo is called */
        ptrace(PTRACE_CONT, child_pid, 0, 0);  
        wait(&wait_status);   
    }

    while(!WIFEXITED(wait_status)){ 

        /* See where child is now */
        ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
        unsigned long data2 = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)regs.rsp, NULL);
        unsigned long data2_trap = (data2 & 0xFFFFFF00) | 0xCC;
        unsigned long ret_addr = *(unsigned long *)(regs.rsp);
        
        /* Set breakpoint on the return address of foo */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2_trap); 
        
        /* Remove the breakpoint in foo by restoring the previous data */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

        /* Execute one line of code */
        ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        
        /* Write the trap instruction 'int 3' into the address again */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data_trap);
        
        while(1){
            /* The child can continue running now, stop only at syscall or at breakpoint*/
            ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
            wait(&wait_status);
            
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
            if(regs.rip == ret_addr + 1) break;
            else if(regs.rax == 0x01){
                if(!copy){
                    regs.rdx = 0;
                    ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
                }
                write(fd, "PRF:: ", 6);
                write(fd,(char *) regs.rsi, regs.rdx);
            }
            
            /* Run system call and stop */
            ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
            wait(&wait_status);
        }

        /* Remove the breakpoint in foo return address by restoring the previous data(2) */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

    }
    

}
int main(int argc, char** argv)
{
    long addr = strtol(argv[1], NULL, 0);
    bool copy = argv[2] == "c";
    char* fname = argv[3];
    char* args[argc-3];
    
    for(int i = 2; i < argc; i++) 
        args[i-2] = argv[i+1];
    
    int fd = open(fname, O_CREAT);

    pid_t c_pid = run_target_with_args(args[0], args);
    // run specific "debugger"
    run_redirection_debugger(c_pid, fd, addr, copy);

    close(fd);
    return 0;
}
