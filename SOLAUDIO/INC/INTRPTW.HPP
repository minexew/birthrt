// intrptw.hpp

#ifndef INTRPTW_HPP
#define INTRPTW_HPP

typedef void (ServerFunc)();

void	InstallServer(ServerFunc* proc, int freqMSec);
void	DisposeServer(ServerFunc*);

#endif
