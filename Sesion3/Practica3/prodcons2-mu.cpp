// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_items             = 20,
   tam_vector            = 10,
   num_prod              = 4,
   num_cons              = 5,
   num_procesos_esperado = num_cons+num_prod+1,
   id_buffer             = num_prod,
   etiq_prod             = 1,
   etiq_cons             = 2;

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
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir( int numProd )
{
   static int 
      contador = numProd*(num_items/num_prod);

   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor " << numProd << " ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor( int numProd )
{
   for ( unsigned int i=0 ; i < num_items/num_prod ; i++ )
   {
      // producir valor
      int valor_prod = producir( numProd );
      // enviar valor
      cout << "Productor "<< numProd <<" va a enviar valor " << valor_prod << endl << flush;
      // Enviamos el mensaje con el valor al buffer con la etiqueta del productor
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor( int numCons)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/num_cons ; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
      cout << "Consumidor " << numCons+num_prod+1 << " ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              productorOConsumidor;    // variable para determinar que tipo de 
                                       // proceso se puede usar
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      productorOConsumidor = 0;
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
         productorOConsumidor = 1;                  // $~~~$ solo prod.
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
         productorOConsumidor = 2 ;                 // $~~~$ solo cons.

      // 2. recibir un mensaje del emisor o emisores aceptables
      if(productorOConsumidor == 1)
         MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_prod, MPI_COMM_WORLD, &estado );
      else if(productorOConsumidor == 2)
         MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_cons, MPI_COMM_WORLD, &estado );
      else
         MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido
      if( estado.MPI_SOURCE>=0 && estado.MPI_SOURCE<num_prod ){
         // si ha sido el productor: insertar en buffer
         buffer[primera_libre] = valor ;
         primera_libre = (primera_libre+1) % tam_vector ;
         num_celdas_ocupadas++ ;
         cout << "Buffer ha recibido valor " << valor << endl ;
      }else if( estado.MPI_SOURCE>=num_prod+1 && estado.MPI_SOURCE<=num_cons+num_prod){
         // si ha sido el consumidor: extraer y enviarle
         valor = buffer[primera_ocupada] ;
         primera_ocupada = (primera_ocupada+1) % tam_vector ;
         num_celdas_ocupadas-- ;
         cout << "Buffer va a enviar valor " << valor << endl ;
         MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int 
      id_propio, 
      num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio >= 0 && id_propio < num_prod ){
         funcion_productor(id_propio);
      }
      else if ( id_propio == num_prod ){
         funcion_buffer();
      }
      else{
         funcion_consumidor(id_propio-num_prod-1);
      }
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
