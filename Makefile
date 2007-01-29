# Things you might need to change in this Makefile:
#
# If you don't have the SensAble libraries installed, take out -lHD and -lHDU from the LIBS line

OBJECTS = osc_chai_glut.o \
	CODEPrimitive.o \
	CODEMesh.o \
	CODEPrism.o \
	CODESphere.o \
	CODEPotentialProxy.o \
	OscObject.o

DEBUG = -g -ggdb
SOURCES = $(OBJECTS:.o=.cpp)
INCLUDE = -Ichai3d/include -Iode-0.7/include -I.
LOCALOBJS = $(notdir $(OBJECTS) )
DEFS = -D_POSIX -D_MAX_PATH=260 -D_LINUX -DLINUX -D__LINUX__ -D_POSIX -DUSE_FREEGLUT
CC   = g++ -c $(DEFS)
CFLAGS = -O3 $(INCLUDE) $(DEBUG)
LD   = g++ -o 
LIBS = -Lchai3d/lib/linux -lchai3d_linux -lGL -lglut -lGLU -ldhd -lpciscan -lpthread -lusb -llo -Lode-0.7/ode/src -lode

all: osc_chai_glut

%.o: %.cpp  $(wildcard *.h)
	$(CC) $(CFLAGS) $<

# Actual target and dependencies
osc_chai_glut: $(OBJECTS)
	$(LD) osc_chai_glut $(LOCALOBJS) $(LIBS)
	@echo "compilation done"

# Target deleting unwanted files
clean:
	\rm -f *.o *~ osc_chai_glut core 
