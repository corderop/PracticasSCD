// Pablo Cordero Romero

#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int num_fumadores = 3;
mutex mtx;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}

void fumar(int num_fumador)
{
  chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": empieza a fumar ("
       << duracion_fumar.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_fumar);

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente "
       << num_fumador << "." << endl;
  mtx.unlock();
}

int producir_ingrediente()
{
  this_thread::sleep_for(chrono::milliseconds( aleatorio<20,200>() ));
  return aleatorio<0,num_fumadores-1>();
}

class FumadoresSU : public HoareMonitor {
  private:
    int ingrediente;
    CondVar mostradorVacio;
    CondVar ingredientes[num_fumadores];

  public:
    FumadoresSU();
    void get_ingrediente(int i);
    void poner_ingrediente(int i);
    void esperar_ingrediente();
};

FumadoresSU::FumadoresSU()
{
  ingrediente = -1;
  mostradorVacio = newCondVar();
  for (int i=0; i<num_fumadores; i++){
    ingredientes[i] = newCondVar();
  }
}

void FumadoresSU::get_ingrediente(int i)
{
  if (ingrediente != i){
    ingredientes[i].wait();
  }
  ingrediente = -1;

  mtx.lock();
  cout << "\t\tRetirado el ingrediente: " << i << endl;
  mtx.unlock();

  mostradorVacio.signal();
}

void FumadoresSU::poner_ingrediente(int i)
{
  ingrediente = i;

  mtx.lock();
  cout << "Puesto el ingrediente: " << ingrediente << endl;
  mtx.unlock();

  ingredientes[i].signal();
}

void FumadoresSU::esperar_ingrediente()
{
  if (ingrediente != -1){
    mostradorVacio.wait();
  }
}

void funcion_hebra_fumador(MRef<FumadoresSU> monitor, int i)
{
  while(true)
  {
    monitor->get_ingrediente(i);
    fumar(i);
  }
}

void funcion_hebra_estanquero(MRef<FumadoresSU> monitor)
{
   while(true)
   {
     int ingred = producir_ingrediente();
     monitor->poner_ingrediente(ingred);
     monitor->esperar_ingrediente();
   }
}

int main()
{
  cout << endl << "--------------------------------------------------------" << endl
               << "Problema de los fumadores" << endl
               << "--------------------------------------------------------" << endl
               << flush ;

  MRef<FumadoresSU> monitor = Create<FumadoresSU>();

  thread hebra_estanquero(funcion_hebra_estanquero, monitor);
  thread hebra_fumador[num_fumadores];

  for(int i=0; i<num_fumadores; i++){
    hebra_fumador[i] = thread(funcion_hebra_fumador, monitor, i);
  }

  for(int i=0; i<num_fumadores; i++){
    hebra_fumador[i].join();
  }

  hebra_estanquero.join();
}
