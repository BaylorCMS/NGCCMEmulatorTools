ngCCMEmuTool.exe : ngCCMEmuTool.cc
	g++ --std=c++11 -L SUB-20-snap-130926/lib/ ngCCMEmuTool.cc -lsub -lusb-1.0 -o ngCCMEmuTool.exe

