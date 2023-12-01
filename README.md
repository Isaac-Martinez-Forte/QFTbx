# How to use

## Dependencies

	- Minimal
		- cmake >= 3.5
		- Qt >= 5.15
		- cxx compiler >= c++11
	- Optional
		- cuda >= 7
		- OpenMp >= 11


Full documentation isn't available but you can use Doxygen if you need to generate some of this documentation. 

You can configure dependencies in CMake file;  automatical configuration is: 

	- OPTION (USE_CLANG "Use CLANG" OFF)
	- OPTION (USE_OpenMP "Use OpenMP" ON)
	- OPTION (USE_CUDA "Use CUDA" OFF)
	- OPTION (USE_Doxygen "Use Doxygen" OFF)



## Compile

Instructions:

	- mkdir build
	- cd build
	- cmake ..
	- make
	
If you previously fulfill the dependencies, you can compile it in linux (using gcc) or in windows (using MinGW).

## Execution

	- ./QFTbx
	
This program is under development. Something may be wrong. An installer isn't included.

# License

GNU GENERAL PUBLIC LICENSE Version 3


# Authors 

Isaac Martínez Forte - <isaac.martinez@upct.es>

Joaquín Cervera López - <jcervera@um.es>


# Bibliography

[I. Martı́nez-Forte y J. Cervera [2021]]. Accelerated quantitative feedback theory interval automatic loop shaping algorithm. International Journal of Robust and Nonlinear Control 31, no 9: 4378–4396. https://doi.org/10.1002/rnc.5499 	http://hdl.handle.net/10201/123363

[I. Martı́nez-Forte y J. Cervera [2022]]. Aceleración de algoritmos intervalares de ajuste automático del lazo en QFT. Tesis Doctoral Universidad de Murcia [2022]. http://hdl.handle.net/10201/122610

[Martínez-Forte[2013]]I. Martínez-Forte. *QFTbx,  herramienta  de  diseño  QFT:  especificación de requisitos y prototipado*. Proyecto Fin de Carrera Universidad de Murcia [2013] http://hdl.handle.net/10201/61459.

[Martínez-Forte[2014]]I. Martínez-Forte. *Paralelización de algoritmos QFT mediante OpenMP y CUDA*. Proyecto Fin de Máster Universidad de Murcia [2014] http://hdl.handle.net/10201/61460.


