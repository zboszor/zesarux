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

		if (byte_repetido[i]!=1 || repeticiones[i]!=i+1) {
			printf ("error\n");
			exit(1);
		}
	}
}


void codetests_main(void)
{
	printf ("Running repetitions code\n");
	codetests_repetitions();
}
