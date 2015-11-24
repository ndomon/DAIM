#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

int main (void)
{

printf (" I am main\n");

int child_p = fork ();

if (child_p == -1) exit (1);
else if (child_p == 0) {
    printf (" I am child\n");
    sleep (5);
    printf (" child die\n");
    
}
else {
    printf (" I am main after child forked\n");
    printf (" waiting for child\n");
    waitpid (child_p, NULL, WUNTRACED | WCONTINUED);
    printf (" child exited\n");
    
}
printf (" Hello\n");
_exit (0);
return 0;
}
