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
DARWIN_INCLUDE = -Ichai3d/darwin/GL -Ichai3d/darwin/GLUT -Iliblo-0.23
LOCALOBJS = $(notdir $(OBJECTS) )
DEFS = -D_POSIX -D_MAX_PATH=260 -D_LINUX -DLINUX -D__LINUX__ -D_POSIX # -DUSE_FREEGLUT
CC   = g++ -c $(DEFS)
CFLAGS = -O3 $(INCLUDE) $(DEBUG)
LD   = g++ -o 

DARWIN_LIBS = -framework OpenGL -framework GLUT
LINUX_LIBS = -lGL -lglut -lGLU
LINUX_CHAI_LIBS = -Lchai3d/lib/linux -lchai3d_linux -ldhd -lpciscan -lusb
DARWIN_CHAI_LIBS = -Lchai3d/lib/darwin -lchai3d_darwin
LIBS = -Lliblo-0.23/src/.libs -lpthread -llo -Lode-0.7/ode/src -lode

ifeq ($(shell uname),Darwin)
INCLUDE += $(DARWIN_INCLUDE)
LIBS +=  $(DARWIN_LIBS) $(DARWIN_CHAI_LIBS)
endif

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
