// audiow.hpp

#ifndef AUDIOW_HPP
#define AUDIOW_HPP

void	DACPauseWaveDev(void);
void	DACResumeWaveDev(void);
void	DACPollWaveBuffs();
uint	ElapsedBytes();

Boolean	DACStop();
int	DACLoc();
Boolean	DACSetRate(int);
Boolean	DACSetBits(int);
Boolean	DACSetChannels(int);
Boolean	DACSetVolume(int);
Boolean	DACTerminate();
Boolean	DACInit();
int	DACGetVolume();
int	DACGetMaxChannels();
int	DACGetMaxBits();
int	DACGetMaxRate();
int	DACSubmitBuffer(char*,int);
#endif

