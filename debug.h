/**
 * @file debug.h
 * @brief debugging and testig macros.
 */

#ifndef DEBUG_H
#define DEBUG_H

/*------- Switch/Control defines*/
#define NO_WWDG //No watchdog for now
#define NO_CRC
#define IWDG_RELOAD_TIMER 5000 //  reload every 5 s
#define FULL_TEST


#define DEBUG_BUFFER_LEN 500


/*GREEN*/
#define LED1_PIN GPIO_Pin_0
#define LED1_PORT GPIOB

/*BLUE*/
#define LED2_PIN  GPIO_Pin_7
#define LED2_PORT GPIOB

/*RED*/
#define LED3_PIN GPIO_Pin_14
#define LED3_PORT GPIOB


#include <string.h>
//#include <stdio.h>


/**
 * @brief number of the uart used for debugging
 */
#define DEBUG_UART_NUM  4

#define EXPANDED__LINE__

/**
 * @brief helper macro
 * @param  x value to be stringified
 */
#define _STR(x) #x

/**
 * @brief convert constant to string, even macro-defined
 * @param  x integer
 */
#define STRINGIFY(x) _STR(x)

/**
 * @brief helper macro (layer of separation)
 */
#define _TOKENPASTE(x,y,z) x ## y ## z

/**
* @brief paste tokens together with a added layer of indirection
*/
#define TOKENPASTE(x,y,z) _TOKENPASTE(x,y,z)

/**
 * @brief helper macro (layer of separation)
 */
#define _DEBUG_OUTPUT(n)  TOKENPASTE(uart,n,Print)

/**
 * @brief debug output function (token) generator
 */
#define DEBUG_OUTPUT _DEBUG_OUTPUT(DEBUG_UART_NUM)

/**
 * @brief helper macro
 */
#define _DEBUG_UART_READ(n) TOKENPASTE(hw_uart,n,_read)
/**
 * @brief standard debug input
 */
#define DEBUG_UART_READ _DEBUG_UART_READ(DEBUG_UART_NUM)
#define _DEBUG_OUTPUT_INIT(n)  TOKENPASTE(hw_uart,n,_Init)
#define DEBUG_OUTPUT_INIT _DEBUG_OUTPUT_INIT(DEBUG_UART_NUM)


/**
* @brief the include file name changes according to the number defined
*/
#define DEBUG_FILEEXT .h
#define DEBUG_UART_FILE(n) STRINGIFY(TOKENPASTE(hw_uart,n,)DEBUG_FILEEXT)
#define DEBUG_UART_INC DEBUG_UART_FILE(DEBUG_UART_NUM)
#include DEBUG_UART_INC


/*--------- DEBUG Macros -----------*/

#ifdef DEBUG_OUTPUT /*DEBUG Macros (disable if DEBUG_OUTPUT isn't defined)*/

/*
 * @brief Print Debug string to Uart and adding a newline
 * @param  str Debug string
 */
#ifndef DEBUG
#define DEBUG(str) \
DEBUG_OUTPUT(str);     \
DEBUG_OUTPUT("\n");
#endif

/**
 * formatted debug for use with threads
 * @param  format  format string
 * @param  VARARGS variable number of arguments
 */
#ifndef DEBUGF
#define DEBUGF(format, ...)                                   \
do {                                                          \
  extern char __buff[DEBUG_BUFFER_LEN];                       \
  snprintf(__buff, DEBUG_BUFFER_LEN, format, __VA_ARGS__);    \
  DEBUG_OUTPUT(__buff);                                       \
  DEBUG_OUTPUT("\n");                                         \
  } while (0)
#endif /* DEBUGF */

/**
 * [BREAK_POINT description]
 * @param  msg [description]
 * @return     [description]
 */
#ifndef BREAK_POINT
#define BREAK_POINT(msg)                                           \
DEBUG_OUTPUT("BREAK_POINT:" __FILE__ ":" STRINGIFY(__LINE__) ":"); \
DEBUG_OUTPUT(msg);                                                 \
DEBUG(" -- press 'c' to continue");                            \
while (1)  {                                                       \
  char c;                                                          \
  DEBUG_UART_READ(&c);                                             \
  if(c=='c') { break; }                                            \
}
#endif

/**
 * [VAR_DUMP description]
 * @param  var [description]
 * @param  fmt [description]
 * @return     [description]
 */
#ifndef VAR_DUMP
#define VAR_DUMP(var,fmt) DEBUG_MSG_F(#var":"fmt,var)
#endif

#ifndef  ITER_VAR_DUMP
#define ITER_VAR_DUMP(buffer,len,fmt)             \
for(uint16_t iterpos = 0;iterpos<len;iterpos++)   \
{                                                 \
  DEBUG_OUTPUT_F(fmt,*((buffer)+iterpos));        \
}                                                 \
DEBUG_OUTPUT("\n")
#endif

/**
* @brief print formatted string to Uart
* @param  format  format string
* @param  VARARGS variadic arguments
*/
#ifndef DEBUG_MSG_F
#define DEBUG_MSG_F(format, ...)              \
do {                                          \
  extern char __buff[DEBUG_BUFFER_LEN];                    \
  snprintf(__buff, DEBUG_BUFFER_LEN, format, __VA_ARGS__); \
  DEBUG_OUTPUT(__buff);                       \
  DEBUG_OUTPUT("\n");                         \
} while (0)
#endif /* DEBUGF */

