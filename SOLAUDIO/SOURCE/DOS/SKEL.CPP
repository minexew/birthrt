#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "audio.hpp"
#include "kernaud.hpp"
#include "custmsys.hpp"

#define	SAMPLE_NUM		100
#define	SAMPLE_TAG		0
#define	SAMPLE_LOOP		0
#define	SAMPLE_VOLUME	127
#define	SAMPLE_PAUSE	0

int
main()
{
	int duration;

	if (!KernelAudioInitialize())
		exit(printf("Init failed\n"));
	if (!kernel->AudioPlay(SAMPLE_NUM, (Boolean)SAMPLE_LOOP, SAMPLE_VOLUME,
			SAMPLE_TAG, (long)SAMPLE_PAUSE))
		exit(printf("Can't play #%d\n",SAMPLE_NUM));
	do {
		if (kbhit()) {
			getch();
			break;
		}
		kernel->AudioDurationAll(&duration);
	} while (duration>= 0);

	kernel->AudioPauseAll();

do {
		if (kbhit()) {
			getch();
			break;
		}
		kernel->AudioDurationAll(&duration);
	} while (duration>= 0)			  ;
						  
	kernel->AudioResumeAll();

do {
		if (kbhit()) {
			getch();
			break;
		}
		kernel->AudioDurationAll(&duration);
	} while (duration>= 0);

	KernelAudioTerminate();   
	return 0;
}

