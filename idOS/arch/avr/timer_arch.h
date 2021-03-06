/** \file       timer_arch.h
 *  \brief      En caso que sea un Arduino avr se configura el Timer1 como timer de sistema.
 *      
 *  \note       Esto es porque Timer 1 es de 16 bits y el framework de Arduino no lo utiliza
 *              por defecto (Timer0 para millis(), micros() y delay() y Timer2 para tone()).
 *  \attention  Timer1 es utilizado en la biblioteca Servo y probablemente en otras. 
 *
 *  \author     Bernardo Yaser León Ávila <bernardoyla@gmail.com>
 */
/** 
 * */

#ifndef _TIMER_ARCH_H_
#define _TIMER_ARCH_H_

#include "../arch.h"
#include <avr/interrupt.h>

typedef uint32_t utime_t;
typedef uint32_t mtime_t;

/** \def    F_CPU
 *  \brief  Reloj (CLK) de la CPU
 *  \note   el IDE puede definir está constante en c_cpp_properties.json 
*/
#ifndef F_CPU
#define F_CPU   16000000UL
#endif

/** \def    FM_CPU 
 *  \brief  Frecuencia de la CPU en MHz para facilitar cálculos
*/
#define  FM_CPU (F_CPU / 1000000L)

/** \def    TIMER_CONT
 *  \brief  Registro que lleva el contador del timer en el micro, en este caso, Timer1 (16 bits) 
 */
#define TIMER_TEMP TCNT1

/** \def    TIMER_COMP
 *  \brief  Registro que lleva el contador del timer en el micro, en este caso, Timer1 (16 bits) 
 */
#define TIMER_COMP OCR1A

/** \def    TIMER_PRESCALER 
 *  \brief  Definición del prescalador para el rtimer
 *  \note   A 16 MHz, el período por preecalador:
 *              Prescaler   Período     1 mS                            1 Seg
 *              1           1/16 uSeg   16000  ciclos                   16 000 000
 *              8           1/2  uSeg   2000   ciclos                   2 000 000
 *              64          4    uSeg   250    ciclos                   125 000   
 *              256         16   uSeg   62,5   ciclos (62 = 0.992 mSeg) 62 500
 *              1024        64   uSeg   16,625 ciclos (16 = 0.960 mSeg) 16 625
*/
#define TIMER_PRESCALER    64

/** \def        MSEC_TO_CLOCK 
 *  \brief      De mseg a clk_ticks
 */
#define MSEC_TO_CLK(msec)   \
          (msec * F_CPU) / (1000UL * TIMER_PRESCALER)

/** \def        USEC_TO_CLOCK 
 *  \brief      De useg a clk_ticks
 *  \attention  La resolución del timer depende del prescalador (ver prescaler) por lo que
 *              a partir de TIMER_PRESCALER = 64 cada ciclo incluye varios microsegundos,
 *              así que deben evitarse valores menores que 4, 16 o 64 en caso de prescalardor
 *              64, 256, 1024 respectivamente. 
 *  \note       Si utiliza un prescalador de 64, 256, 1024, para valores exactos puede utilizar 
 *              valores de usec divisibles por 4, 16, o 64 respectivamente.
*/
#define USEC_TO_CLK(usec)   \
          (usec * FM_CPU) / TIMER_PRESCALER

/** \def    MAX_USEC
 *  \brief  Es la cantidad máxima de microsegundos que se pueden contar en un ciclo completo de
 *          del Timer1, que cuenta desde 0x0000 hasta 0xFFFF (16 bits) 65535
 */

#if TIMER_PRESCALER < 64

/** \def        USEC_PER_CLK_CYCLES
 *  \brief      Cantidad de incrementos del contador del timer TIMER_TEMP por cada microsegundo
 *  \attention  Esta constate solo existe para los prescaladores 1 y 8
 */
#define USEC_PER_CLK_CYCLES (FM_CPU / TIMER_PRESCALER)

#else
/** \def        CLK_CYCLES_PER_USEC
 *  \brief      Cantidad de microsegundos por cada incremento del contador del timer TIMER_TEMP
 *  \attention  Esta constate solo existe para los prescaladores 64, 256 y 1024
 */
#define CLK_CYCLES_PER_USEC (TIMER_PRESCALER / FM_CPU)

#endif

#define MAX_USEC            \
          0xFFFFUL * TIMER_PRESCALER / FM_CPU

/** \def    SEI_TIMER
 *  \brief  Activa la interrupción del timer. 
 * 
 *          Limpiar todas la baderas del comparador del Timer1 y se activa la interrupciónpor el  
 *          ComparadorA del Timer1 (OCIE1A).
*/
#define SEI_TIMER()                     \
            TIFR1 = 0xFF;               \
            TIMSK1 = 2

/** \def    CLI_TIMER
 *  \brief  Desactiva la interrupción del timer. 
 * 
 *          Se desactiva por el ComparadorA del Timer1.
*/
#define CLI_TIMER()\
            TIFR1 = 0xFF;               \
            (TIMSK1 = 0)                \

/** \def    IS_TIMER_SET 
 *  \brief  Verifica si la interrupción del Timer1 está activa (OCIE1A).
*/
#define IS_TIMER_SET (TIMSK1 & 2)
//!!!!#define IS_TIMER_SET (TIMSK1 & _BV(OCIE1A)) //_BV devuelve el valor de bit corrido <<, esta operación supuestamente no penaliza tiempo de cpu, hay que ver

/* -------------------------- Funciones ----------------------------------- */

/** \brief  Inicializamos el contador de tiempo del sistema que se utilizará en las funciones u_now() y m_now() 
 * 
 *          Se utiliza el Timer0 con un prescalador de 64, que significa 4usec por ciclo (resolución mínima) a 250 ciclos para 1 msec 
*/
void timer_sys_init_arch(void);

/** \brief  Devuelve los microsegundos desde reset 
 * 
 *          utime_t tiene 32 bits de largo por lo que se va a resetar cada 4295 segundos, unos cada ~ 1,20 horas.
 *          Los usec se leen como la suma del temporizador entre 2 y la cantidad de msec ((TCNT1 / 2) + m_now())
*/
utime_t u_now(void);

/** \brief  Devuelve los milisegundos desde reset 
 * 
 *          utime_t tiene 32 bits de largo por lo que se va a resetar cada ~ 3 días
 *  \note   Suponiendo que la INT de timer1 ocurra ejecutando mtime_t mtime = sys_msec la función tendrá un error máximo de 1 ms
*/
mtime_t m_now(void);

/** \brief  Inicializar el Timer1 a partir de la arquitectura avr. 
 * 
 *          Se activa la interrupción por el ComparadorA (FOC1A) y se configura el prescalador 
 *          según TIMER_PRESCALER.
 */
void rtimer_init_arch(void);


#endif //_TIMER_ARCH_H_