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


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cpu.h"
#include "debug.h"
#include "operaciones.h"
#include "zx8081.h"
#include "mem128.h"
#include "ay38912.h"
#include "compileoptions.h"
#include "tape_smp.h"
#include "audio.h"
#include "screen.h"
#include "menu.h"
#include "tape.h"
#include "snap.h"
#include "snap_z81.h"
#include "snap_zx8081.h"
#include "utils.h"
#include "ula.h"
#include "joystick.h"
#include "realjoystick.h"
#include "z88.h"
#include "chardetect.h"
#include "jupiterace.h"
#include "cpc.h"
#include "timex.h"
#include "zxuno.h"
#include "ulaplus.h"
#include "chloe.h"
#include "prism.h"
#include "diviface.h"
#include "snap_rzx.h"
#include "divmmc.h"


#include "autoselectoptions.h"

#if defined(__APPLE__)
        #include <sys/syslimits.h>
#endif

#define ZSF_NOOP_ID 0
#define ZSF_MACHINEID 1
#define ZSF_Z80_REGS_ID 2
#define ZSF_MOTO_REGS_ID 3
#define ZSF_RAMBLOCK 4
#define ZSF_SPEC128_MEMCONF 5
#define ZSF_SPEC128_RAMBLOCK 6
#define ZSF_AYCHIP 7
#define ZSF_ULA 8
#define ZSF_ULAPLUS 9
#define ZSF_ZXUNO_RAMBLOCK 10
#define ZSF_ZXUNO_CONF 11
#define ZSF_ZX8081_CONF 12


int zsf_force_uncompressed=0; //Si forzar bloques no comprimidos

/*
Format ZSF:
* All numbers are LSB

Every block is defined with a header:

2 bytes - 16 bit: block ID
4 bytes - 32 bit: block Lenght
After these 6 bytes, the data for the block comes.



-Block ID 0: ZSF_NOOP
No operation. Block lenght 0

-Block ID 1: ZSF_MACHINEID
Defines which machine is this snapshot. Normally it should come after any other block, but can appear later
Even it could be absent, so the snapshot will be loaded according to the current machine

Byte fields:
0: Machine ID. Same ID  defined on function set_machine_params from cpu.c

-Block ID 2: ZSF_Z80_REGS
Z80 CPU Registers

-Block ID 3: ZSF_MOTO_REGS
Motorola CPU Registers

-Block ID 4: ZSF_RAMBLOCK
A ram binary block
Byte Fields:
0: Flags. Currently: bit 0: if compressed with repetition block DD DD YY ZZ, where 
    YY is the byte to repeat and ZZ the number of repetitions (0 means 256)
1,2: Block start address
3,4: Block lenght (if 0, means 65536. Value 0 only used on Inves)
5 and next bytes: data bytes



-Block ID 5: ZSF_SPEC128_MEMCONF
Byte Fields:
0: Port 32765 contents
1: Port 8189 contents
2: Total memory multiplier: 1 for 128kb ram, 2 for 256 kb ram, 4 for 512 kb ram

-Block ID 6: ZSF_SPEC128_RAMBLOCK
A ram binary block for a spectrum 128, p2 or p2a machine
Byte Fields:
0: Flags. Currently: bit 0: if compressed with repetition block DD DD YY ZZ, where
    YY is the byte to repeat and ZZ the number of repetitions (0 means 256)
1,2: Block start address (currently unused)
3,4: Block lenght
5: ram block id (0..7) for a spectrum 128k for example
6 and next bytes: data bytes


-Block ID 7: ZSF_AYCHIP
Byte fields:
0: AY Chip number, starting at 0. A normal spectrum will be only the 0. A turbosound, 0 and 1, etc
1: Current AY Chip selected (variable ay_chip_selected). Redundant in all ZSF_AYCHIP blocks
2: AY Last Register selection
3-18: AY Chip contents

-Block ID 8: ZSF_ULA
Byte fields:
0: Border color (Last out to port 254 AND 7)


-Block ID 9: ZSF_ULAPLUS
Byte fields:
0: ULAplus mode
1: ULAplus last out to port BF3B
2: ULAplus last out to port FF3B
3-66: ULAplus palette


-Block ID 10: ZSF_ZXUNO_RAMBLOCK
A ram binary block for a zxuno
Byte Fields:
0: Flags. Currently: bit 0: if compressed with repetition block DD DD YY ZZ, where
    YY is the byte to repeat and ZZ the number of repetitions (0 means 256)
1,2: Block start address (currently unused)
3,4: Block lenght
5: ram block id 
6 and next bytes: data bytes



-Block ID 11: ZSF_ZXUNO_CONF
Ports and internal registers of ZXUNO machine
Byte fields:
0: Last out to port FC3B
1-256: 256 internal ZXUNO registers
257: Flash SPI bus index
258: Flash SPI next read byte   
259: Flash SPI status register
260-262: 24 bit value with last spi write address
263-265: 24 bit value with last spi read address
266-273: 8 byte with spi bus contents


-Block ID 12: ZSF_ZX8081_CONF
Internal configuration and state of ZX80/81 machine
Byte fields:
0: Ram assigned to ZX80/81 (1..16), not counting ram packs on 2000H, 8000H or C000H 
1: Flags1: Bits:

*7-5: Reserved for future use
*4: if 16k RAM block in C000H 
*3: if 16k RAM block in 8000H 
*2: if 8k RAM block in 2000H 
*1: if hsync generator is active 
*0: if nmi generator is active

2: Flags2: Reserved for future use


-Como codificar bloques de memoria para Spectrum 128k, zxuno, tbblue, tsconf, etc?
Con un numero de bloque (0...255) pero... que tamaño de bloque? tbblue usa paginas de 8kb, tsconf usa paginas de 16kb
Quizá numero de bloque y parametro que diga tamaño, para tener un block id comun para todos ellos

*/

