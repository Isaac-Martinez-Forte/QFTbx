//include C++
#include <iostream>
#include <vector>
#include <complex>
#include <string>
#include <cmath>


//include CUDA
#include <cuda_runtime.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/extrema.h>
#include <thrust/unique.h>
#include <thrust/execution_policy.h>


#define inf 0x7f800000
#define pi 3.1416

typedef float2 Complex;

using namespace std;
using namespace thrust;


int threadsPerBlock = 128;
size_t size_double = 0;
int blocksPerGrid = 0;

//Función principal para resolver e_hull con cuda.
extern "C"
vector<complex<double> > e_hull_cuda(vector <complex <double> > puntos, float epsilon);

//Función que encuentra el punto segundo de la nube de puntos.
static __global__ void buscar_segundo_kernel(Complex primero, Complex * puntos, float epsilon,
                                             double * retorno, int n_puntos);
//Función que encuentra el punto siguiente de la nube de puntos.
static __global__ void buscar_siguiente_kernel(Complex primero, Complex segundo, Complex * puntos, float epsilon,
                                               double * retorno, int n_puntos);


//Funciones internas
//int buscar_primero(host_vector<double> vector_buscar_primero_h);
int buscar_segundo (Complex * puntos_d, Complex complejo_primero, float epsilon, int numElements);
int buscar_siguiente (Complex complejo_primero, Complex complejo_segundo, Complex * puntos_d, float epsilon, int numElements,
                      double * retorno_siguiente_d);

//Funciones internas GPU
static __device__ __host__ inline double abs(Complex a);
static __device__ __host__ inline double arg(Complex a);
static __device__ __host__ inline Complex operator-(Complex a, Complex b);
static __device__ __host__ inline double arg(Complex a);
static __device__ __host__ inline Complex operator/(Complex a, Complex b);
static __device__ __host__ inline bool operator!=(Complex a, Complex b);


