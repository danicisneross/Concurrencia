#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include <stdbool.h>
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
    int* iterator;      //variable que realizara el seguimiento del recuento de iteraciones actual
    struct array* arr;  //array del thread
    mtx_t* mutex;       //array de mutex
};

bool actualizar_contador (int* i, int max_iterations, mtx_t* mutex){        //int* i: contador de iteraciones actual 
    
    mtx_lock(mutex);            //bloqueo del mutex para evitar que otros hilos accedan al contador de iteraciones al mismo tiempo        
    if (*i < max_iterations){   //si el contador actual es menor que el max de iteraciones
        *i = *i + 1;            //incrementamos el contador
        mtx_unlock(mutex);      //liberamos el mutex
        return true;            //devolvemos true porque el contador se actualizo   
    } else {                    //si es igual o mayor 
        mtx_unlock(mutex);      //liberamos el mutex 
        return false;           //no actualizamos el comtador por lo que devolvemos false 
    }
}

void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++);       //consume ciclos de cpu
}

int increment(void* p){     //es un tipo de puntero generico que puede apuntar a cualquier tipo de objeto
    struct thread_data* d = (struct thread_data *) p;       //conversion del puntero p al struct thread_data
    int pos, val;

    while(actualizar_contador(d->iterator, *d->iterations, d->mutex)) {     //mientras que el contador actual sea menor que el max de iteraciones actualizar_contador sera verdadera 
        pos = rand() % d->arr->size;        //generamos un numero aleatorio con rand y lo pasamos en base del tamaño del array

        mtx_lock(&d->mutex[pos]);           //utilizamos un mutex para cada posicion y asi nos aseguramos de que solo un hilo puede acceder a dicha posicion    

        printf("%d increasing position %d\n", d->id, pos);

        val = d->arr->arr[pos];     //obtenemos el valor actual de la posicion aleatoria del array
        apply_delay(*d->delay);

        val ++;                     //incrementamos en uno
        apply_delay(*d->delay);

        d->arr->arr[pos] = val;     //almacenamos el nuevo valor en dicha posicion

        mtx_unlock(&d->mutex[pos]);     //liberamos el acceso al mutex de esa posicion
        apply_delay(*d->delay);
    }    
    return 0;
}

int swap(void *p){
    struct thread_data* d = (struct thread_data *) p;
    int pos1, pos2, val1, val2;

    while (actualizar_contador(d->iterator, *d->iterations, d->mutex)){     //mientras que el contador actual sea menor que el max de iteraciones actualizar_contador sera verdadera 
        pos1 = rand() % d->arr->size;       //generamos un numero aleatorio con rand y lo pasamos en base del tamaño del array 
        
        //Hacemos este bucle para controlar que las posiciones nunca sean iguales (para evitar un interbloqueo)
        do {
            pos2 = rand() % d->arr->size;   //generamos un numero aleatorio con rand y lo pasamos en base del tamaño del array
        } while (pos2 == pos1);
        
        if(pos1 < pos2){        //controlamos que no haya interbloqueos entre los mutex bloqueándolos en orden
            
            //Protegemos los mutex de cada posicion
            mtx_lock(&d->mutex[pos1]);
            mtx_lock(&d->mutex[pos2]);

        } else {
            mtx_lock(&d->mutex[pos2]);
            mtx_lock(&d->mutex[pos1]);
        }

        printf("%d swaping of position %d for %d\n", d->id, pos1, pos2);

        val1 = d->arr->arr[pos1];       //obtenemos el valor actual de pos1 
        val2 = d->arr->arr[pos2];       //obtenemos el valor actual de pos2
        apply_delay(*d->delay);

        val1--;                         //decrementamos en uno
        val2++;                         //incrementamos en uno
        apply_delay(*d->delay);

        d->arr->arr[pos1] = val1;       //almacenamos el nuevo valor en la pos1 del array
        d->arr->arr[pos2] = val2;       //almacenamos el nuevo valor en la pos2 del array 

        //Liberamos el acceso a los mutex de dichas posiciones
        mtx_unlock(&d->mutex[pos1]);    
        mtx_unlock(&d->mutex[pos2]);
        apply_delay(*d->delay);
    }
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
    int iterator_inc = 0;                   //entero que llevara la cuenta de las iteraciones actuales de incremento
    int iterator_swap = 0;                  //entero que llevara la cuenta de las iteraciones actuales de intercambio
    struct options opt;                     //struct para las diferentes opciones     
    struct array   arr;                     //struct del array
    struct thread_data* thread_data;        //argumentos de los hilos usados para el incremento
    struct thread_data* thread_data_swap;   //argumentos de los hilos usados para el intercambio
    thrd_t* thr_swap;                       //array de hilos del intercambio
    thrd_t* thr;                            //array de hilos del incremento
    mtx_t* mutex;                           //array de mutex


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
    thread_data_swap = malloc(opt.num_threads * sizeof(struct thread_data));
    thr = malloc(opt.num_threads * sizeof(thrd_t));
    thr_swap = malloc(opt.num_threads * sizeof(thrd_t));
    mutex = malloc(arr.size * sizeof(mtx_t));

    memset(arr.arr, 0, arr.size * sizeof(int));     //inicializamos el array con 0s
    
    for(i = 0; i < arr.size; i++){      //utilizamos el for para poder inicializar el array de mutex por posiciones
        mtx_init(&mutex[i], 0);
    }

    //Argumentos
    for (i=0; i<opt.num_threads; i++){
        thread_data[i].id = i;
        thread_data[i].iterations = &opt.iterations;            //para que todos los hilos tengan el mismo numero de iteraciones
        thread_data[i].delay = &opt.delay;                      //para que todos los hilos tengan el mismo tiempo de retardo
        thread_data[i].arr = &arr;                              //para que todos los hilos accedan al mismo array
        thread_data[i].mutex = mutex;                           //cada hilo tiene un puntero al mismo array de mutex
        thread_data[i].iterator = &iterator_inc;
        thrd_create(&thr[i], increment, &thread_data[i]);       //creamos los threads. increment() es una funcion de tipo int para poder pasarselo como argumento a los hilos
        
        thread_data_swap[i].id = i + opt.num_threads;           //asi asignamos un identificador diferente al anterior thread
        thread_data_swap[i].iterations = &opt.iterations;
        thread_data_swap[i].delay = &opt.delay;
        thread_data_swap[i].arr = &arr;
        thread_data_swap[i].mutex = mutex;
        thread_data_swap[i].iterator = &iterator_swap;
        thrd_create(&thr_swap[i], swap, &thread_data_swap[i]);  //creamos los threads de intercambio
    }
    
    //Esperamos que todos los hilos terminen su ejecucion
    for (i=0; i<opt.num_threads; i++){
        thrd_join(thr[i], NULL);
        thrd_join(thr_swap[i], NULL);
    }

    print_array(arr);       //imprimimos el array
    
    mtx_destroy(mutex);
    free(mutex);
    free(arr.arr);
    free(thread_data);
    free(thread_data_swap);
    free(thr);
    free(thr_swap);
    return 0;
}
