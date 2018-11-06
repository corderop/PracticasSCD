# Makefile
# 	compilar: compila con la opción especial para hebras y con la version de c++11
#	ejecutar: ejecuta el ejecutable

EJECUTABLE=IntegralesPorHebras

ejecutar: compilar
	./$(EJECUTABLE)

compilar: IntegralesPorHebras.cpp
	g++ -std=c++11 -pthread -o IntegralesPorHebras IntegralesPorHebras.cpp

clean:
	rm -R $(EJECUTABLE)
