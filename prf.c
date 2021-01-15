
#include <stdbool.h>

int main(int argc, char** argv)
{
	long addr = argv[1];
	bool copy = argv[2] == 'c';
	char* fname = argv[3];
	char* args[argc-3];
	
	for(int i = 2; i < argc; i++) 
		args[i-2] = argv[i+1];
    
    FILE * fp = open(fname, O_CREATE);

    pid_t c_pid = run_target_with_args(args[0], args);
	// run specific "debugger"
	run_redirection_debugger(child_pid);

    return 0;
}