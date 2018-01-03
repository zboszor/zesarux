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


#include "betadisk.h"
#include "cpu.h"
#include "debug.h"
#include "utils.h"
#include "operaciones.h"
#include "ula.h"


z80_bit betadisk_enabled={0};


z80_byte *betadisk_memory_pointer;

z80_bit betadisk_active={0};


int betadisk_nested_id_core;
int betadisk_nested_id_peek_byte;
int betadisk_nested_id_peek_byte_no_time;



char *betadisk_rom_file_name="trdos.rom";

//http://problemkaputt.de/zxdocs.htm#spectrumdiscbetabetaplusbeta128diskinterfacetrdos


int betadisk_check_if_rom_area(z80_int dir)
{
	if (betadisk_active.v && dir<16384) return 1;
	else return 0;
}

z80_byte betadisk_read_byte(z80_int dir)
{
	//printf ("Returning betadisk data address %d\n",dir);
	return betadisk_memory_pointer[dir];
}


z80_byte betadisk_peek_byte(z80_int dir,z80_byte value GCC_UNUSED)
{

	z80_byte valor_leido=debug_nested_peek_byte_call_previous(betadisk_nested_id_peek_byte,dir);

	if (betadisk_check_if_rom_area(dir)) {
       		//t_estados +=3;
		return betadisk_read_byte(dir);
	}

	//return betadisk_original_peek_byte(dir);
	return valor_leido;
}

z80_byte betadisk_peek_byte_no_time(z80_int dir,z80_byte value GCC_UNUSED)
{

	z80_byte valor_leido=debug_nested_peek_byte_no_time_call_previous(betadisk_nested_id_peek_byte_no_time,dir);

	if (betadisk_check_if_rom_area(dir)) {
                return betadisk_read_byte(dir);
        }

	//else return debug_nested_peek_byte_no_time_call_previous(betadisk_nested_id_peek_byte_no_time,dir);
	return valor_leido;
}



//Establecer rutinas propias
void betadisk_set_peek_poke_functions(void)
{
                debug_printf (VERBOSE_DEBUG,"Setting betadisk poke / peek functions");

	//Asignar mediante funciones de core anidados
	betadisk_nested_id_peek_byte=debug_nested_peek_byte_add(betadisk_peek_byte,"Betadisk peek_byte");
	betadisk_nested_id_peek_byte_no_time=debug_nested_peek_byte_no_time_add(betadisk_peek_byte_no_time,"Betadisk peek_byte_no_time");

}

//Restaurar rutinas de betadisk
void betadisk_restore_peek_poke_functions(void)
{
                debug_printf (VERBOSE_DEBUG,"Restoring original poke / peek functions before betadisk");


	debug_nested_peek_byte_del(betadisk_nested_id_peek_byte);
	debug_nested_peek_byte_no_time_del(betadisk_nested_id_peek_byte_no_time);
}




z80_byte cpu_core_loop_betadisk(z80_int dir GCC_UNUSED, z80_byte value GCC_UNUSED)
{
	//Llamar a anterior
	debug_nested_core_call_previous(betadisk_nested_id_core);


	//http://problemkaputt.de/zxdocs.htm#spectrumdiscbetabetaplusbeta128diskinterfacetrdos

	/*
	MEM:3C00h..3CFFh - Enable ROM & I/O Ports (Beta/TRDOSv1 and BetaPlus/TRDOSv4)
	MEM:3D00h..3DFFh - Enable ROM & I/O Ports (Beta128/TRDOSv5.0x)
	The enable region was originally at 3Cxxh (which is FFh-filled in Spectrum 16K/48K/Plus BIOSes), and was invoked via RANDOMIZE USR 15360 and 15363 from BASIC. For compatibility with the Spectrum 128 (which contains BIOS code in that region), the region was moved to 3Dxxh (which contains only character set data, but no program code) and is now invoked via USR 15616 and 15619. The first address is used to enter TRDOS,
  	RANDOMIZE USR 15360 or 15616  --> switch from BASIC to TRDOS ;3C00h/3D00h
  	RETURN (aka Y-key)            --> switch from TRDOS to BASIC
	The second address is used as prefix to normal BASIC commands, eg.:
  	RANDOMIZE USR 15363 or 15619:REM:LOAD"filename"              ;3C03h/3D03h

  	MEM:4000h..FFFFh - Disable ROM & I/O Ports (all versions)
	Opcode fetches outside of the ROM region (ie. at 4000h..FFFFh) do automatically disable the TRDOS ROM and I/O ports.

	*/

	if (betadisk_active.v) {
		if (reg_pc>=0x4000) {
			//printf ("Unactivating betadisk rom space\n");
			betadisk_active.v=0;
		}
	}

	else {
		if (reg_pc>=0x3C00 && reg_pc<=0x3DFF) {
			//printf ("Activating betadisk rom space\n");
			betadisk_active.v=1;
		}
	}

	//Para que no se queje el compilador, aunque este valor de retorno no lo usamos
	return 0;

}




void betadisk_set_core_function(void)
{
	debug_printf (VERBOSE_DEBUG,"Setting betadisk Core loop");
	//Asignar mediante nuevas funciones de core anidados
	betadisk_nested_id_core=debug_nested_core_add(cpu_core_loop_betadisk,"Betadisk core");
}




void betadisk_restore_core_function(void)
{
        debug_printf (VERBOSE_DEBUG,"Restoring original betadisk core");
	debug_nested_core_del(betadisk_nested_id_core);
}



void betadisk_alloc_memory(void)
{
        int size=BETADISK_SIZE;  

        debug_printf (VERBOSE_DEBUG,"Allocating %d kb of memory for betadisk emulation",size/1024);

        betadisk_memory_pointer=malloc(size);
        if (betadisk_memory_pointer==NULL) {
                cpu_panic ("No enough memory for betadisk emulation");
        }


}

int betadisk_load_rom(void)
{

        FILE *ptr_betadisk_romfile;
        int leidos=0;

        debug_printf (VERBOSE_INFO,"Loading betadisk rom %s",BETADISK_ROM_FILENAME);

        open_sharedfile(BETADISK_ROM_FILENAME,&ptr_betadisk_romfile);

  			ptr_betadisk_romfile=fopen(betadisk_rom_file_name,"rb");
        if (!ptr_betadisk_romfile) {
                        debug_printf (VERBOSE_ERR,"Unable to open ROM file");
        }

        if (ptr_betadisk_romfile!=NULL) {

                leidos=fread(betadisk_memory_pointer,1,BETADISK_SIZE,ptr_betadisk_romfile);
                fclose(ptr_betadisk_romfile);

        }



        if (leidos!=BETADISK_SIZE || ptr_betadisk_romfile==NULL) {
                debug_printf (VERBOSE_ERR,"Error reading betadisk rom");
                return 1;
        }

        return 0;


}



void betadisk_enable(void)
{

  if (!MACHINE_IS_SPECTRUM) {
    debug_printf(VERBOSE_INFO,"Can not enable betadisk on non Spectrum machine");
    return;
  }

	if (betadisk_enabled.v) return;


	betadisk_alloc_memory();
	if (betadisk_load_rom()) return;

	betadisk_set_peek_poke_functions();

	betadisk_set_core_function();

	betadisk_active.v=0;


	betadisk_enabled.v=1;


}

void betadisk_disable(void)
{
	if (betadisk_enabled.v==0) return;

	betadisk_restore_peek_poke_functions();

	free(betadisk_memory_pointer);

	betadisk_restore_core_function();

	betadisk_enabled.v=0;
}



