#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int tam_vec = 15;
const int num_items = 75;
int vector[tam_vec];
Semaphore estanquero = 1;
Semaphore fumador = 0;
int ingredienteFumador = -1;
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

int producir(){
  return aleatorio<0,2>();
}

void produccion(){
  mtx.lock();
    cout << "El estanquero produce el ingrendiente: "<<ingredienteFumador<<endl;
  mtx.unlock();
}

void retirar(){
  mtx.lock();
    cout << "El fumador "<<ingredienteFumador<<" retira el ingrediente"<<endl;
  mtx.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(){
  while (true){
    sem_wait(estanquero);
      ingredienteFumador = producir();
      produccion();
    sem_signal(fumador);
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
  mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
  mtx.unlock();

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
  mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
  mtx.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador ){
   while( true ){
      if(num_fumador == ingredienteFumador){
        sem_wait(fumador);
          retirar();
          ingredienteFumador=-1;
        sem_signal(estanquero);
          fumar(num_fumador);
      }
   }
}

//----------------------------------------------------------------------

int main()
{
   thread  hFumadores[3], hEstanquero;

   hEstanquero = thread(funcion_hebra_estanquero);
   for( int i=0 ; i<3 ; i++)
     hFumadores[i]=thread(funcion_hebra_fumador, i );
   
   hEstanquero.join();
   for( int i=0 ; i<3 ; i++)
     hFumadores[i].join();
}