vector <complex <double> > e_hull_cuda(vector<complex<double> > puntos, float epsilon){

    // Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;

    vector <int> indice_contorno;

    // Creamos los tamaños de los distintos vectores
    int numElements = puntos.size();
    size_t size_complex = numElements * sizeof(Complex);
    size_double = numElements * sizeof(double);
    blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;

    ///////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Buscar Primero /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    //Creamos el vector de complejos
    Complex * puntos_h = (Complex *) malloc(size_complex);

    //Creamos el vector del host para buscar el primer número
    host_vector<double> vector_buscar_primero_h (numElements);
    
    double numDe = -inf;
    int posicion_primero = 0;

    for (int i = 0; i < numElements; i++){
            
        puntos_h[i].x = real(puntos.at(i));
	
        puntos_h[i].y = imag(puntos.at(i));
	
	if (puntos_h[i].y > numDe){
            posicion_primero = i;
            numDe = puntos_h[i].y;
        }

        //vector_buscar_primero_h[i] =  puntos.at(i).real();
    }

    //int posicion_primero = buscar_primero(vector_buscar_primero_h);

    indice_contorno.push_back(posicion_primero);

    //Guardamos el primero número complejo del contorno
    Complex complejo_primero;
    complejo_primero.x = puntos_h[posicion_primero].x;
    complejo_primero.y = puntos_h[posicion_primero].y;

    ///////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Buscar Segundo /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    Complex * puntos_d = NULL;
    err = cudaMalloc((void **)&puntos_d, size_complex);

    if (err != cudaSuccess)
    {
        cerr << "Error al reservar memoria en CUDA (error code " <<  cudaGetErrorString(err) << ")!" << endl ;
        exit(EXIT_FAILURE);
    }

    //Copiamos a la memoria de CUDA los datos.
    cudaMemcpy(puntos_d, puntos_h, size_complex, cudaMemcpyHostToDevice);

    int posicion_segundo = buscar_segundo(puntos_d, complejo_primero, epsilon, numElements);

    if (posicion_segundo < 0){
        cudaFree(puntos_d);
        free(puntos_h);

        vector <complex <double> > vector_nulo;
        return vector_nulo;
    }

    indice_contorno.push_back(posicion_segundo);

    //Preparamos el complejo segundo.
    Complex complejo_segundo;
    complejo_segundo.x = puntos_h[posicion_segundo].x;
    complejo_segundo.y = puntos_h[posicion_segundo].y;


    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////// Buscar Siguiente ////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////


    int punto_previo = posicion_primero;
    int punto_actual = posicion_segundo;


    //Creamos las estructuras necesarias para guardar el retorno del kernel
    double * retorno_siguiente_d = NULL;
    cudaMalloc((void **)&retorno_siguiente_d, size_double);

    int punto_siguiente = buscar_siguiente(complejo_primero, complejo_segundo, puntos_d,
                                           epsilon, numElements, retorno_siguiente_d);

    if (punto_siguiente < 0){
        cudaFree(retorno_siguiente_d);
        cudaFree(puntos_d);
        free(puntos_h);

        vector <complex <double> > vector_nulo;
        return vector_nulo;
    }

    int contador = 2;

    int MAXP = numElements;

    while (posicion_primero != punto_actual || posicion_segundo != punto_siguiente){

        indice_contorno.push_back(punto_siguiente);
        contador++;

        if (contador > MAXP){
            break;
        }

        punto_previo = punto_actual;
        punto_actual = punto_siguiente;

        ////////////////////////////////////////////////////////////////////////////
        //Preparamos los números complejos anteriores
        Complex complejo_previo;
        complejo_previo.x = puntos_h[punto_previo].x;
        complejo_previo.y = puntos_h[punto_previo].y;


        Complex complejo_actual;
        complejo_actual.x = puntos_h[punto_actual].x;
        complejo_actual.y = puntos_h[punto_actual].y;

        punto_siguiente = buscar_siguiente(complejo_previo, complejo_actual, puntos_d,
                                           epsilon, numElements, retorno_siguiente_d);


        if (punto_siguiente < 0){
            cudaFree(retorno_siguiente_d);
            cudaFree(puntos_d);
            free(puntos_h);

            vector <complex <double> > vector_nulo;
            return vector_nulo;
        }

        ////////////////////////////////////////////////////////////////////////////

    }

    ///////////////////////////////////////////////////////////////////////////////


    cudaFree(retorno_siguiente_d);
    cudaFree(puntos_d);
    free(puntos_h);

    /*device_vector <int> indice_contorno_d = indice_contorno;

    device_vector<int>::iterator final = thrust::unique(thrust::host, indice_contorno_d.begin(), indice_contorno_d.end());
    vector <complex <double> > puntos_contorno;*/



    vector <int>  aux;
    bool isRepetido = false;

    for (int i = 0; i < indice_contorno.size(); i++) {
      
        isRepetido = false;
      
        for (int j = 0;j < aux.size(); j++) {
            if (indice_contorno.at(i) == indice_contorno.at(j)){
                isRepetido = true;
                break;
            }
        }
        if (!isRepetido){
            aux.push_back(indice_contorno.at(i));
        }
    }

    vector <complex <double> > puntos_contorno;

    for (int i = 0; i < aux.size(); i++){
        puntos_contorno.push_back(puntos.at(aux.at(i)));
    }

    return puntos_contorno;
}

/*int buscar_primero(host_vector<double> vector_buscar_primero_h){
    //copiamos los valores al vector del device para buscar el primer número
    device_vector<double> vector_buscar_primero_d = vector_buscar_primero_h;

    //Buscamos el máximo.
    device_vector<double>::iterator iter = max_element(vector_buscar_primero_d.begin(), vector_buscar_primero_d.end());

    //Recuperamos la posición del complejo primero.
    int posicion_primero = iter - vector_buscar_primero_d.begin();

    return posicion_primero;
}*/