//Maxima longitud de los bloques de descripcion
#define MAX_ZSF_BLOCK_ID_NAMELENGTH 30

//Total de nombres sin contar el unknown final
#define MAX_ZSF_BLOCK_ID_NAMES 12
char *zsf_block_id_names[]={
 //123456789012345678901234567890
  "ZSF_NOOP",
  "ZSF_MACHINEID",
  "ZSF_Z80_REGS",
  "ZSF_MOTO_REGS",
  "ZSF_RAMBLOCK",
  "ZSF_SPEC128_MEMCONF",
  "ZSF_SPEC128_RAMBLOCK",
  "ZSF_AYCHIP",
  "ZSF_ULA",
  "ZSF_ULAPLUS",
  "ZSF_ZXUNO_RAMBLOCK",
  "ZSF_ZXUNO_CONF",
  "ZSF_ZX8081_CONF",

  "Unknown"  //Este siempre al final
};


char zsf_magic_header[]="ZSF ZEsarUX Snapshot File.";

char *zsf_get_block_id_name(int block_id)
{
  if (block_id>=MAX_ZSF_BLOCK_ID_NAMES) return zsf_block_id_names[MAX_ZSF_BLOCK_ID_NAMES];
  else return zsf_block_id_names[block_id];
}

int zsf_write_block(FILE *ptr_zsf_file, z80_byte *source,z80_int block_id, unsigned int lenght)
{
  z80_byte block_header[6];
  block_header[0]=value_16_to_8l(block_id);
  block_header[1]=value_16_to_8h(block_id);

  block_header[2]=(lenght)      & 0xFF;
  block_header[3]=(lenght>>8)   & 0xFF;
  block_header[4]=(lenght>>16)  & 0xFF;
  block_header[5]=(lenght>>24)  & 0xFF;

  //Write header
  fwrite(block_header, 1, 6, ptr_zsf_file);

  //Write data block
  if (lenght) fwrite(source, 1, lenght, ptr_zsf_file);

  return 0;

}

void load_zsf_spec128_memconf(z80_byte *header)
{
/*
-Block ID 5: ZSF_SPEC128_MEMCONF
Byte Fields:
0: Port 32765 contents
1: Port 8189 contents
2: Total memory multiplier: 1 for 128kb ram, 2 for 256 kb ram, 4 for 512 kb ram
*/

	puerto_32765=header[0];
	puerto_8189=header[1];
	mem128_multiplicador=header[2];

//Distinguir entre 128/p2 y p2a
	if (MACHINE_IS_SPECTRUM_128_P2) {
		debug_printf(VERBOSE_DEBUG,"Paging 128k according to port 32765: %02XH",puerto_32765);
		mem_page_ram_128k();
		mem_page_rom_128k();
	}

	if (MACHINE_IS_SPECTRUM_P2A_P3) {
		mem_page_ram_p2a();

		if (puerto_8189&1) mem_page_ram_rom();
		else mem_page_rom_p2a();


		//mem_init_memory_tables_p2a();
/*

p2a
32765:
                        //asignar ram
                        mem_page_ram_p2a();

                        //asignar rom
                        mem_page_rom_p2a();


8189:

ram in rom: mem_page_ram_rom();
mem_page_rom_p2a();
*/
	}


}

void load_zsf_snapshot_z80_regs(z80_byte *header)
{
  reg_c=header[0];
  reg_b=header[1];
  reg_e=header[2];
  reg_d=header[3];
  reg_l=header[4];
  reg_h=header[5];

        store_flags(header[6]);
        reg_a=header[7];

        reg_ix=value_8_to_16(header[9],header[8]);
        reg_iy=value_8_to_16(header[11],header[10]);

        reg_c_shadow=header[12];
        reg_b_shadow=header[13];
        reg_e_shadow=header[14];
        reg_d_shadow=header[15];
        reg_l_shadow=header[16];
        reg_h_shadow=header[17];

        store_flags_shadow(header[18]);
        reg_a_shadow=header[19];

        reg_r=header[20];
        reg_r_bit7=reg_r&128;

        reg_i=header[21];

        reg_sp=value_8_to_16(header[23],header[22]);

        reg_pc=value_8_to_16(header[25],header[24]);

        im_mode=header[26] & 2;
        if (im_mode==1) im_mode=2;

        iff1.v=iff2.v=header[26] &1;
}


