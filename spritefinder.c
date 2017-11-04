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


#include "spritefinder.h"
#include "cpu.h"
#include "debug.h"
#include "utils.h"
#include "operaciones.h"
#include "ula.h"


z80_bit spritefinder_enabled={0};





int spritefinder_nested_id_poke_byte;
int spritefinder_nested_id_poke_byte_no_time;
int spritefinder_nested_id_peek_byte;
int spritefinder_nested_id_peek_byte_no_time;





z80_byte spritefinder_poke_byte(z80_int dir,z80_byte valor)
{

	//spritefinder_original_poke_byte(dir,valor);
        //Llamar a anterior
        debug_nested_poke_byte_call_previous(spritefinder_nested_id_poke_byte,dir,valor);


        //Para que no se queje el compilador, aunque este valor de retorno no lo usamos
        return 0;


}

z80_byte spritefinder_poke_byte_no_time(z80_int dir,z80_byte valor)
{
        //spritefinder_original_poke_byte_no_time(dir,valor);
        //Llamar a anterior
        debug_nested_poke_byte_no_time_call_previous(spritefinder_nested_id_poke_byte_no_time,dir,valor);


        //Para que no se queje el compilador, aunque este valor de retorno no lo usamos
        return 0;


}

z80_byte spritefinder_peek_byte(z80_int dir,z80_byte value GCC_UNUSED)
{

	z80_byte valor_leido=debug_nested_peek_byte_call_previous(spritefinder_nested_id_peek_byte,dir);

	return valor_leido;
}

z80_byte spritefinder_peek_byte_no_time(z80_int dir,z80_byte value GCC_UNUSED)
{

	z80_byte valor_leido=debug_nested_peek_byte_no_time_call_previous(spritefinder_nested_id_peek_byte_no_time,dir);


	return valor_leido;
}



//Establecer rutinas propias
void spritefinder_set_peek_poke_functions(void)
{
                debug_printf (VERBOSE_DEBUG,"Setting spritefinder poke / peek functions");

	//Asignar mediante nuevas funciones de core anidados
	spritefinder_nested_id_poke_byte=debug_nested_poke_byte_add(spritefinder_poke_byte,"Spritefinder poke_byte");
	spritefinder_nested_id_poke_byte_no_time=debug_nested_poke_byte_no_time_add(spritefinder_poke_byte_no_time,"Spritefinder poke_byte_no_time");
	spritefinder_nested_id_peek_byte=debug_nested_peek_byte_add(spritefinder_peek_byte,"Spritefinder peek_byte");
	spritefinder_nested_id_peek_byte_no_time=debug_nested_peek_byte_no_time_add(spritefinder_peek_byte_no_time,"Spritefinder peek_byte_no_time");

}

//Restaurar rutinas de spritefinder
void spritefinder_restore_peek_poke_functions(void)
{
                debug_printf (VERBOSE_DEBUG,"Restoring original poke / peek functions before spritefinder");


	debug_nested_poke_byte_del(spritefinder_nested_id_poke_byte);
	debug_nested_poke_byte_no_time_del(spritefinder_nested_id_poke_byte_no_time);
	debug_nested_peek_byte_del(spritefinder_nested_id_peek_byte);
	debug_nested_peek_byte_no_time_del(spritefinder_nested_id_peek_byte_no_time);
}





void spritefinder_enable(void)
{

  if (!MACHINE_IS_SPECTRUM) {
    debug_printf(VERBOSE_INFO,"Can not enable spritefinder on non Spectrum machine");
    return;
  }

	if (spritefinder_enabled.v) {
		debug_printf (VERBOSE_DEBUG,"Already enabled");
		return;
	}


	spritefinder_set_peek_poke_functions();

	spritefinder_enabled.v=1;





}

void spritefinder_disable(void)
{
	if (spritefinder_enabled.v==0) return;

	spritefinder_restore_peek_poke_functions();


	spritefinder_enabled.v=0;
}



