//include C++
#include <iostream>
#include <vector>
#include <complex>
#include <string>
#include <cmath>


//include OpenCL
#include <CL/cl.hpp>


#define inf 0x7f800000
#define pi 3.1416

using namespace std;
using namespace cl;

typedef float2 Complex;

int threadsPerBlock = 128;
int blocksPerGrid = 0;

//Función principal para resolver e_hull con cuda.
extern "C"
vector<complex<double> > e_hull_opencl(vector <complex <double> > puntos, float epsilon);


//Funciones internas
int buscar_siguiente (Complex complejo_primero, Complex complejo_segundo, Complex * puntos_d, float epsilon, int numElements,
                      double * retorno_siguiente_d);


vector <complex <double> > e_hull_opencl(vector<complex<double> > puntos, float epsilon){


    vector <int> indice_contorno;

    // Creamos los tamaños de los distintos vectores
    int numElements = puntos.size();
    size_t size_complex = numElements * sizeof(Complex);
	size_t size_double = numElements * sizeof(double);
	size_t size_double = numElements * sizeof(int);
    
	///////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Buscar Primero /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    //Creamos el vector de complejos
    Complex * puntos_h = (Complex *) malloc(size_complex);
    
    double numDe = -inf;
    int posicion_primero = 0;

    for (int i = 0; i < numElements; i++){
            
        puntos_h[i].x = real(puntos.at(i));
	
        puntos_h[i].y = imag(puntos.at(i));
	
	if (puntos_h[i].y > numDe){
            posicion_primero = i;
            numDe = puntos_h[i].y;
        }


    }

    indice_contorno.push_back(posicion_primero);

    //Guardamos el primero número complejo del contorno
    Complex complejo_primero;
    complejo_primero.x = puntos_h[posicion_primero].x;
    complejo_primero.y = puntos_h[posicion_primero].y;

    ///////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Buscar Segundo /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

	//Inicialización
    vector<Platform> platforms;
    err = Platform::get(&platforms);
    if(err)cout << "Error en get platforms (" << err <<")." << endl;
    
    vector<cl::Device> devices;
    err = platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if(err)cout << "Error en get devices (" << err <<")." << endl;
    
    Context * context;
    context = new Context(CL_DEVICE_TYPE_GPU, 0, NULL, NULL, &err);
    if(err)cout << "Error en new Context (" << err <<")." << endl;


    cl::Context context({default_device});

    cl::Program::Sources sources;

    // kernel buscar segundo
    std::string kernel_code=
			"void kernel buscar_segundo(Complex primero, global Complex * puntos, float epsilon, global double * retorno, int n_puntos,"
			"global int * compartida)"
			"{"
    		"	int i = get_global_id(0);"
			"	int local_index = get_local_id(0);"
			"	result[get_local_id(0)] = inf;"

    		"	if (i < n_puntos){"

        	"		double dist = abs(primero - puntos[i]);"

        	"		if (dist > 0 && dist <= epsilon){"

            "			compartida[local_index] = arg(puntos[i] - primero) ;"

            "			if (compartida[local_index] < 0){"
            "    			compartida[local_index] += 2 * M_PI;"
            "			}"

            "			compartida[local_index] -= acos(dist / epsilon);"
        	"		} else {"
            "			compartida[local_index] = inf;"
        	"		}"
    		"	} else {"
			"		compartida[local_indes] = inf;"
			"	}"
				
			"	barrier(CLK_LOCAL_MEM_FENCE);"
  			"	for(int offset = 1; offset < get_local_size(0); offset <<= 1) {"
    		"		int mask = (offset << 1) - 1;"
    		"		if ((local_index & mask) == 0) {"
      		"			float other = compartida[local_index + offset];"
      		"			float mine = compartida[local_index];"
      		"			compartida[local_index] = (mine < other) ? mine : other;"
			"			result[local_index] = (mine < other) ? local_index : local_index + offset;"
    		"		}"
    		"		barrier(CLK_LOCAL_MEM_FENCE);"
  			"	}"
			"}";

    sources.push_back({kernel_code.c_str(),kernel_code.length()});

    cl::Program program(context,sources);
    if(program.build({default_device})!=CL_SUCCESS){
        std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)<<"\n";
        exit(1);
    }


    //Copiamos a la memoria de OpenCL los datos.
	cl::Buffer * puntos_d = new cl::Buffer(context,CL_MEM_READ_WRITE,size_complex);
	cl::CommandQueue queue(context,default_device);
	queue.enqueueWriteBuffer(*puntos_d,CL_TRUE,0,size_complex,*puntos_h);


    ///////////////////////////////////////////////////////////////////////////////
	//Asignamos memoria el vector de retorno de OpenCL
	cl::Buffer * retorno_d = new cl::Buffer(context,CL_MEM_READ_WRITE,size_double);

	//Creamos el buffer intermedio para OpenCL
	cl::Buffer * partials_buffer =  new cl::Buffer(context, CL_MEM_READ_WRITE, size_double);

    // Lanzamos la ejecución del kernel
    cl::KernelFunctor buscar_segundo(cl::Kernel(program,"buscar_segundo"),queue,cl::NullRange,cl::NDRange(numElements),cl::NullRange);
    buscar_segundo(complejo_primero, puntos_d, epsilon, retorno_d, numElements, partials_buffer);


	int * retorno_h = (int *) malloc(sizeof(int));

	queue.enqueueReadBuffer(retorno_d,CL_TRUE,0,sizeof(int),retorno_h);

    //Recuperamos la posición del valor mínimo.
    int posicion_segundo = *retorno_h[0];


    if (posicion_segundo == inf){
         posicion_segundo = -1;
    }

	///////////////////////////////////////////////////////////////////////////////

    if (posicion_segundo < 0){
        delete [] puntos_d;
		delete [] retorno_d;
		delete [] partials_buffer;
		delete context;
        free (puntos_h);
		free (retorno_h);

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

	/////////////////////////////////////////////////////////////////////////////////////////

	// kernel buscar siguiente
    kernel_code=
			"void kernel buscar_siguiente(Complex primero, Complex segundo, global Complex * puntos, float epsilon, global double * retorno, int n_puntos,"
			"global int * compartida)"
			"{"
    		"	int i = get_global_id(0);"
			"	int local_index = get_local_id(0);"
			"	result[get_local_id(0)] = inf;"

    		"	if (i < n_puntos){"

        	"		double dist = abs(puntos[i] - segundo);"

        	"		if (ddist > 0 && dist <= epsilon && puntos[i] != primero && puntos[i] != segundo){"

            "			double fase = arg((puntos[i] - segundo) / (primero - segundo));"

            "			if (fase < 0){"
            "    			fase += 2 * M_PI;"
            "			}"

			"			double aco1 = acos(dist / epsilon);
            "			double aco2 = acos(abs(primero - segundo) / epsilon);

            "			if (fase == 0){
            "			    compartida[local_index] = 2 * M_PI - aco1 - aco2;
            "			} else if (fase < aco2){
            "			    compartida[local_index] = fase + aco1 - aco2;
            "			} else{
            "			    compartida[local_index] = fase - aco1 - aco2;
            "			}

            "			if (retorno[i] < 0){
            "			    compartida[local_index] += 2 * M_PI;
            "			}
        	"		} else {"
            "			compartida[local_index] = inf;"
        	"		}"
    		"	} else {"
			"		compartida[local_indes] = inf;"
			"	}"
				
			"	barrier(CLK_LOCAL_MEM_FENCE);"
  			"	for(int offset = 1; offset < get_local_size(0); offset <<= 1) {"
    		"		int mask = (offset << 1) - 1;"
    		"		if ((local_index & mask) == 0) {"
      		"			float other = compartida[local_index + offset];"
      		"			float mine = compartida[local_index];"
      		"			compartida[local_index] = (mine < other) ? mine : other;"
			"			result[local_index] = (mine < other) ? local_index : local_index + offset;"
    		"		}"
    		"		barrier(CLK_LOCAL_MEM_FENCE);"
  			"	}"
			"}";

	cl::Program::Sources sources2;
    sources2.push_back({kernel_code.c_str(),kernel_code.length()});

	cl::Program program2(context,sources2);
    if(program2.build({default_device})!=CL_SUCCESS){
        std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)<<"\n";
        exit(1);
    }


	// Lanzamos la ejecución del kernel
    cl::KernelFunctor buscar_siguiente(cl::Kernel(program2,"buscar_siguiente"),queue,cl::NullRange,cl::NDRange(numElements),cl::NullRange);
    buscar_siguiente(complejo_primero, complejo_segundo, puntos_d, epsilon, retorno_d, numElements, partials_buffer);

	//Recuperamos el vector retorno
	queue.enqueueReadBuffer(retorno_d,CL_TRUE,0,sizeof(int),retorno_h);

    //Recuperamos la posición del valor mínimo.
    int punto_siguiente = *retorno_h[0];


    if (punto_siguiente == inf){
         punto_siguiente = -1;
    }

	/////////////////////////////////////////////////////////////////////////////////////////



    if (punto_siguiente < 0){
        delete [] puntos_d;
		delete [] retorno_d;
		delete [] partials_buffer;
		delete context;
        free (puntos_h);
		free (retorno_h);

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

		//llamamos al kernel
        buscar_siguiente(complejo_primero, complejo_segundo, puntos_d, epsilon, retorno_d, numElements, partials_buffer);

		//Recuperamos el vector retorno
		queue.enqueueReadBuffer(retorno_d,CL_TRUE,0,sizeof(int),retorno_h);

    	//Recuperamos la posición del valor mínimo.
    	punto_siguiente = *retorno_h[0];


    	if (punto_siguiente == inf){
         	punto_siguiente = -1;
    	}


        if (punto_siguiente < 0){
            delete [] puntos_d;
			delete [] retorno_d;
			delete [] partials_buffer;
			delete context;
        	free (puntos_h);
			free (retorno_h);

            vector <complex <double> > vector_nulo;
            return vector_nulo;
        }

        ////////////////////////////////////////////////////////////////////////////

    }

    ///////////////////////////////////////////////////////////////////////////////


		delete [] puntos_d;
		delete [] retorno_d;
		delete [] partials_buffer;
		delete context;
        free (puntos_h);
		free (retorno_h);

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