/*
Cargar bloque de datos en destino indicado
block_length: usado en bloques no comprimidos (lo que dice la cabecera que ocupa)
longitud_original: usado en bloques comprimidos (lo que ocupan los bloques comprimidos)
*/
void load_zsf_snapshot_block_data_addr(z80_byte *block_data,z80_byte *destino,int block_lenght, int longitud_original,int si_comprimido)
{
  
  //printf ("load_zsf_snapshot_block_data_addr block_lenght: %d longitud_original: %d si_comprimido: %d\n",block_lenght,longitud_original,si_comprimido);

  if (si_comprimido) {
    //Comprimido
    util_uncompress_data_repetitions(block_data,destino,longitud_original,0xDD);
  }


  else {
    int i=0;
    while (block_lenght) {
      *destino=block_data[i++];
      destino++;
      block_lenght--;
    }
  }
}

void load_zsf_snapshot_block_data(z80_byte *block_data,int longitud_original)
{



  int i=0;
  z80_byte block_flags=block_data[i];

  //longitud_original : tamanyo que ocupa todo el bloque con la cabecera de 5 bytes

  i++;
  z80_int block_start=value_8_to_16(block_data[i+1],block_data[i]);
  i +=2;
  int block_lenght=value_8_to_16(block_data[i+1],block_data[i]);
  i+=2;

  if (block_lenght==0) block_lenght=65536;

  debug_printf (VERBOSE_DEBUG,"Block start: %d Lenght: %d Compressed: %s Length_source: %d",block_start,block_lenght,(block_flags&1 ? "Yes" : "No"),longitud_original);
  //printf ("Block start: %d Lenght: %d Compressed: %d Length_source: %d\n",block_start,block_lenght,block_flags&1,longitud_original);


  longitud_original -=5;


  load_zsf_snapshot_block_data_addr(&block_data[i],&memoria_spectrum[block_start],block_lenght,longitud_original,block_flags&1);

}


void load_zsf_spec128_snapshot_block_data(z80_byte *block_data,int longitud_original)
{



  int i=0;
  z80_byte block_flags=block_data[i];

  //longitud_original : tamanyo que ocupa todo el bloque con la cabecera de 5 bytes

  i++;
  z80_int block_start=value_8_to_16(block_data[i+1],block_data[i]);
  i +=2;
  z80_int block_lenght=value_8_to_16(block_data[i+1],block_data[i]);
  i+=2;

  z80_byte ram_page=block_data[i];
  i++;

  debug_printf (VERBOSE_DEBUG,"Block ram_page: %d start: %d Lenght: %d Compressed: %s Length_source: %d",ram_page,block_start,block_lenght,(block_flags&1 ? "Yes" : "No"),longitud_original);


  longitud_original -=6;


  load_zsf_snapshot_block_data_addr(&block_data[i],ram_mem_table[ram_page],block_lenght,longitud_original,block_flags&1);

}

void load_zsf_zxuno_snapshot_block_data(z80_byte *block_data,int longitud_original)
{



  int i=0;
  z80_byte block_flags=block_data[i];

  //longitud_original : tamanyo que ocupa todo el bloque con la cabecera de 5 bytes

  i++;
  z80_int block_start=value_8_to_16(block_data[i+1],block_data[i]);
  i +=2;
  z80_int block_lenght=value_8_to_16(block_data[i+1],block_data[i]);
  i+=2;

  z80_byte ram_page=block_data[i];
  i++;

  debug_printf (VERBOSE_DEBUG,"Block ram_page: %d start: %d Lenght: %d Compressed: %s Length_source: %d",ram_page,block_start,block_lenght,(block_flags&1 ? "Yes" : "No"),longitud_original);


  longitud_original -=6;


  load_zsf_snapshot_block_data_addr(&block_data[i],zxuno_sram_mem_table_new[ram_page],block_lenght,longitud_original,block_flags&1);

}


