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
    int wait_status;

    /* Wait for child to stop on its first instruction */
    wait(&wait_status);
    printf("Starting debugger.\n");
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
        printf("First break hit\n");
        /* See where child is now */
        ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
        /* Get the function return addres */
        unsigned long ret_addr = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)(regs.rsp), NULL);//ptrace(PTRACE_PEEKDATA, child_pid, (void*)(regs.rbp), NULL);
        /* Put a breakpoint at the return address of the function */
        unsigned long data2 = ptrace(PTRACE_PEEKTEXT, child_pid, ret_addr, NULL);
        unsigned long data2_trap = (data2 & 0xFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2_trap); 
        
        /* Remove the breakpoint in foo by restoring the previous data */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

        /* Execute one line of code */
        ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        /* Put back the breakpoint */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)start_addr, (void*)data_trap); 

        while(1){
            /* The child can continue running now, stop only at syscall or ret */
            ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
            wait(&wait_status);

            if(WIFEXITED(wait_status)) return;
            else if(regs.rip == ret_addr || regs.rip - 1== ret_addr) break;       
            //printf("syscall happend\n");    
            printf("%llx - %d\n" , regs.rip+1, (regs.rip-1 == ret_addr)); 
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
            if(regs.rax == 1){
                printf("syscall is write from %lld, %lld chars\n",regs.rsi, regs.rdx);     
                write(fd, "PRF:: ", 6);
                write(fd,(void *)regs.rsi, regs.rdx);
                if(copy == 0){
                    printf("Moving\n");
                    regs.rdx = 0;
                    ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
                }
            }
            
            /* Run system call and stop */
            ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
            wait(&wait_status);
        }

        /* Remove the breakpoint in foo return address by restoring the previous data(2) */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)ret_addr, (void*)data2);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);
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
    int fd = open(fname, O_CREAT | O_RDWR);

    pid_t c_pid = run_target_with_args(args[0], args);
    // run specific "debugger"
    run_redirection_debugger(c_pid, fd, addr, copy);

    close(fd);
    return 0;
}
