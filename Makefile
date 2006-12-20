# Things you might need to change in this Makefile:
#
# If you don't have the SensAble libraries installed, take out -lHD and -lHDU from the LIBS line

OBJECTS = osc_chai_glut.o WOscServer.o

DEBUG = -g -ggdb
SOURCES = $(OBJECTS:.o=.cpp)
INCLUDE = -Ichai3d/include -I.
LOCALOBJS = $(notdir $(OBJECTS) )
DEFS = -D_POSIX -D_MAX_PATH=260 -D_LINUX -DLINUX -D__LINUX__ -DUSE_FREEGLUT
CC   = g++ -c $(DEFS)
CFLAGS = -O3 $(INCLUDE) $(DEBUG)
LD   = g++ -o 
LIBS = -Lchai3d/lib/linux -lchai3d_linux -lGL -lglut -lGLU -ldhd -lpciscan -lpthread -lusb -LWOscLib/build/linux/ -lwosclib

all: osc_chai_glut

%.o: %.cpp 
	$(CC) $(CFLAGS) $<
  
# Actual target and dependencies
osc_chai_glut: $(OBJECTS)
	$(LD) osc_chai_glut $(LOCALOBJS) $(LIBS)
	@echo "compilation done"

# Target deleting unwanted files
clean:
	\rm -f *.o *~ osc_chai_glut core 