void load_zsf_aychip(z80_byte *header)
{

  
  ay_chip_present.v=1;

  z80_byte header_aychip_number=header[0];
  z80_byte header_aychip_selected=header[1];

  //MAX_AY_CHIPS
  debug_printf(VERBOSE_DEBUG,"Loading AY Chip number %d contents",header_aychip_number);

  if (header_aychip_number>MAX_AY_CHIPS-1 || header_aychip_selected>MAX_AY_CHIPS-1) {
    debug_printf(VERBOSE_ERR,"Snapshot uses more ay chips than we have (%d), ignoring this ZSF_AYCHIP block",MAX_AY_CHIPS);
    return;
  }

  ay_chip_selected=header_aychip_selected;

  //Si el numero de chip a cargar (0..) es mayor que el numero actual de chips (-1)
  if (header_aychip_number>total_ay_chips-1) {
    total_ay_chips=header_aychip_number+1;
    debug_printf(VERBOSE_DEBUG,"Increasing total ay chips to %d",total_ay_chips);
  }

  ay_3_8912_registro_sel[header_aychip_number]=header[2];


      int j;
      for (j=0;j<16;j++) ay_3_8912_registros[header_aychip_number][j]=header[3+j];
  

/*
      
-Block ID 7: ZSF_AYCHIP
Byte fields:
0: AY Chip number, starting at 0. A normal spectrum will be only the 0. A turbosound, 0 and 1, etc
1: Current AY Chip selected (variable ay_chip_selected). Redundant in all ZSF_AYCHIP blocks
2: AY Last Register selection
3-18: AY Chip contents
      */
  /*

*/

}

void load_zsf_ula(z80_byte *header)
{
  out_254=header[0] & 7;
  out_254_original_value=out_254;

  //printf ("border: %d\n",out_254);
  modificado_border.v=1;
}


void load_zsf_ulaplus(z80_byte *header)
{
/*
-Block ID 9: ZSF_ULAPLUS
Byte fields:
0: ULAplus mode
1: ULAplus last out to port BF3B
2: ULAplus last out to port FF3B
3-66: ULAplus palette
*/

	ulaplus_presente.v=1;
                        
	ulaplus_mode=header[0];
	if (ulaplus_mode) ulaplus_enabled.v=1;
	else ulaplus_enabled.v=0;

	debug_printf (VERBOSE_DEBUG,"Setting ULAplus mode %d",ulaplus_mode);


        ulaplus_last_send_BF3B=header[1];
        ulaplus_last_send_FF3B=header[2];


        //Leer 64 bytes de paleta ulaplus
        int i;
        for (i=0;i<64;i++) ulaplus_palette_table[i]=header[3+i];
}

void load_zsf_zx8081_conf(z80_byte *header)
{
/*
0: Ram assigned to ZX80/81 (1..16), not counting ram packs on 2000H, 8000H or C000H 
1: Flags1: Bits:

*7-5: Reserved for future use
*4: if 16k RAM block in C000H 
*3: if 16k RAM block in 8000H 
*2: if 8k RAM block in 2000H 
*1: if hsync generator is active 
*0: if nmi generator is active

2: Flags2: Reserved for future use

*/

  z80_int zx8081ram=header[0];
  set_zx8081_ramtop(zx8081ram);

  ram_in_49152.v=(header[1]>>4)&1;
  ram_in_32768.v=(header[1]>>3)&1;
  ram_in_8192.v=(header[1]>>2)&1;
  hsync_generator_active.v=(header[1]>>1)&1;
  nmi_generator_active.v=header[1]&1;


}


void load_zsf_zxuno_conf(z80_byte *header)
{
/*
-Block ID 11: ZSF_ZXUNO_CONF
Ports and internal registers of ZXUNO machine
Byte fields:
0: Last out to port FC3B
1-256: 256 internal ZXUNO registers
257: Flash SPI bus index
258: Flash SPI next read byte   
259: Flash SPI status register
260-262: 24 bit value with last spi write address
263-265: 24 bit value with last spi read address
266-273: 8 byte with spi bus contents
*/

  last_port_FC3B=header[0];
  int i;
  for (i=0;i<256;i++) zxuno_ports[i]=header[1+i];

  zxuno_spi_bus_index=header[257];
  next_spi_read_byte=header[258];
  zxuno_spi_status_register=header[259];


  last_spi_write_address=(header[260]) + (256 * header[261]) + (65536 * header[262]);
  last_spi_read_address=(header[263]) + (256 * header[264]) + (65536 * header[265]);

  for (i=0;i<8;i++) zxuno_spi_bus[i]=header[266+i];

  zxuno_set_memory_pages();    

  //Resetear settings mmc. Ya los habilitara luego si conviene
  divmmc_diviface_enabled.v=0;
  diviface_enabled.v=0;


  //Sincronizar settings de emulador con los valores de puertos de zxuno
  zxuno_set_emulador_settings();



  ulaplus_set_extended_mode(zxuno_ports[0x40]);
}

