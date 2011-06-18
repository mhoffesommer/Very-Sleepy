// simple test program; shows how to use the new Very Sleepy profiling API
// to control the profiler programmatically
#include "../api/sleepy_api.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	SleepyApi &api=SleepyApi::Instance();

	api.Pause(false);
	printf("%d\n",api.IsPaused());

	char help[10];
	gets_s(help);

	api.Pause(true);
	printf("%d\n",api.IsPaused());

	return 0;
}
