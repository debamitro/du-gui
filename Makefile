CFLAGS := `$(WX_HOME)/wx-config --cflags` -std=c++14 -g
LIBS := `$(WX_HOME)/wx-config --libs core,base,adv`

all: duapp

clean:
	$(RM) duapp *.o

duapp: main.o DuApp.o DuFrame.o
	$(CXX) $(LIBS) -o $@ main.o DuApp.o DuFrame.o

%.o: %.cc
	$(CXX) $(CFLAGS) -c -o $@ $<