void load_zsf_snapshot(char *filename)
{

  FILE *ptr_zsf_file;

  ptr_zsf_file=fopen(filename,"rb");
  if (!ptr_zsf_file) {
          debug_printf (VERBOSE_ERR,"Error reading snapshot file %s",filename);
          return;
  }


  //Verificar que la cabecera inicial coincide
  //zsf_magic_header

  char buffer_magic_header[256];

  int longitud_magic=strlen(zsf_magic_header);


  int leidos=fread(buffer_magic_header,1,longitud_magic,ptr_zsf_file);

  if (leidos!=longitud_magic) {
    debug_printf(VERBOSE_ERR,"Invalid ZSF file, small magic header");
    fclose(ptr_zsf_file);
    return;
  }

  //Comparar texto
  buffer_magic_header[longitud_magic]=0;

  if (strcmp(buffer_magic_header,zsf_magic_header)) {
    debug_printf(VERBOSE_ERR,"Invalid ZSF file, invalid magic header");
    fclose(ptr_zsf_file);
    return;
  }


  z80_byte block_header[6];

  //Read blocks
  while (!feof(ptr_zsf_file)) {
    //Read header block
    unsigned int leidos=fread(block_header,1,6,ptr_zsf_file);
    if (leidos==0) break; //End while

    if (leidos!=6) {
      debug_printf(VERBOSE_ERR,"Error reading snapshot file. Read: %u Expected: 6",leidos);
      return;
    }

    z80_int block_id;
    block_id=value_8_to_16(block_header[1],block_header[0]);
    unsigned int block_lenght=block_header[2]+(block_header[3]*256)+(block_header[4]*65536)+(block_header[5]*16777216);

    debug_printf (VERBOSE_INFO,"Block id: %u (%s) Lenght: %u",block_id,zsf_get_block_id_name(block_id),block_lenght);

    z80_byte *block_data;

    //Evitar bloques de longitud cero
    //Por si acaso inicializar a algo
    z80_byte buffer_nothing;
    block_data=&buffer_nothing;

    if (block_lenght) {
      block_data=malloc(block_lenght);

      if (block_data==NULL) {
        debug_printf(VERBOSE_ERR,"Error allocation memory reading ZSF file");
        return;
      }

      //Read block data
      leidos=fread(block_data,1,block_lenght,ptr_zsf_file);
      if (leidos!=block_lenght) {
        debug_printf(VERBOSE_ERR,"Error reading snapshot file. Read: %u Expected: %u",leidos,block_lenght);
        return;
      }
    }

    //switch for every possible block id
    switch(block_id)
    {

      case ZSF_NOOP_ID:
        //no hacer nada
      break;

      case ZSF_MACHINEID:
        current_machine_type=*block_data;
        set_machine(NULL);
        reset_cpu();
      break;

      case ZSF_Z80_REGS_ID:
        load_zsf_snapshot_z80_regs(block_data);
      break;

      case ZSF_RAMBLOCK:
        load_zsf_snapshot_block_data(block_data,block_lenght);
      break;

      case ZSF_SPEC128_MEMCONF:
        load_zsf_spec128_memconf(block_data);
      break;

      case ZSF_SPEC128_RAMBLOCK:
        load_zsf_spec128_snapshot_block_data(block_data,block_lenght);
      break;


      case ZSF_AYCHIP:
        load_zsf_aychip(block_data);
      break;

      case ZSF_ULA:
        load_zsf_ula(block_data);
      break;

      case ZSF_ULAPLUS:
        load_zsf_ulaplus(block_data);
      break;

      case ZSF_ZXUNO_RAMBLOCK:
        load_zsf_zxuno_snapshot_block_data(block_data,block_lenght);
      break;

      case ZSF_ZXUNO_CONF:
        load_zsf_zxuno_conf(block_data);
      break;

      case ZSF_ZX8081_CONF:
        load_zsf_zx8081_conf(block_data);
      break;      

      default:
        debug_printf(VERBOSE_ERR,"Unknown ZSF Block ID: %u. Continue anyway",block_id);
      break;

    }


    if (block_lenght) free(block_data);

  }

  fclose(ptr_zsf_file);


}

void save_zsf_snapshot_cpuregs(FILE *ptr)
{

  z80_byte header[27];

  header[0]=reg_c;
  header[1]=reg_b;
  header[2]=reg_e;
  header[3]=reg_d;
  header[4]=reg_l;
  header[5]=reg_h;

  header[6]=get_flags();
  header[7]=reg_a;



  header[8]=value_16_to_8l(reg_ix);
  header[9]=value_16_to_8h(reg_ix);
  header[10]=value_16_to_8l(reg_iy);
  header[11]=value_16_to_8h(reg_iy);

  header[12]=reg_c_shadow;
  header[13]=reg_b_shadow;
  header[14]=reg_e_shadow;
  header[15]=reg_d_shadow;
  header[16]=reg_l_shadow;
  header[17]=reg_h_shadow;



  header[18]=get_flags_shadow();
  header[19]=reg_a_shadow;

  header[20]=(reg_r&127) | (reg_r_bit7&128);

  header[21]=reg_i;

  header[22]=value_16_to_8l(reg_sp);
  header[23]=value_16_to_8h(reg_sp);
  header[24]=value_16_to_8l(reg_pc);
  header[25]=value_16_to_8h(reg_pc);


  z80_byte bits_estado=(iff1.v) | (im_mode==2 ? 2 : 0);
  header[26]=bits_estado;

  zsf_write_block(ptr, header,ZSF_Z80_REGS_ID, 27);

}