int buscar_segundo (Complex * puntos_d, Complex complejo_primero, float epsilon, int numElements){

    //Asignamos memoria el vector de fases de CUDA
    double * retorno_segundo_d = NULL;
    cudaMalloc((void **) &retorno_segundo_d, size_double);

    // Lanzamos la ejecución del kernel

    buscar_segundo_kernel <<<blocksPerGrid, threadsPerBlock>>> (complejo_primero, puntos_d, epsilon,
                                                                retorno_segundo_d, numElements);
    cudaGetLastError();

    device_ptr<double> d_vec (retorno_segundo_d);
    device_ptr<double> d_vec_final = d_vec + numElements;
    device_ptr<double> result = min_element(d_vec, d_vec_final);

    //Recuperamos la posición del valor mínimo.
    unsigned int posicion_segundo = result - d_vec;

    double min_val = *result;

    cudaFree(retorno_segundo_d);

    if (min_val == inf){
        return -1;
    }

    return posicion_segundo;
}

inline int buscar_siguiente (Complex complejo_primero, Complex complejo_segundo, Complex * puntos_d, float epsilon, int numElements, double * retorno_siguiente_d){
    // Lanzamos la ejecución del kernel

    buscar_siguiente_kernel <<<blocksPerGrid, threadsPerBlock>>> (complejo_primero, complejo_segundo, puntos_d, epsilon,
                                                                  retorno_siguiente_d, numElements);

    cudaGetLastError();

    device_ptr<double> d_vec (retorno_siguiente_d);
    device_ptr<double> d_vec_final = d_vec + numElements;
    device_ptr<double> result = min_element(d_vec, d_vec_final);

    //Recuperamos la posición del valor mínimo.
    unsigned int posicion_siguiente = result - d_vec;

    double min_val = *result;

    if (min_val == inf){
        return -1;
    }

    return posicion_siguiente;
}

static __global__ void buscar_segundo_kernel(Complex primero, Complex * puntos, float epsilon,
                                             double * retorno, int n_puntos)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < n_puntos){

        double dist = abs(primero - puntos[i]);

        if (dist > 0 && dist <= epsilon){

            retorno[i] = arg(puntos[i] - primero) ;

            if (retorno[i] < 0){
                retorno[i] += 2 * M_PI;
            }

            retorno[i] -= acos(dist / epsilon);
        } else {
            retorno[i] = inf;
        }
    }
}

static __global__ void buscar_siguiente_kernel(Complex primero, Complex segundo, Complex * puntos, float epsilon,
                                               double * retorno, int n_puntos)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < n_puntos){

        double dist = abs(puntos[i] - segundo);

        if (dist > 0 && dist <= epsilon && puntos[i] != primero && puntos[i] != segundo){

            double fase = arg((puntos[i] - segundo) / (primero - segundo));

            if (fase < 0){
                fase += 2 * M_PI;
            }

            double aco1 = acos(dist / epsilon);
            double aco2 = acos(abs(primero - segundo) / epsilon);

            if (fase == 0){
                retorno[i] = 2 * M_PI - aco1 - aco2;
            } else if (fase < aco2){
                retorno[i] = fase + aco1 - aco2;
            } else{
                retorno[i] = fase - aco1 - aco2;
            }

            if (retorno[i] < 0){
                retorno[i] += 2 * M_PI;
            }

        } else {
            retorno[i] = inf;
        }
    }
}

//Funciones para operar con números complejos.
static __device__ __host__ inline double abs(Complex a)
{
    //return hypot((float)a.x, (float) a.y);
    
    float t,x = a.x,y = a.y;
    x = abs(x);
    y = abs(y);
    t = min(x,y);
    x = max(x,y);
    t = t/x;
    
    return x*sqrt(1+t*t);
}

static __device__ __host__ inline Complex operator-(Complex a, Complex b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    return a;
}

static __device__ __host__ inline double arg(Complex a)
{
    return atan2(a.y, a.x);
}

static __device__ __host__ inline Complex operator/(Complex a, Complex b)
{
    Complex c;
    double i = pow(b.x,2) + pow(b.y,2);
    c.x = (a.x * b.x + a.y * b.y)/i;
    c.y = (a.y * b.x - a.x * b.y)/i;
    return c;
}

static __device__ __host__ inline bool operator!=(Complex a, Complex b)
{
    if ((a.x != b.x) && (a.y != b.y))
        return true;
    
    return false;
}