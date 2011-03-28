
DSSIDIR		= /Users/dave/.dssi
BINDIR		= /usr/local/bin

# To compile with the VeSTige compatibility header:
#CXXFLAGS	= -Ivestige -Wall -g

# To compile with the official VST SDK v2.4r2:
CXXFLAGS	= -m32 -I../../Downloads/vstsdk2.4/pluginterfaces/vst2.x -Wall

LDFLAGS		= 

TARGETS		= dssi-vst-server.exe.so 

HEADERS		= remoteplugin.h \
		  remotepluginclient.h \
		  remotepluginserver.h \
		  rdwrops.h \
		  paths.h

OBJECTS		= remotepluginclient.o \
		  remotepluginserver.o \
		  rdwrops.o \
		  paths.o

all:		$(TARGETS)

install:	all
		mkdir -p $(DSSIDIR)/dssi-vst
#		install dssi-vst.so $(DSSIDIR)
		install dssi-vst-server.exe.so dssi-vst-server $(DSSIDIR)/dssi-vst

clean:
		rm -f $(OBJECTS) libremoteplugin.a

distclean:	clean
		rm -f $(TARGETS) dssi-vst-scanner dssi-vst-server *~ *.bak

%.exe.so:	%.cpp libremoteplugin.a $(HEADERS)
		wineg++ -m32 $(CXXFLAGS) $< -o $* $(LDFLAGS) -L. -lremoteplugin -lpthread

libremoteplugin.a:	remotepluginclient.o remotepluginserver.o rdwrops.o paths.o
		ar r $@ $^

remotepluginclient.o:	remotepluginclient.cpp	$(HEADERS)
		g++ $(CXXFLAGS) remotepluginclient.cpp -c

remotepluginserver.o:	remotepluginserver.cpp $(HEADERS)
		g++ $(CXXFLAGS) remotepluginserver.cpp -c