//Guarda en destino el bloque de memoria comprimido, siempre que salga a cuenta comprimirlo. 
//Si ocupa mas que el original, lo guardara comprimido
//Retorna longitud en valor retorno, flag indicando si esta comprimido o no
//Tener en cuenta que el destino sea al menos de tamanyo el doble que origen
int save_zsf_copyblock_compress_uncompres(z80_byte *origen,z80_byte *destino,int tamanyo_orig,int *si_comprimido)
{

  int longitud_comprimido=util_compress_data_repetitions(origen,destino,tamanyo_orig,0xDD);

  if (zsf_force_uncompressed || longitud_comprimido>tamanyo_orig) {
    *si_comprimido=0;
    memcpy(destino,origen,tamanyo_orig);
    return tamanyo_orig;
  }

  else {
    *si_comprimido=1;
    return longitud_comprimido;
  }
}

void save_zsf_snapshot(char *filename)
{

  FILE *ptr_zsf_file;

  //ZSF File
  ptr_zsf_file=fopen(filename,"wb");
  if (!ptr_zsf_file) {
          debug_printf (VERBOSE_ERR,"Error writing snapshot file %s",filename);
          return;
  }


  //Save header
  fwrite(zsf_magic_header, 1, strlen(zsf_magic_header), ptr_zsf_file);
  


  //First save machine ID
  z80_byte save_machine_id=current_machine_type;
  zsf_write_block(ptr_zsf_file, &save_machine_id,ZSF_MACHINEID, 1);



  //Save cpu registers. Z80 or Moto or MK14
  if (CPU_IS_MOTOROLA) {

  }
  else if (CPU_IS_SCMP) {
  }

  else {
    save_zsf_snapshot_cpuregs(ptr_zsf_file);
  }



 //Ula block
  z80_byte ulablock[1];

  ulablock[0]=out_254 & 7;

  zsf_write_block(ptr_zsf_file, ulablock,ZSF_ULA, 1);

  //ULAPlus block. Mejor que este esto hacia el principio, asi si por ejemplo se carga zxuno y esta un modo ulaplus extendido,
  //como radastan, no hay problema en que el modo ulaplus estandard esté desactivado
  if (ulaplus_presente.v) {
   z80_byte ulaplusblock[67];
/*
-Block ID 9: ZSF_ULAPLUS
Byte fields:
0: ULAplus mode
1: ULAplus last out to port BF3B
2: ULAplus last out to port FF3B
3-66: ULAplus palette
*/
  ulaplusblock[0]=ulaplus_mode;
  ulaplusblock[1]=ulaplus_last_send_BF3B;
  ulaplusblock[2]=ulaplus_last_send_FF3B;

        int i;
        for (i=0;i<64;i++) ulaplusblock[3+i]=ulaplus_palette_table[i];

     zsf_write_block(ptr_zsf_file, ulaplusblock,ZSF_ULAPLUS, 67);
  }

  //Algunos flags zx80/81
  if (MACHINE_IS_ZX8081) {
    z80_byte zx8081confblock[3];

    z80_byte ram_zx8081=(ramtop_zx8081-16383)/1024;
    zx8081confblock[0]=ram_zx8081;

    z80_byte flags1;
    z80_byte flags2=0;

    flags1=(ram_in_49152.v<<4)|(ram_in_32768.v<<3)|(ram_in_8192.v<<2)|(hsync_generator_active.v<<1)|nmi_generator_active.v;

    zx8081confblock[1]=flags1;
    zx8081confblock[2]=flags2;

    zsf_write_block(ptr_zsf_file, zx8081confblock,ZSF_ZX8081_CONF, 3);


  }


  //Maquinas Spectrum de 48kb y zx80/81
  if (MACHINE_IS_SPECTRUM_16_48 || MACHINE_IS_ZX8081) {

	  int inicio_ram=16384;
	  int longitud_ram=49152;
	  if (MACHINE_IS_SPECTRUM_16) longitud_ram=16384;

	  if (MACHINE_IS_INVES) {
  		//Grabar tambien la ram oculta de inves (0-16383). Por tanto, grabar todos los 64kb de ram
	  	longitud_ram=65536; //65536
		  inicio_ram=0;
	  }

    if (MACHINE_IS_ZX8081) {
      int final_ram=get_ramtop_with_rampacks()+1;
      if (ram_in_8192.v) inicio_ram=8192;
      longitud_ram=final_ram-inicio_ram;
    }

    //Test. Save 48kb block
    //Allocate 5+48kb bytes
    /*z80_byte *ramblock=malloc(longitud_ram+5);
    if (ramblock==NULL) {
      debug_printf (VERBOSE_ERR,"Error allocating memory");
      return;
    }*/


    //Para el bloque comprimido
    z80_byte *compressed_ramblock=malloc(longitud_ram*2);
    if (compressed_ramblock==NULL) {
      debug_printf (VERBOSE_ERR,"Error allocating memory");
      return;
    } 

    /*
    0: Flags. Currently: bit 0: if compressed
    1,2: Block start
    3,4: Block lenght
    5 and next bytes: data bytes
    */

    compressed_ramblock[0]=0;
    compressed_ramblock[1]=value_16_to_8l(inicio_ram);
    compressed_ramblock[2]=value_16_to_8h(inicio_ram);
    compressed_ramblock[3]=value_16_to_8l(longitud_ram);
    compressed_ramblock[4]=value_16_to_8h(longitud_ram);

    int si_comprimido;
    int longitud_bloque=save_zsf_copyblock_compress_uncompres(&memoria_spectrum[inicio_ram],&compressed_ramblock[5],longitud_ram,&si_comprimido);
    if (si_comprimido) compressed_ramblock[0]|=1;

    //Store block to file
    zsf_write_block(ptr_zsf_file, compressed_ramblock,ZSF_RAMBLOCK, longitud_bloque+5);

    free(compressed_ramblock);

  }

/*
  Maquinas Spectrum de 128kb (128, p2, p2a)
  Permitir mas ram segun parametro 
  int mem128_multiplicador;
  Si hay mas de 128kb para maquinas tipo 128k y +2a
  Si vale 1, solo 128k
  Si vale 2, hay 256 kb
  Si vale 4, hay 512 kb
  Otros parametros implicados aqui
  z80_byte *ram_mem_table[32];

  z80_byte puerto_32765;
  z80_byte puerto_8189;

  Hay que crear trama nueva para guardar esos puertos de paginacion, y el valor del multiplicador



*/
  if (MACHINE_IS_SPECTRUM_128_P2_P2A_P3 || MACHINE_IS_ZXUNO || MACHINE_IS_CHLOE || MACHINE_IS_PRISM || MACHINE_IS_TBBLUE || MACHINE_IS_PENTAGON || MACHINE_IS_CHROME || MACHINE_IS_ZXEVO) {
/*
-Block ID 5: ZSF_SPEC128_MEMCONF
Byte Fields:
0: Port 32765 contents
1: Port 8189 contents
2: Total memory multiplier: 1 for 128kb ram, 2 for 256 kb ram, 4 for 512 kb ram
*/
	z80_byte memconf[3];
	memconf[0]=puerto_32765;
	memconf[1]=puerto_8189;
	memconf[2]=mem128_multiplicador;

  	zsf_write_block(ptr_zsf_file, memconf,ZSF_SPEC128_MEMCONF, 3);

}

if (MACHINE_IS_SPECTRUM_128_P2_P2A_P3) {

   int longitud_ram=16384;

  //Allocate 6+48kb bytes
  /*z80_byte *ramblock=malloc(longitud_ram+6);
  if (ramblock==NULL) {
    debug_printf (VERBOSE_ERR,"Error allocating memory");
    return;
  }*/


  //Para el bloque comprimido
   z80_byte *compressed_ramblock=malloc(longitud_ram*2);
  if (compressed_ramblock==NULL) {
    debug_printf (VERBOSE_ERR,"Error allocating memory");
    return;
  }

  /*

-Block ID 6: ZSF_SPEC128_RAMBLOCK
A ram binary block for a spectrum 128, p2 or p2a machine
Byte Fields:
0: Flags. Currently: bit 0: if compressed with repetition block DD DD YY ZZ, where
    YY is the byte to repeat and ZZ the number of repetitions (0 means 256)
1,2: Block start address
3,4: Block lenght
5: ram block id (0..7) for a spectrum 128k for example
6 and next bytes: data bytes
  */

  int paginas=8*mem128_multiplicador;
  z80_byte ram_page;

  for (ram_page=0;ram_page<paginas;ram_page++) {

	  compressed_ramblock[0]=0;
	  compressed_ramblock[1]=value_16_to_8l(16384);
	  compressed_ramblock[2]=value_16_to_8h(16384);
	  compressed_ramblock[3]=value_16_to_8l(longitud_ram);
	  compressed_ramblock[4]=value_16_to_8h(longitud_ram);
	  compressed_ramblock[5]=ram_page;

	  int si_comprimido;
	  int longitud_bloque=save_zsf_copyblock_compress_uncompres(ram_mem_table[ram_page],&compressed_ramblock[6],longitud_ram,&si_comprimido);
	  if (si_comprimido) compressed_ramblock[0]|=1;

	  debug_printf(VERBOSE_DEBUG,"Saving ZSF_SPEC128_RAMBLOCK ram page: %d length: %d",ram_page,longitud_bloque);

	  //Store block to file
	  zsf_write_block(ptr_zsf_file, compressed_ramblock,ZSF_SPEC128_RAMBLOCK, longitud_bloque+6);

  }

  free(compressed_ramblock);


  }

if (MACHINE_IS_ZXUNO) {

    z80_byte zxunoconfblock[274];

/*
-Block ID 11: ZSF_ZXUNO_CONF
Ports and internal registers of ZXUNO machine
Byte fields:
0: Last out to port FC3B
1-256: 256 internal ZXUNO registers
257: Flash SPI bus index
258: Flash SPI next read byte   
259: Flash SPI status register
260-262: 24 bit value with last spi write address
263-265: 24 bit value with last spi read address
266-273: 8 byte with spi bus contents
*/    

    zxunoconfblock[0]=last_port_FC3B;
    int i;
    for (i=0;i<256;i++) zxunoconfblock[1+i]=zxuno_ports[i];

    zxunoconfblock[257]=zxuno_spi_bus_index;
    zxunoconfblock[258]=next_spi_read_byte;  
    zxunoconfblock[259]=zxuno_spi_status_register;  


    zxunoconfblock[260]=last_spi_write_address & 0xFF;
    zxunoconfblock[261]=(last_spi_write_address>>8) & 0xFF;
    zxunoconfblock[262]=(last_spi_write_address>>16) & 0xFF;

    zxunoconfblock[263]=last_spi_read_address & 0xFF;
    zxunoconfblock[264]=(last_spi_read_address>>8) & 0xFF;
    zxunoconfblock[265]=(last_spi_read_address>>16) & 0xFF;

    for (i=0;i<8;i++) zxunoconfblock[266+i]=zxuno_spi_bus[i];

    zsf_write_block(ptr_zsf_file, zxunoconfblock,ZSF_ZXUNO_CONF, 274);



   int longitud_ram=16384;

  
   //Para el bloque comprimido
   z80_byte *compressed_ramblock=malloc(longitud_ram*2);
  if (compressed_ramblock==NULL) {
    debug_printf (VERBOSE_ERR,"Error allocating memory");
    return;
  }

  /*

-Block ID 10: ZSF_ZXUNO_RAMBLOCK
A ram binary block for a zxuno
Byte Fields:
0: Flags. Currently: bit 0: if compressed with repetition block DD DD YY ZZ, where
    YY is the byte to repeat and ZZ the number of repetitions (0 means 256)
1,2: Block start address (currently unused)
3,4: Block lenght
5: ram block id 
6 and next bytes: data bytes
  */

  int paginas=ZXUNO_SRAM_PAGES;
  z80_byte ram_page;

  for (ram_page=0;ram_page<paginas;ram_page++) {

    compressed_ramblock[0]=0;
    compressed_ramblock[1]=value_16_to_8l(16384);
    compressed_ramblock[2]=value_16_to_8h(16384);
    compressed_ramblock[3]=value_16_to_8l(longitud_ram);
    compressed_ramblock[4]=value_16_to_8h(longitud_ram);
    compressed_ramblock[5]=ram_page;

    int si_comprimido;
    int longitud_bloque=save_zsf_copyblock_compress_uncompres(zxuno_sram_mem_table_new[ram_page],&compressed_ramblock[6],longitud_ram,&si_comprimido);
    if (si_comprimido) compressed_ramblock[0]|=1;

    debug_printf(VERBOSE_DEBUG,"Saving ZSF_ZXUNO_RAMBLOCK ram page: %d length: %d",ram_page,longitud_bloque);

    //Store block to file
    zsf_write_block(ptr_zsf_file, compressed_ramblock,ZSF_ZXUNO_RAMBLOCK, longitud_bloque+6);

  }

  free(compressed_ramblock);


  }


  //Registros chip AY
  if (ay_chip_present.v) {
    int i;
    for (i=0;i<total_ay_chips;i++) {
      z80_byte aycontents[19];

      /*

-Block ID 7: ZSF_AYCHIP
Byte fields:
0: AY Chip number, starting at 0. A normal spectrum will be only the 0. A turbosound, 0 and 1, etc
1: Current AY Chip selected (variable ay_chip_selected). Redundant in all ZSF_AYCHIP blocks
2: AY Last Register selection
3-18: AY Chip contents
      */
      aycontents[0]=i;
      aycontents[1]=ay_chip_selected;
      aycontents[2]=ay_3_8912_registro_sel[i];

      int j;
      for (j=0;j<16;j++) aycontents[3+j]=ay_3_8912_registros[i][j];

      zsf_write_block(ptr_zsf_file, aycontents,ZSF_AYCHIP, 19);
    }
  }

 


  //test meter un NOOP
  zsf_write_block(ptr_zsf_file, NULL,0, 0);


  fclose(ptr_zsf_file);

}
