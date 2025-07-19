#include <ti/getcsc.h>
#include <ti/screen.h>
#include <ti/real.h>
#include <sys/timers.h>
#include <time.h>

int main(void)
{
	os_SetCursorPos(0, 0);
	os_ClrHome();
	os_PutStrFull("First of all, your handwriting is ugly. Secondly, I had the same problem, but I waited a while, and the problem disappeared. -Kevin Liu");
	os_SetCursorPos(8, 0);
	os_PutStrFull("Press any key to continue...");
	while (!os_GetCSC());
	return 0;
}