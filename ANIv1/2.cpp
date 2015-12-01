#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main (void)
{

int f;
char lldp[1024];

memset(lldp, '\0', sizeof (lldp));

f =  open ("lldpout", O_RDONLY);
int l = -1;

if (f != -1) {
l = read (f, lldp, sizeof (lldp));
printf ("%s\n", lldp);
close (f);
}
else printf ("Cant\n");

return 0;
}