/**
 * output formatted debug message withot trailing newline.
 * @param  format  format string.
 * @param  VARARGS arguments to print.
 */
#ifndef DEBUG_OUTPUT_F
#define DEBUG_OUTPUT_F(format, ...)            \
do {                                           \
  extern char __buff[DEBUG_BUFFER_LEN];                     \
  snprintf(__buff, DEBUG_BUFFER_LEN, format, __VA_ARGS__);  \
  DEBUG_OUTPUT(__buff);                        \
} while (0)
#endif/* DEBUG_OUTPUT_F */

/**
 * @brief Perform basic assertion  based on a condition
 * @param  str  [description]
 * @param  cond [description]
 */
#ifndef ASSERT
#define ASSERT(str, cond)                                             \
if (cond) {                                                           \
  DEBUG_OUTPUT("ASSERT:" __FILE__ ":" STRINGIFY(__LINE__) ": at ");   \
  DEBUG_OUTPUT(__func__);                                             \
  DEBUG_OUTPUT(":");                                                  \
  DEBUG_OUTPUT(str);                                                  \
  DEBUG_OUTPUT("\n");                                                 \
}
#endif /* ASSERT */

/**
 * assert errors, and operate on them based on a condition.
 *
 * @param  str     debug string
 * @param  cond    condition
 * @param  finally on err operation (e.g. a statement, like "return ERR;")
 */
#ifndef ASSERT_ERROR
#define ASSERT_ERROR(str, cond, finally)                              \
if (cond) {                                                           \
  DEBUG_OUTPUT("\n\n\tERROR:" __FILE__ ":" STRINGIFY(__LINE__) ":");  \
  DEBUG_OUTPUT(__func__);                                             \
  DEBUG_OUTPUT(":");                                                  \
  DEBUG_OUTPUT(str);                                                  \
  DEBUG_OUTPUT("\n\n");                                               \
  finally;                                                            \
}
#endif /* ASSERT_ERROR */

/**
 * assert err with autogen err string
 * @param  str  [description]
 * @param  cond [description]
 * @return      [description]
 */
#ifndef A_ASSERT_ERROR
#define A_ASSERT_ERROR(cond, finally)                             \
if (cond) {                                                    \
  DEBUG_OUTPUT("\n\n\tERROR:" __FILE__ ":" STRINGIFY(__LINE__) ":");  \
  DEBUG_OUTPUT(__func__);                                             \
  DEBUG_OUTPUT(":" #cond);                                            \
  DEBUG_OUTPUT("\n\n");                                               \
  finally;                                                            \
}
#endif

/**
 * Assert critical message and lock up CPU.
 *
 * @param  str  to be printed on error
 * @param  cond condition for critical error
 */
#ifndef ASSERT_CRITICAL
#define ASSERT_CRITICAL(str, cond)                                            \
if (cond) {                                                                   \
  DEBUG_OUTPUT("\n\n\tCRITICAL ERROR:" __FILE__ ":" STRINGIFY(__LINE__) ":"); \
  DEBUG_OUTPUT(__func__);                                                     \
  DEBUG_OUTPUT(":");                                                          \
  DEBUG_OUTPUT(str);                                                          \
  DEBUG_OUTPUT("\n");                                                         \
  GPIO_SetBits(LED3_PORT, LED3_PIN);                                          \
  vTaskSuspendAll();/*if this does not work try "int a=1/0;"*/                                                      \
  while(1);                                                                   \
}
#endif /* ASSERT_CRITICAL */

/**
 * Same as ASSERT_CRITICAL but with aditional clause for printing any
 * relevant information.
 *
 * @param  str     message
 * @param  cond    condition for critical error
 * @param  finally operation to perform on critical error
 */
#ifndef ASSERT_CRITICAL_WF
#define ASSERT_CRITICAL_WF(str, cond, finally)                                \
if (cond) {                                                                   \
  DEBUG_OUTPUT("\n\n\tCRITICAL ERROR:" __FILE__ ":" STRINGIFY(__LINE__) ":"); \
  DEBUG_OUTPUT(__func__);                                                     \
  DEBUG_OUTPUT(":");                                                          \
  DEBUG_OUTPUT(str);                                                          \
  DEBUG_OUTPUT("\n");                                                         \
  GPIO_SetBits(LED3_PORT, LED3_PIN);                                          \
  finally;                                                                    \
  while(1);                                                                   \
}
#endif /* ASSERT_CRITICAL */

#else /* DEBUG */

#define BREAK_POINT(msg)
#define SET_STATE(statevar,state ) statevar=state;
#define DEBUG_OUTPUT(msg)
#define DEBUG(str)
#define DEBUGF(str, ...)
#define ASSERT_ERROR(str, cond, finally)
#define ASSERT_CRITICAL(str, cond)
#define A_ASSERT_CRITICAL(str, cond, finally)
#define ASSERT(str, cond)

#endif /* DEBUG */

#endif /*DEBUG_H*/
