//include C++
#include <iostream>
#include <vector>
#include <complex>
#include <string>
#include <limits>

//include CUDA
#include <cuda_runtime.h>


#define inf 0x7f800000
#define pi 3.1416

typedef float2 Complex;


using namespace std;

//Función principal para resolver e_hull con cuda.
extern "C"
std::vector <float *> * bnd_cuda(vector<complex<double> > templates, complex <double> p0, double infinito, vector<float> fas, vector<float> mag);



//Funciones de kernel
static __global__ void bnd_kernel(float * fases, float * mag, float infinito,
                                  int puntos_mag, int puntos_fas, Complex * p,
                                  Complex p0, int nElementos, float * sabanaEstabilidadRuido, float * sabanaSeguimiento, 
				  float * sabanaRPS, float * sabanaRPE, float * sabanaEC);

//Funciones accesorias GPU

static __device__ __host__ inline float abs(Complex a);
static __device__ __host__ inline Complex operator/(Complex a, Complex b);
static __device__ __host__ inline Complex operator+(Complex a, Complex b);


std::vector <float *> * bnd_cuda (vector <complex <double> >templates, complex <double> p0, double infinito,
                   vector <float> fas, vector <float> mag){

    int nElementos = templates.size();

    size_t tamano = nElementos * sizeof (Complex);
    size_t tamano_float = sizeof (float);
    int fas_size = fas.size();
    int mag_size = mag.size();

    float infinito_sabana;

    if (infinito < 0){
        infinito_sabana = numeric_limits<float>::infinity();
    }else {
        infinito_sabana = infinito;
    }

    //Creamos los vectores para pasar a cuda.
    Complex * p_h = (Complex *) malloc (tamano);
    float * fas_h =  fas.data();
    float * mag_h = mag.data();

    for (int i = 0; i < nElementos; i++){
        p_h[i].x = (float) templates.at(i).real();
        p_h[i].y = (float) templates.at(i).imag();
    }

    //Copiamos la memoria del dispositivo los datos necesarios.
    cudaError_t err = cudaSuccess;

    Complex * p_d = NULL;
    err = cudaMalloc((void **) &p_d, tamano);

    if (err != cudaSuccess)
    {
        cerr << "Error al reservar memoria en CUDA (error code " <<  cudaGetErrorString(err) << ")!" << endl ;
        exit(EXIT_FAILURE);
    }

    cudaMemcpy(p_d, p_h, tamano, cudaMemcpyHostToDevice);

    float * fas_d = NULL;
    cudaMalloc((void **) &fas_d, fas_size * tamano_float);
    cudaMemcpy(fas_d, fas_h, fas_size * tamano_float, cudaMemcpyHostToDevice);

    float * mag_d = NULL;
    cudaMalloc((void **) &mag_d, mag_size * tamano_float);
    cudaMemcpy(mag_d, mag_h, mag_size * tamano_float, cudaMemcpyHostToDevice);

    int sabana_size = fas_size * mag_size * tamano_float;

    
    //Declaramos las sábanas
    
    /*sabanaEstabilidadRuido->append(vectorEstabilidadRuido);
        sabanaSeguimiento->append(vectorSeguimiento);
        sabanaRPS->append(vectorRPS);
        sabanaRPE->append(vectorRPE);
        sabanaEC->append(vectorEC);*/
    
    float * sabanaEstabilidadRuido_d = NULL;
    float * sabanaSeguimiento_d = NULL;
    float * sabanaRPS_d = NULL;
    float * sabanaRPE_d = NULL;
    float * sabanaEC_d = NULL;
    
    cudaMalloc((void **) &sabanaEstabilidadRuido_d, sabana_size);
    cudaMalloc((void **) &sabanaSeguimiento_d, sabana_size);
    cudaMalloc((void **) &sabanaRPS_d, sabana_size);
    cudaMalloc((void **) &sabanaRPE_d, sabana_size);
    cudaMalloc((void **) &sabanaEC_d, sabana_size);

    Complex p0_d; p0_d.x = p0.real(), p0_d.y = p0.imag();

    int threadsPerBlock = 128;
    int blocksPerGrid = (fas_size + threadsPerBlock - 1) / threadsPerBlock;

    bnd_kernel <<<blocksPerGrid, threadsPerBlock>>>(fas_d, mag_d, infinito_sabana, mag_size,
                                                    fas_size, p_d, p0_d , nElementos, sabanaEstabilidadRuido_d, sabanaSeguimiento_d
						   , sabanaRPS_d, sabanaRPE_d, sabanaEC_d);

    cudaGetLastError();

    float * sabanaEstabilidadRuido_h = (float *) malloc (sabana_size);
    float * sabanaSeguimiento_h = (float *) malloc (sabana_size);
    float * sabanaRPS_h = (float *) malloc (sabana_size);
    float * sabanaRPE_h = (float *) malloc (sabana_size);
    float * sabanaEC_h = (float *) malloc (sabana_size);

    cudaMemcpy(sabanaEstabilidadRuido_h, sabanaEstabilidadRuido_d, sabana_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(sabanaSeguimiento_h, sabanaSeguimiento_d, sabana_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(sabanaRPS_h, sabanaRPS_d, sabana_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(sabanaRPE_h, sabanaRPE_d, sabana_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(sabanaEC_h, sabanaEC_d, sabana_size, cudaMemcpyDeviceToHost);

    cudaFree(p_d);
    cudaFree(fas_d);
    cudaFree(mag_d);
    cudaFree(sabanaEstabilidadRuido_d);
    cudaFree(sabanaSeguimiento_d);
    cudaFree(sabanaRPS_d);
    cudaFree(sabanaRPE_d);
    cudaFree(sabanaEC_d);

    free(p_h);
        
    std::vector <float *> * vec = new std::vector <float *> ();
    
    vec->push_back(sabanaEstabilidadRuido_h);
    vec->push_back(sabanaSeguimiento_h);
    vec->push_back(sabanaRPS_h);
    vec->push_back(sabanaRPE_h);
    vec->push_back(sabanaEC_h);
    
    return vec;
}


static __global__ void bnd_kernel(float *fases, float *mag, float infinito, int puntos_mag, int puntos_fas,
                                  Complex * p, Complex p0, int nElementos, float * sabanaEstabilidadRuido, float * sabanaSeguimiento, 
				  float * sabanaRPS, float * sabanaRPE, float * sabanaEC){

    int i = blockDim.x * blockIdx.x + threadIdx.x;

    int contador, contador2;
    Complex L;
    Complex aux_complex;

    float dEstabilidadRuido;
    float dRPS;
    float dRPE;
    float dEC;
    float dSeguimiento;
    
    float dTempEstabilidadRuidoSeguimiento;
    float dTempRPS;
    float dTempRPE;
    float dTempEC;

    if (i < puntos_fas) {
        for (contador = 0; contador < puntos_mag; contador++){

            L.x = pow(10.,mag[contador]/20.) * cos (fases[i] * M_PI /180.);
            L.y = pow(10.,mag[contador]/20.) * sin (fases[i] * M_PI /180.);
	    
	    

            dEstabilidadRuido = -infinito;
	    dRPS = -infinito;
	    dRPE = -infinito;
	    dEC = -infinito;
	    dSeguimiento = infinito;

            for (contador2 = 0; contador2 < nElementos; contador2++){
		
		aux_complex = (p0 / p[contador2]) + L;

                //Estabilidad y ruido del sensor
                dTempEstabilidadRuidoSeguimiento = 20 * log10 (abs (L / aux_complex));

                //Rechazo de perturbaciones a la salida de la planta
                dTempRPS =  20 * log10 (abs ((p0 / p[contador2]) / aux_complex));

                //Rechado de perturbaciones a la entrada de la planta
                dTempRPE = 20 * log10 (abs (p0 / aux_complex));

                //Esfuerzo de control
                dTempEC = 20 * log10 (abs ((L / p[contador2]) / aux_complex));

                if (dTempEstabilidadRuidoSeguimiento > dEstabilidadRuido){
                    dEstabilidadRuido = dTempEstabilidadRuidoSeguimiento;
                } else if (dTempEstabilidadRuidoSeguimiento < dSeguimiento){
                    dSeguimiento = dTempEstabilidadRuidoSeguimiento;
                }
                if (dTempRPS > dRPS){
                    dRPS = dTempRPS;
                }
                if (dTempRPE > dRPE){
                    dRPE = dTempRPE;
                }
                if (dTempEC > dEC) {
                    dEC = dTempEC;
                }
            }

            sabanaEstabilidadRuido [i * puntos_mag + contador] = dEstabilidadRuido;
	    sabanaSeguimiento [i * puntos_mag + contador] = dSeguimiento;
	    sabanaRPS [i * puntos_mag + contador] = dRPS;
	    sabanaRPE [i * puntos_mag + contador] = dRPE;
	    sabanaEC [i * puntos_mag + contador] = dEC;
        }
    }
}

//Funciones para operar con números complejos.
static __device__ __host__ inline float abs(Complex a)
{
    //return hypot(a.x, a.y);
    
    float t,x = a.x,y = a.y;
    x = abs(x);
    y = abs(y);
    t = min(x,y);
    x = max(x,y);
    t = t/x;
    
    return x*sqrt(1+t*t);
}

static __device__ __host__ inline Complex operator/(Complex a, Complex b)
{
    Complex c;
    float i = pow(b.x,2) + pow(b.y,2);
    c.x = (a.x * b.x + a.y * b.y)/i;
    c.y = (a.y * b.x - a.x * b.y)/i;
    return c;
}


static __device__ __host__ inline Complex operator+(Complex a, Complex b)
{
    a.x = a.x + b.x;
    a.y = a.y + b.y;
    return a;
}
