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

#include "cpu.h"
#include "baseconf.h"
#include "mem128.h"
#include "debug.h"
#include "contend.h"
#include "menu.h"
#include "screen.h"
#include "ula.h"
#include "operaciones.h"


//Direcciones donde estan cada pagina de rom. 32 paginas de 16 kb
z80_byte *baseconf_rom_mem_table[32];

//Direcciones donde estan cada pagina de ram, en paginas de 16 kb
z80_byte *baseconf_ram_mem_table[256];


//Direcciones actuales mapeadas, bloques de 16 kb
z80_byte *baseconf_memory_paged[4];


//Numeros de bloques de memoria asignados
z80_byte baseconf_memory_segments[4];

//Tipos de bloques de memoria asignados
//0: rom. otra cosa: ram
z80_byte baseconf_memory_segments_type[4];

z80_byte baseconf_last_port_77;

z80_byte baseconf_shadow_ports;

void baseconf_reset_cpu(void)
{


    //TODO. Que otros puertos de baseconf se ponen a 0 en el reset?




    baseconf_set_memory_pages();
    //baseconf_set_sizes_display();
}

void baseconf_init_memory_tables(void)
{
	debug_printf (VERBOSE_DEBUG,"Initializing BaseConf memory pages");

	z80_byte *puntero;
	puntero=memoria_spectrum;

	int i;
	for (i=0;i<BASECONF_ROM_PAGES;i++) {
		baseconf_rom_mem_table[i]=puntero;
		puntero +=16384;
	}

	for (i=0;i<BASECONF_RAM_PAGES;i++) {
		baseconf_ram_mem_table[i]=puntero;
		puntero +=16384;
	}




}



void baseconf_set_memory_pages(void)
{

        int i=0;

        for (i=0;i<4;i++) {
                z80_byte pagina=baseconf_memory_segments[i];
                z80_byte pagina_es_ram=baseconf_memory_segments_type[i];

                if ((baseconf_shadow_ports&1)==0) {
                        //A8: if 0, then disable the memory manager. In each window processor is installed the last page of ROM. 0 after reset.
                        pagina=255;
                        pagina_es_ram=0;
                }

                //TODO: A9: If 0 then "force" the inclusion of TR-DOS and the shadow ports. 0 after reset.

                if (pagina_es_ram) {
                        baseconf_memory_paged[i]=baseconf_ram_mem_table[pagina];
                        debug_paginas_memoria_mapeadas[i]=pagina;
                }
                else {
                        pagina=pagina & 31;
                        baseconf_memory_paged[i]=baseconf_rom_mem_table[pagina];
                        debug_paginas_memoria_mapeadas[i]=DEBUG_PAGINA_MAP_ES_ROM+pagina;
                }

                printf ("segmento %d pagina %d\n",i,pagina);
        }
	


  //printf ("32765: %02XH rom %d ram1 %d ram2 %d ram3 %d\n",puerto_32765,rom_page,ram_page_40,ram_page_80,ram_page_c0);


}


void baseconf_hard_reset(void)
{

  debug_printf(VERBOSE_DEBUG,"BaseConf Hard reset cpu");

  //Asignar bloques memoria
  baseconf_memory_segments[0]=baseconf_memory_segments[1]=baseconf_memory_segments[2]=baseconf_memory_segments[3]=255;
  baseconf_memory_segments_type[0]=baseconf_memory_segments_type[1]=baseconf_memory_segments_type[2]=baseconf_memory_segments_type[3]=0;

 
  reset_cpu();


	int i;


       //Borrar toda memoria ram
        int d;
        z80_byte *puntero;
        
        for (i=0;i<BASECONF_RAM_PAGES;i++) {
                puntero=baseconf_ram_mem_table[i];
                for (d=0;d<16384;d++,puntero++) {
                        *puntero=0;
                }
        }
baseconf_last_port_77=0;
baseconf_shadow_ports=0;

        baseconf_set_memory_pages();

}

void baseconf_out_port(z80_int puerto,z80_byte valor)
{

        z80_byte puerto_h=puerto>>8;

        if ( (puerto&0x00FF)==0x77 ) {
                baseconf_shadow_ports=puerto_h;
               baseconf_last_port_77=valor; 

               baseconf_set_memory_pages();
        }

        //The memory manager pages.
        if ( (puerto&0x0FFF)==0xFF7 ) {
                z80_byte pagina=valor ^ 255;
                z80_byte es_ram=valor & 64;

                z80_byte segmento=puerto_h>>6;
                if (es_ram==0) pagina=pagina&31;

                baseconf_memory_segments[segmento]=pagina;  
                baseconf_memory_segments_type[segmento]=es_ram;

                //TODO: bit 7 de variable valor         


               baseconf_set_memory_pages();
        }

        if (puerto==0x7ffd) {
                //mapeamos ram y rom , pero sin habilitando memory manager
                //baseconf_shadow_ports |=1;

                //ram
                baseconf_memory_segments[3]=valor%7;
                baseconf_memory_segments_type[3]=1;

                //rom
                baseconf_memory_segments[0] &=254;
                if (valor&16) baseconf_memory_segments[0] |= 1;
                baseconf_memory_segments_type[0]=0;       


                puerto_32765=valor;
        }
}


void screen_baseconf_refresca_pantalla(void)
{

	/*
	//Como spectrum clasico

	//modo clasico. sin rainbow
	if (rainbow_enabled.v==0) {
        screen_baseconf_refresca_border();
        z80_byte modo_video=baseconf_get_video_mode_display();


        //printf ("modo video: %d\n",modo_video );
        if (modo_video==0) scr_baseconf_refresca_pantalla_zxmode_no_rainbow();
        if (modo_video==1) scr_baseconf_refresca_pantalla_16c_256c_no_rainbow(1);
        if (modo_video==2) scr_baseconf_refresca_pantalla_16c_256c_no_rainbow(2);
        if (modo_video==3) screen_baseconf_refresca_text_mode();

	}

	else {
	//modo rainbow - real video
        if (baseconf_si_render_spritetile_rapido.v) baseconf_fast_tilesprite_render();

        screen_baseconf_refresca_rainbow();
	}
*/
}





