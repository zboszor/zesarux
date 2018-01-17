/*
    ZEsarUX  ZX Second-Emulator And Released for UniX
    Copyright (C) 2013 Cesar Hernandez Bano

    This file is part of ZEsarUX.

    ZEsarUX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#if defined(__APPLE__)
        #include <sys/syslimits.h>
#endif


#include "codetests.h"
#include "cpu.h"
#include "debug.h"
#include "utils.h"
#include "operaciones.h"
#include "ula.h"
#include "mem128.h"
#include "screen.h"


void codetests_repetitions(void)
{

	z80_byte repetitions0[]={1,2,3,4,5,6,7,8,9,10};
	z80_byte repetitions1[]={1,1,3,4,5,6,7,8,9,10};
	z80_byte repetitions2[]={1,1,1,4,5,6,7,8,9,10};
	z80_byte repetitions3[]={1,1,1,1,5,6,7,8,9,10};
	z80_byte repetitions4[]={1,1,1,1,1,6,7,8,9,10};

	//int util_get_byte_repetitions(z80_byte *memoria,int longitud,z80_byte *byte_repetido)

	int repeticiones[5];
	z80_byte byte_repetido[5];

	int i;
	z80_byte *puntero=NULL;

	for (i=0;i<5;i++) {
		if      (i==0) puntero=repetitions0;
		else if (i==1) puntero=repetitions1;
		else if (i==2) puntero=repetitions2;
		else if (i==3) puntero=repetitions3;
		else if (i==4) puntero=repetitions4;

		repeticiones[i]=util_get_byte_repetitions(puntero,10,&byte_repetido[i]);

		printf ("step %d repetitions: %d byte_repeated: %d\n",i,repeticiones[i],byte_repetido[i]);

		//Validar cantidad de valores repetidos y que byte repetido
		printf ("expected: repetitions: %d byte_repeated: 1\n",i+1);
		if (byte_repetido[i]!=1 || repeticiones[i]!=i+1) {
			printf ("error\n");
			exit(1);
		}
	}
}

void coretests_dumphex(z80_byte *ptr,int longitud)
{
	while (longitud) {
		printf ("%02X ",*ptr);
		ptr++;
		longitud--;
	}
}



//mostrar unos cuantos del inicio y del final
void coretests_dumphex_inicio_fin(z80_byte *ptr,int longitud,int max_mostrar)
{

	int mostrar;
	int cortado=0;
	if (longitud>max_mostrar*2) {
		mostrar=max_mostrar;
		cortado=1;
	}
	else mostrar=longitud;

	coretests_dumphex(ptr,mostrar);


	if (cortado) {
		printf (" ... ");
		coretests_dumphex(ptr+longitud-mostrar,mostrar);
	}

}


void coretests_compress_repetitions(void)
{


#define MAX_COMP_REP_ARRAY 1024

	z80_byte repetitions[MAX_COMP_REP_ARRAY];

	z80_byte compressed_data[MAX_COMP_REP_ARRAY*2];

	int max_array=MAX_COMP_REP_ARRAY; //siempre menor o igual que MAX_COMP_REP_ARRAY. tamanyo de los datos a analizar

        int i;

	int max_veces=MAX_COMP_REP_ARRAY; //Siempre menor o igual que MAX_COMP_REP_ARRAY. cuantos bytes repetimos

        for (i=0;i<=max_veces;i++) {

		int j;

		//Inicializar con valores consecutivos
		for (j=0;j<max_array;j++) {
			repetitions[j]=j&255;
		}

		//Meter valores "0" al principio
		for (j=0;j<=i;j++) {
			repetitions[j]=0;
		}

		//Meter valores "1" al final 
		for (j=0;j<=i;j++) {
			repetitions[max_array-1-j]=1;
		}

                //repeticiones[i]=util_get_byte_repetitions(puntero,10,&byte_repetido[i]);
		printf ("step %d length: %d. 0's at beginning: %d. 1's at end: %d\n",i,max_array,i+1,i+1);

		coretests_dumphex_inicio_fin(repetitions,max_array,20);

		printf ("\n");

		int longitud_destino=util_compress_data_repetitions(repetitions,compressed_data,max_array,0xDD);

		printf ("compressed length: %d\n",longitud_destino);

		//coretests_dumphex(compressed_data,longitud_destino);
		coretests_dumphex_inicio_fin(compressed_data,longitud_destino,20);
		printf ("\n");



		//Validar, pero solo para iteraciones < 256. mas alla de ahi, dificil de calcular
		if (i<256-4) {
			//Validacion solo de longitud comprimida. El contenido, hacer una validacion manual
			int valor_esperado_comprimido=max_array;
			if (i>3) valor_esperado_comprimido=max_array-(i-3)*2;

			printf ("Expected length: %d\n",valor_esperado_comprimido);

			if (valor_esperado_comprimido!=longitud_destino) {
                	        printf ("error\n");
                        	exit(1);
	                }
		}

		printf ("\n");
        }

}


void codetests_main(void)
{
	printf ("\nRunning repetitions code\n");
	codetests_repetitions();

	printf ("\nRunning compress repetitions code\n");
	coretests_compress_repetitions();

}
