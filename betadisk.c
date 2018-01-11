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
#include "mem128.h"
#include "screen.h"


z80_bit betadisk_enabled={0};


z80_byte *betadisk_memory_pointer;

z80_bit betadisk_active={0};

z80_bit betadisk_allow_boot_48k={1};


int betadisk_nested_id_core;
int betadisk_nested_id_peek_byte;
int betadisk_nested_id_peek_byte_no_time;


char trd_file_name[PATH_MAX]="";

char *betadisk_rom_file_name="trdos.rom";

z80_bit trd_enabled={0};

void betadisk_trdoshandler_read_sectors(void);

//http://problemkaputt.de/zxdocs.htm#spectrumdiscbetabetaplusbeta128diskinterfacetrdos

/*
Estado actual de la emulación de floppy: parece que detecta el floppy aunque da siempre un disk error
El único caso que no da disk error es al formatear
*/



int betadisk_check_if_rom_area(z80_int dir)
{

	//Si no betadisk activo, volver
	if (betadisk_active.v==0) return 0;

	//Si maquina 128k y es rom 0, volver
	if (MACHINE_IS_SPECTRUM_128_P2_P2A) {
		if (!(puerto_32765&16)) return 0;
	}

	if (dir<16384) return 1;
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

void betadisk_cambio_pagina(z80_int dir)
{



	if (betadisk_active.v) {
		if (dir>=0x4000) {
			//printf ("Unactivating betadisk rom space\n");
			betadisk_active.v=0;
		}
	}

	else {
		if (dir>=0x3C00 && dir<=0x3DFF) {
			//printf ("Activating betadisk rom space\n");
			betadisk_active.v=1;
		}
	}
}

void betadisk_reset(void)
{

	betadisk_active.v=0;

	//Al hacer reset, se activa betadisk en maquinas 48k

	if (MACHINE_IS_SPECTRUM_16_48 && betadisk_allow_boot_48k.v) {
		betadisk_active.v=1;
	}
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

	betadisk_cambio_pagina(reg_pc);


	//http://www.worldofspectrum.org/pub/sinclair/hardware-info/TR-DOS_Programming.txt

	//Debug
	//if (betadisk_check_if_rom_area(reg_pc) ) {
		//if (reg_pc==0x3D13 || reg_pc==0x3C13) {
		//	printf ("TR-DOS Call function %02XH on addr %04XH\n",reg_c,reg_pc);
		//}
	//}


	//Handler
	if (trd_enabled.v) {
		if (betadisk_check_if_rom_area(reg_pc) ) {
			/*if (reg_pc==0x1e3d) {
				char buffer_registros[8192];
				print_registers(buffer_registros);
				printf ("Handler for read sectors\n");
				printf ("%s\n",buffer_registros);
				betadisk_trdoshandler_read_sectors();
			}*/


			//Si A=0, lectura. Si A=255, escritura
			if (reg_pc==0x1e67 && reg_a==0) {
				char buffer_registros[8192];
				print_registers(buffer_registros);
				printf ("\n\nHandler for transfer_sectors\n");
				printf ("%s\n",buffer_registros);
				generic_footertext_print_operating("TRD");
				betadisk_trdoshandler_read_sectors();
			}

			/*if (reg_pc==0xef2) {
				char buffer_registros[8192];
				print_registers(buffer_registros);
				printf ("read_sector_DE_into_tmp_buffer:  equ 0x03F2\n");
				printf ("%s\n",buffer_registros);
			}*/

			/*if (reg_pc==0x1e36) {
				char buffer_registros[8192];
				print_registers(buffer_registros);
				printf ("sub_1e36h\n");
				printf ("%s\n",buffer_registros);
			}*/
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


z80_byte *trd_memory_pointer;

int betadisk_bytes_por_sector=256;
int betadisk_sectores_por_pista=16;


void trd_insert(void)
{

        //Si existe
        if (!si_existe_archivo(trd_file_name)) {
                debug_printf (VERBOSE_ERR,"File %s does not exist",trd_file_name);
                trd_disable();
                return;
        }


	trd_memory_pointer=malloc(TRD_FILE_SIZE);

        FILE *ptr_trdfile;
	ptr_trdfile=fopen(trd_file_name,"rb");

        if (!ptr_trdfile) {
                debug_printf (VERBOSE_ERR,"Unable to open trd file file");
                trd_disable();
                return;
        }

        int leidos=fread(trd_memory_pointer,1,TRD_FILE_SIZE,ptr_trdfile);

        fclose(ptr_trdfile);



}

z80_byte betadisk_get_byte_disk(int pista, int sector, int byte_en_sector)
{

	int bytes_por_pista=betadisk_sectores_por_pista*betadisk_bytes_por_sector;
	int offset=pista*bytes_por_pista+sector*betadisk_bytes_por_sector+byte_en_sector;

	if (offset>=TRD_FILE_SIZE) {
		debug_printf (VERBOSE_ERR,"Error. Trying to read beyond trd. Size: %ld Asked: %u. Disabling TRD",TRD_FILE_SIZE,offset);
                trd_disable();
                return 0;
	}

	z80_byte byte_leido=trd_memory_pointer[offset];
	
	//z80_byte caracter=byte_leido;
	//if (caracter<32 || caracter>127) caracter='.';
	//printf ("%c",caracter);

	return byte_leido;
}


void betadisk_trdoshandler_read_sectors(void)
{
	/*


read_sectors:                                               equ 0x1E3D
write_sectors:                                              equ 0x1E4D

A = número de sectores
D = pista del primer sector a usar (0..159)
E = primer sector a usar de la pista (0..15)
HL = dirección de memoria para carga o lectura de los sectores




	#05 - Read group of sectors. In HL must be  loaded address where  sector data
       should  be readed, B must be  loaded with  number of sectors to read, D
       must be loaded  with track number and E with sector number. If B loaded
       with  #00 then interface will only read sector address mark - useful if
       you only want check is there sector with defined number on the track.

	*/

	//z80_byte numero_sectores=reg_a;
	z80_byte numero_sectores=reg_b;
	z80_byte pista=reg_d;
	z80_byte sector=reg_e;
	int byte_en_sector;
	z80_int destino=reg_hl;

	printf ("Reading %d sectors from track %d sector %d to address %04XH\n",numero_sectores,pista,sector,destino);


		//poke_byte_no_time(TRD_SYM_trdos_variable_sector_rw_flag,reg_a);

	//prueba
	//if (numero_sectores>1) numero_sectores=1;
	//if (numero_sectores==0) numero_sectores=1;

	//numero_sectores++;
	//numero_sectores &=15;

	//temp prueba. con paralact consigue cargar el basic y el resto
	//if (sector==0 && pista==1) {

	if (destino>0x5d25) { //con esto paralaktika perfecto, agressor.trd perfecto, metalmanru.trd perfecto, seto.trd, recycler.trd


	//if (destino==0x5E3C) {
		
		//printf ("\nHACK for paralactika basic. increment number of sectors to read\n");
		//numero_sectores++;
	}

	//temp
	//if (numero_sectores) numero_sectores=1;

	//carga bytes 1024 bytes-> 4 sectores
	//carga bytes 1025 bytes-> 4 sectores
	//->tiene que haber otra rutina que cargue el sector final que no estamos capturando


	int leidos=0;

	for (;numero_sectores>0;numero_sectores--) {
		for (byte_en_sector=0;byte_en_sector<betadisk_bytes_por_sector;byte_en_sector++) {
			z80_byte byte_leido=betadisk_get_byte_disk(pista,sector,byte_en_sector);
			poke_byte_no_time(destino++,byte_leido);
			leidos++;
		}
		sector++;
	}

	printf ("\ntotal leidos: %d\n",leidos);
	
	//No error
	reg_a=0;
	Z80_FLAGS |=FLAG_Z;

	//??
	reg_hl=destino;
	reg_e=sector;
	reg_d=pista;

/*
   23807 | 1* | Sector number for sector read/write Tr-Dos functions
   23808 | 2* | Current address of buffer for Tr-Dos #05 and #06 functions
*/
	//23807 sector number
	//poke_byte_no_time(23807,sector);
	//poke_byte_no_time(23808,reg_l);
	//poke_byte_no_time(23809,reg_h);


	poke_byte_no_time(TRD_SYM_trdos_variable_current_sector,reg_e);
	poke_byte_no_time(TRD_SYM_trdos_variable_current_track,reg_d);



	//Return
	reg_pc=pop_valor();
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






void trd_enable(void)
{
        debug_printf (VERBOSE_INFO,"Enabling trd");
        trd_enabled.v=1;

        trd_insert();


}

void trd_disable(void)
{


        trd_enabled.v=0;


}




