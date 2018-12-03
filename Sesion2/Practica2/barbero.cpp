// Pablo Cordero Romero

#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

const int num_clientes = 5;
mutex mtx;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}

void cortado_pelo()
{
  chrono::milliseconds duracion_corte( aleatorio<50,500>() );

  mtx.lock();
  cout << "\t\tBarbero corta el pelo al cliente: ("
       << duracion_corte.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_corte);

  mtx.lock();
  cout << "\t\tBarbero termina de pelar al cliente " << endl;
  mtx.unlock();
}

void esperar_fuera(int cliente)
{
  chrono::milliseconds duracion_espera( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tCliente " << cliente << " espera fuera de la barbería durante ("
       << duracion_espera.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_espera);
}

class BarberoSU : public HoareMonitor {
  private:
    int cliente;
    CondVar silla_vacia;
    CondVar esperando;
    CondVar cortando;

  public:
    BarberoSU();
    void cortar_pelo(int cliente);
    void siguiente_cliente();
    void fin_cliente();
};

BarberoSU::BarberoSU( )
{
  silla_vacia = newCondVar();
  esperando = newCondVar();
  cortando = newCondVar();
}

void BarberoSU::cortar_pelo(int i)
{
  mtx.lock();
  cout << "\t\tCliente " << i << " entra en la barbería" << endl;
  mtx.unlock();

  if(silla_vacia.get_nwt() != 0){
    silla_vacia.signal();
  }
  else{
    esperando.wait();
  }

  mtx.lock();
  cout << "\t\tCliente " << i << " se está pelando" << endl;
  mtx.unlock();

  cortando.wait();
}

void BarberoSU::siguiente_cliente()
{
  if(esperando.get_nwt() == 0){
    silla_vacia.wait();
  }
  else{
    esperando.signal();
  }
}

void BarberoSU::fin_cliente()
{
  cortando.signal();
}

void funcion_hebra_cliente(MRef<BarberoSU> monitor, int i)
{
  while(true)
  {
    monitor->cortar_pelo(i);
    esperar_fuera(i);
  }
}

void funcion_hebra_barbero(MRef<BarberoSU> monitor)
{
   while(true)
   {
     monitor->siguiente_cliente();
     cortado_pelo();
     monitor->fin_cliente();
   }
}

int main()
{
  cout << endl << "--------------------------------------------------------" << endl
               << "Problema de la barbería" << endl
               << "--------------------------------------------------------" << endl
               << flush ;

  MRef<BarberoSU> monitor = Create<BarberoSU>();

  thread hebra_barbero(funcion_hebra_barbero, monitor);
  thread hebra_cliente[num_clientes];

  for(int i=0; i<num_clientes; i++){
    hebra_cliente[i] = thread(funcion_hebra_cliente, monitor, i);
  }

  for(int i=0; i<num_clientes; i++){
    hebra_cliente[i].join();
  }

  hebra_barbero.join();
}
