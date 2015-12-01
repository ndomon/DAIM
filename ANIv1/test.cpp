#include <iostream>
#include <time.h>
#include <unistd.h>

using namespace std;

int main (void)
{

clock_t start, end;
double cpu_time_used;

start = clock ();

sleep (10);

end = clock ();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

cout << cpu_time_used << endl;

return 0;

}
