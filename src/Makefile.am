
bin_PROGRAMS = dimple

dimple_SOURCES = AudioStreamer.cpp dimple.cpp	\
   HapticsSim.cpp InterfaceSim.cpp OscBase.cpp OscObject.cpp			\
   OscValue.cpp PhysicsSim.cpp Simulation.cpp ValueTimer.cpp			\
   VisualSim.cpp
dimple_LDADD =

if WINDRES

dimple_LDADD += dimple.res

dimple.res: ../icon/dimple.rc
	$(WINDRES) $^ -O coff $@

endif
