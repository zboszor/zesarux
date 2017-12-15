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

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>


#include "cpu.h"
#include "debug.h"
#include "tape.h"
#include "audio.h"
#include "screen.h"
#include "ay38912.h"
#include "operaciones.h"
#include "snap.h"
#include "timer.h"
#include "menu.h"
#include "compileoptions.h"
#include "contend.h"
#include "utils.h"
#include "realjoystick.h"
#include "chardetect.h"
#include "m68k.h"
#include "ula.h"




z80_byte byte_leido_core_ql;
char buffer_disassemble[100000];

extern unsigned char ql_pc_intr;

int refresca=0;


//bucle principal de ejecucion de la cpu de jupiter ace
void cpu_core_loop_ql(void)
{

                debug_get_t_stados_parcial_post();
                debug_get_t_stados_parcial_pre();


		timer_check_interrupt();

//#ifdef COMPILE_STDOUT
//              if (screen_stdout_driver) scr_stdout_printchar();
//#endif
//
//#ifdef COMPILE_SIMPLETEXT
//                if (screen_simpletext_driver) scr_simpletext_printchar();
//#endif
                if (chardetect_detect_char_enabled.v) chardetect_detect_char();
                if (chardetect_printchar_enabled.v) chardetect_printchar();




                ql_rom_traps();







		if (0==1) { }




		else {
			if (esperando_tiempo_final_t_estados.v==0) {



#ifdef EMULATE_CPU_STATS
                                util_stats_increment_counter(stats_codsinpr,byte_leido_core_ql);
#endif



        /*char buffer_disassemble[1000];
        unsigned int pc_ql=100;

        // Access the internals of the CPU
        pc_ql=m68k_get_reg(NULL, M68K_REG_PC);

        int longitud=m68k_disassemble(buffer_disassemble, pc_ql, M68K_CPU_TYPE_68000);
        printf ("%08XH %d %s\n",pc_ql,longitud,buffer_disassemble);
        */


	//intento de detectar cuando se llama a rutina rom de mt.ipcom
	//unsigned int pc_ql=get_ql_pc();
	//if (pc_ql==0x0872) exit(1);

	//Ejecutar opcode
#ifdef EMULATE_VISUALMEM
        set_visualmemopcodebuffer(get_pc_register() );
#endif

                // Values to execute determine the interleave rate.
                // Smaller values allow for more accurate interleaving with multiple
                // devices/CPUs but is more processor intensive.
                // 100000 is usually a good value to start at, then work from there.

                // Note that I am not emulating the correct clock speed!
                m68k_execute(1);


		/* Disasemble one instruction at pc and store in str_buff */
//unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type)
//unsigned int m68k_get_reg(void* context, m68k_register_t regnum)
//                case M68K_REG_PC:       return MASK_OUT_ABOVE_32(cpu->pc);
//		unsigned int registropcql=m68k_get_reg(NULL,M68K_REG_PC);
//		printf ("Registro PC: %08XH\n",registropcql);
//		m68k_disassemble(buffer_disassemble,registropcql,M68K_CPU_TYPE_68000);


				//Simplemente incrementamos los t-estados un valor inventado, aunque luego al final parece ser parecido a la realidad
				t_estados+=4;


                        }
                }



		//Esto representa final de scanline

		//normalmente
		if ( (t_estados/screen_testados_linea)>t_scanline  ) {

			t_scanline++;



                        //Envio sonido

                        audio_valor_enviar_sonido=0;

                        audio_valor_enviar_sonido +=da_output_ay();




                        if (realtape_inserted.v && realtape_playing.v) {
                                realtape_get_byte();
                                //audio_valor_enviar_sonido += realtape_last_value;
				if (realtape_loading_sound.v) {
				audio_valor_enviar_sonido /=2;
                                audio_valor_enviar_sonido += realtape_last_value/2;
				}
                        }

                        //Ajustar volumen
                        if (audiovolume!=100) {
                                audio_valor_enviar_sonido=audio_adjust_volume(audio_valor_enviar_sonido);
                        }


			//printf ("sonido: %d\n",audio_valor_enviar_sonido);

                        audio_buffer[audio_buffer_indice]=audio_valor_enviar_sonido;


                        if (audio_buffer_indice<AUDIO_BUFFER_SIZE-1) audio_buffer_indice++;
                        //else printf ("Overflow audio buffer: %d \n",audio_buffer_indice);


                        ay_chip_siguiente_ciclo();

			//se supone que hemos ejecutado todas las instrucciones posibles de toda la pantalla. refrescar pantalla y
			//esperar para ver si se ha generado una interrupcion 1/50

			//Final de frame

			if (t_estados>=screen_testados_total) {

				t_scanline=0;

                                //Parche para maquinas que no generan 312 lineas, porque si enviamos menos sonido se escuchara un click al final
                                //Es necesario que cada frame de pantalla contenga 312 bytes de sonido
				//Igualmente en la rutina de envio_audio se vuelve a comprobar que todo el sonido a enviar
				//este completo; esto es necesario para Z88

                                int linea_estados=t_estados/screen_testados_linea;

                                while (linea_estados<312) {

                                        audio_buffer[audio_buffer_indice]=audio_valor_enviar_sonido;
                                        if (audio_buffer_indice<AUDIO_BUFFER_SIZE-1) audio_buffer_indice++;
                                        linea_estados++;
                                }


				t_estados -=screen_testados_total;

				cpu_loop_refresca_pantalla();


				vofile_send_frame(rainbow_buffer);

				siguiente_frame_pantalla();

        contador_parpadeo--;
            //printf ("Parpadeo: %d estado: %d\n",contador_parpadeo,estado_parpadeo.v);
            if (!contador_parpadeo) {
                    contador_parpadeo=20; //TODO no se si esta es la frecuencia normal de parpadeo
                    estado_parpadeo.v ^=1;
            }


				if (debug_registers) scr_debug_registers();


				if (!interrupcion_timer_generada.v) {
					//Llegado a final de frame pero aun no ha llegado interrupcion de timer. Esperemos...
					esperando_tiempo_final_t_estados.v=1;
				}

				else {
					//Llegado a final de frame y ya ha llegado interrupcion de timer. No esperamos.... Hemos tardado demasiado
					//printf ("demasiado\n");
					esperando_tiempo_final_t_estados.v=0;
				}


			//Prueba generar interrupcion
			//printf ("Generar interrupcion\n");
			 //m68k_set_irq(0); de 0 a 6 parece no tener efecto. poniendo 7 no pasa el test de ram
			/*m68k_set_irq(1);
			m68k_set_irq(2);
			m68k_set_irq(3);
			m68k_set_irq(4);
			m68k_set_irq(5);
			m68k_set_irq(6);*/


/*
* read addresses
pc_intr equ     $18021  bits 4..0 set as pending level 2 interrupts
*/

      //Sirve para algo esto????
			ql_pc_intr |=31;
			m68k_set_irq(2);
			//Esto acaba generando llamadas a leer PC_INTR		Interrupt register


                                //Final de instrucciones ejecutadas en un frame de pantalla
                                if (iff1.v==1) {
                                        interrupcion_maskable_generada.v=1;
                                }



			}



		}

		if (esperando_tiempo_final_t_estados.v) {
			timer_pause_waiting_end_frame();
		}


              //Interrupcion de 1/50s. mapa teclas activas y joystick
                if (interrupcion_fifty_generada.v) {
                        interrupcion_fifty_generada.v=0;

                        //y de momento actualizamos tablas de teclado segun tecla leida
                        //printf ("Actualizamos tablas teclado %d ", temp_veces_actualiza_teclas++);
                       scr_actualiza_tablas_teclado();


                       //lectura de joystick
                       realjoystick_main();

                        //printf ("temp conta fifty: %d\n",tempcontafifty++);
                }


                //Interrupcion de procesador y marca final de frame
                if (interrupcion_timer_generada.v) {
                        interrupcion_timer_generada.v=0;
                        esperando_tiempo_final_t_estados.v=0;
                        interlaced_numero_frame++;
                        //printf ("%d\n",interlaced_numero_frame);
                }



		//Interrupcion de cpu. gestion im0/1/2.
		if (interrupcion_maskable_generada.v || interrupcion_non_maskable_generada.v) {



                        //ver si esta en HALT
                        if (z80_ejecutando_halt.v) {
                                        z80_ejecutando_halt.v=0;
                                        reg_pc++;

                        }

			if (1==1) {

					if (interrupcion_non_maskable_generada.v) {
						debug_anota_retorno_step_nmi();
						interrupcion_non_maskable_generada.v=0;



					}


					if (1==1) {


					//justo despues de EI no debe generar interrupcion
					//e interrupcion nmi tiene prioridad
						if (interrupcion_maskable_generada.v && byte_leido_core_ql!=251) {
						debug_anota_retorno_step_maskable();
						//Tratar interrupciones maskable




					}
				}


			}

  }


}
