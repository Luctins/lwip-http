/**
* @file http.c
*/

#ifndef HTTP_H
#define HTTP_H

#include "lwip/ip_addr.h"
#include "hw_uart1.h"
#include "string.h"

/* --------- Defines --------- */
#define HTTP_MAX_PAYLOAD_LEN  4000
#define HTTP_R_OK 200


/* --------- Enums --------- */

/**
 * @brief connection States values under 0 are for errors.
    usually TCP error codes.
*/
enum conn_state {
  HTTP_IDLE = 0,  /* waiting */
  HTTP_CONN = 1,  /* connecting */
  HTTP_SEND = 2,  /* sending */
  HTTP_RECV = 3,  /* receiving data */
};

/**
 * @brief HTTP return codes
*/
enum http_err {
  HTTP_OK = 0, /*OK*/
  HTTP_ERR /* any reported error */
};



/**
 * @typedef http_cbfunc
 * receive confirmation callback func type.
 */
typedef void (*http_cbfunc)(uint16_t result, void * arg);

/*------Storage Classes-------*/

/**
 * @brief used to store connection variables and parameters
 */
typedef struct socket {
  struct ip_addr target_ip;           /**< target server ip*/
  uint32_t port;                      /**< target port*/
  uint8_t state;                      /**< connection state*/
  char payload[HTTP_MAX_PAYLOAD_LEN]; /**< buffer for payload (string) storage and manipulation*/
  uint16_t payload_len;               /**< payload length*/
  char * id;                          /**< socket id*/
  char * target;                      /**< server target (file)*/
  http_cbfunc callback;               /**< callback function*/
  void * arg;                         /**< argument*/
} http_sock_t;

/*--- functions ----- */


void http_init(http_sock_t *,http_cbfunc cb);

int http_request(http_sock_t * sock, const char * method,/* const char * host,
  const char * target,*/const char * headers,const char * payload,void * arg);

/**
 * @brief handler for connection and DHCP pooling
 */
void handle_http(void);


#endif /* HTTP_H */
