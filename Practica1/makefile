.SUFFIXES:
.PHONY: pc
.PRECIOUS: %_exe

compilador:=g++

fu: fumadores_exe
	./$<

%_exe: %.cpp
		$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp 

clean:
	rm -f *_exe
