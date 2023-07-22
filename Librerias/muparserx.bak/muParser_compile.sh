#!/bin/sh

#Compilamos la librería muParser


if [ $# != 0 ];
then
	if [ $1 = "clean" ]
	then
		rm -rfv build
	fi
else
	mkdir build
	cd build
	cmake ..
	make
	cd ..
fi



