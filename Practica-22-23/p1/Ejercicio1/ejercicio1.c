#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include "options.h"

#define DELAY_SCALE 1000


struct array {
    int size;   //tamaño del array 
    int *arr;   //puntero a un entero que apunta a la primera pos
};

struct thread_data {
    int id;             //identificador
    int* iterations;    //numero de iteraciones
    int* delay;         //tiempo de retardo 
    struct array* arr;  //array del thread
    mtx_t* mutex;       //mutex del thread
};


void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++);       //consume ciclos de cpu
}


int increment(void* p){     //p es un tipo de puntero generico que puede apuntar a cualquier tipo de objeto
    struct thread_data* d = (struct thread_data *) p;       //conversion del puntero p al struct thread_data
    int pos, val; 
    
    mtx_lock(d->mutex);     //protegemos el acceso al mutex, esto garantiza la consistencia del estado del array en todo momento         
    for(int i = 0; i < *d->iterations; i++) {
        pos = rand() % d->arr->size;        //generamos un numero aleatorio con rand y lo pasamos en base del tamaño del array 
        
        printf("%d increasing position %d\n", d->id, pos);      

        val = d->arr->arr[pos];     //obtenemos el valor actual de la posicion aleatoria del array
        apply_delay(*d->delay);

        val ++;                     //incrementamos en uno
        apply_delay(*d->delay);

        d->arr->arr[pos] = val;     //almacenamos el nuevo valor en dicha posicion
        apply_delay(*d->delay);
    }
    mtx_unlock(d->mutex);       //desbloqueamos el acceso al mutex
    return 0;
}


void print_array(struct array arr) {        
    int total = 0;

    for(int i = 0; i < arr.size; i++) {
        total += arr.arr[i];            //suma de los elementos del array
        printf("%d ", arr.arr[i]);      //imprimimos cada elemento del array 
    }

    printf("\nTotal: %d\n", total);
}


int main (int argc, char **argv){
    int i;
    struct options opt;                 //struct para las diferentes opciones     
    struct array   arr;                 //struct del array
    struct thread_data* thread_data;    //struct de argumentos del hilo
    thrd_t* thr;                        //puntero a hilo
    mtx_t mutex;                        //mutex

    mtx_init(&mutex, 0);        //inicializacion del mutex

    srand(time(NULL));          //generamos numeros aleatorios utilizando el valor actual del tiempo

    //Default values for the options
    opt.num_threads  = 5;
    opt.size         = 10;
    opt.iterations   = 100;
    opt.delay        = 1000;

    read_options(argc, argv, &opt);     //leemos los argumentos del programa, actualizando las opciones (si asi lo requiere)
    
    arr.size = opt.size;        
    arr.arr  = malloc(arr.size * sizeof(int));
    thread_data = malloc(opt.num_threads * sizeof(struct thread_data));
    thr = malloc(opt.num_threads * sizeof(thrd_t));


    memset(arr.arr, 0, arr.size * sizeof(int));     //inicializamos el array con 0s
    
    //Argumentos de cada threads
    for (i = 0; i < opt.num_threads; i++){
        thread_data[i].id = i;
        thread_data[i].iterations = &opt.iterations;        //para que todos los hilos tengan el mismo numero de iteraciones
        thread_data[i].delay = &opt.delay;                  //para que todos los hilos tengan el mismo tiempo de retardo
        thread_data[i].arr = &arr;                          //para que todos los hilos accedan al mismo array
        thread_data[i].mutex = &mutex;                      //para que todos los hilos accedan al mutex compartido
        thrd_create(&thr[i], increment, &thread_data[i]);   //creamos los threads. Increment() es una funcion de tipo int para poder pasarselo como argumento a los hilos
    }

    for (i=0; i<opt.num_threads; i++){
        thrd_join(thr[i], NULL);        //esperamos que todos los hilos terminen su ejecucion
    }


    print_array(arr);       //imprimimos el array
    
    mtx_destroy(&mutex);    
    free(arr.arr);
    free(thread_data);
    free(thr);
    return 0;
}
