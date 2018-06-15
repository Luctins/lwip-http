/**
 * @file http.c
 * @author: Lucas Martins Mendes -
 * @date 2017/10/3
 *
 * @brief HTTP over LwIP Implementation
 */

//Stdlib
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//LwIP
#include <lwip/ip_addr.h>
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <lwip/tcp.h>
#include <lwip/inet.h>
#include <lwip/udp.h>
#include <lwip/dns.h>
#include <lwip/sys.h>

#include "ethernet/netconf.h"
#include "rtc.h"

#include "debug.h"

#include "http.h"

static uint32_t tmr = 0;
static uint8_t lwip_init=0; /**< flag to avoid calling lwip init multiple times*/

/*---------- local functions ---------*/
void _dummy(uint16_t resultcode, void * arg)
{
}

err_t _dummy2(void *arg, struct tcp_pcb * tpcb)
{
  return ERR_OK;
}

/* --- Internal Use prototypes --- */
err_t _connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t _sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
err_t _recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *recv, err_t err);
err_t _poll_cb(void *arg, struct tcp_pcb *tpcb);
err_t _accept(void *arg, struct tcp_pcb *newpcb, err_t err);
void _err(void *arg, err_t err);



/**
* @brief initalize http parameters
* @param  sock socket to be inialized
* @param  port port to be used
* @param id socket ID
* @return void
*/
void http_init(http_sock_t * sock, http_cbfunc cb)
{
  ASSERT_ERROR("socket is NULL", !sock, return);
  ASSERT_ERROR("callback is NULL", !cb, return);

  sock->callback=cb;

  if(!lwip_init)
  {
    DEBUG("initializing LwIP");
    LwIP_Init();
    lwip_init=1;
  }

  DEBUG("done");

  DEBUGF("socket config:\nip_addr:%lu.%lu.%lu.%lu\nport:%lu\n",
              (sock->target_ip.addr)&0xff,
              (sock->target_ip.addr>>8)&0xff,
              (sock->target_ip.addr>>16)&0xff,
              (sock->target_ip.addr>>24)&0xff, sock->port);
  sock->state=HTTP_IDLE;
  //DEBUG("http init done");
}



/**
 * Execute a HTTP request with given method to target in
 * the server ip address stored in the socket, using given headers and payload.
 *
 * @param  sock          connection socket containing server ip and port (and more)
 * @param  method        the method to be used
 * @param  headers       the content headers
 * @param  payload       payload to be sent
 * @param  arg           argument to be passed to callback
 *
 */
int http_request(
                http_sock_t * sock,
                const char * method,
                /*const char * host,*/
                /*const char * target,*/
                const char * headers,
                const char * payload,
                void * arg
                            )
{
  #define REQ_ARGS method, target, host, headers, payload
  const char * message_fmt = "%s %s HTTP/1.1\r\nHost: %s\r\n%s\r\n\r\n%s\r\n";

  char * target = sock->target;
  char * req_str = sock->payload;
  char host[50];
  uint16_t req_str_len;
  err_t err_result;
  struct tcp_pcb * tpcb;

  //Check ethernet Link & DHCP & not sending
  ASSERT_ERROR("not connected",
  (gnetif.flags & NETIF_FLAG_LINK_UP) == 0 || gnetif.dhcp->state != DHCP_BOUND,
                            sock->state=HTTP_IDLE; return HTTP_ERR;);
  ASSERT_ERROR("socket busy",
      (sock->state == HTTP_CONN) | (sock->state == HTTP_SEND),return HTTP_ERR;);

  ASSERT("arg is NULL",arg);

  sock->arg=arg;
  sock->state=HTTP_CONN;


  //SET_STATE(sock->state,HTTP_CONN);
  /*Assemble host string*/
  snprintf(host,50,"%lu.%lu.%lu.%lu",
                                  sock->target_ip.addr >> 0   & 0xff,
                                  sock->target_ip.addr >> 8   & 0xff,
                                  sock->target_ip.addr >> 16  & 0xff,
                                  sock->target_ip.addr >> 24  & 0xff);

  /* Assemble the request string */
  req_str_len= snprintf(NULL, 0, message_fmt, REQ_ARGS); /* Measure message len */
  ASSERT_ERROR("HTTP_PAYLOAD_MAX_LEN < req_str_len",req_str_len>=HTTP_MAX_PAYLOAD_LEN, return HTTP_ERR;);
  snprintf(req_str, HTTP_MAX_PAYLOAD_LEN, message_fmt, REQ_ARGS);
  DEBUGF("req_str_len: %d",req_str_len);

  sock->payload_len= req_str_len;
  //DEBUG("req_str: ");
  //DEBUG(req_str);

  /* configures the lwip callbacks for the current socket */
  DEBUG_OUTPUT("tcp_new");
  tpcb = tcp_new(); /* Creates a new TCP Protocol Control Block - TPCB */
  ASSERT_ERROR("tpcb is NULL", tpcb == NULL,tcp_close(tpcb); return HTTP_ERR; ); /* Check for empty pcb */
  tcp_arg(tpcb, sock); /* Give sock pointer to lwip as the argument passed to callbacks */
  tcp_sent(tpcb, _sent_cb); /*configures the "data sent" callback */
  tcp_recv(tpcb, _recv_cb); /*receive callback */
  tcp_err(tpcb, _err);/* set on error cb */
  tcp_poll(tpcb, _poll_cb, 20); /* poll every 20*500 ms (TCP_coarse_interval) */
  DEBUG("done");

  /* connect to server */
  DEBUG("connecting..");
  err_result=tcp_connect(tpcb, &sock->target_ip, sock->port, _connected);
  ASSERT_ERROR("tcp_connect ", err_result!=ERR_OK, return HTTP_ERR);

  #undef REQ_ARGS
  return HTTP_OK;
}

/*connected callback*/
err_t _connected( void * arg, struct tcp_pcb * tpcb, err_t err)
{
  err_t err_result;
  http_sock_t * sock = (http_sock_t *)arg;

  ASSERT("err != ERR_OK", err!=ERR_OK);

  DEBUG("connected");
  sock->state=HTTP_SEND;
  err_result=tcp_write(tpcb, sock->payload, sock->payload_len, 0x00);
  ASSERT_ERROR("tcp_write err",err_result!=ERR_OK,
              VAR_DUMP(err_result,"%d"); sock->callback(0 ,sock); return err_result;);
  tcp_output(tpcb);
  return err_result;
}

/* sent callback */
err_t _sent_cb(void  * arg, struct tcp_pcb * tpcb, u16_t len)
{
  DEBUGF("sent, %u bytes", len);
  ((http_sock_t *)arg)->state=HTTP_IDLE;
  return ERR_OK;
}

/*data received callback*/
err_t _recv_cb(void * arg, struct tcp_pcb * tpcb, struct pbuf * recv, err_t err)
{
  http_sock_t * sock = ((http_sock_t *)arg);
  uint16_t result;
  char result_str[30];

  DEBUG("recv ans");

  sock->state=HTTP_RECV;

  /*get http result code and pass it to callback*/
  memcpy(result_str,recv->payload,15);
  result_str[15]='\0';
  sscanf(result_str,"HTTP/1.1 %hu %*s",&result);

  ASSERT("err != ERR_OK", err != ERR_OK);

  /*print received data */
  ((char *)recv->payload)[50]='\0';
  DEBUG("\nreceived:");
  DEBUG((char *)recv->payload);
  DEBUG("..truncated to 50 char\n");


  /*If the callback is called with err=ERR_OK, it is responsible
  to deallocate the packet buffer (pbuf), else,
  do nothing, so lwip code can reuse it */
  if(err==ERR_OK) { pbuf_free(recv); DEBUG("pbuf freed"); }
  tpcb->state=8;/*/*Closing, BUG: needed because of bug in TCP/IP stack that
  causes circular list to form (with a neverending check loop)*/
  tcp_close(tpcb); DEBUG("conn close\r\n");
  sock->state=HTTP_IDLE;

  sock->callback(result,sock);
  #undef BUFFSIZE
  return ERR_OK;
}

/*error callback*/
void _err(void * arg, err_t err)
{
  http_sock_t * sock = (http_sock_t *)arg;
  DEBUGF("\n\nTCP ERROR:%d\n",err);
  sock->state= HTTP_IDLE;
  sock->callback(0, sock);
}

/*tcp stack calls this when the connection is idle*/
err_t _poll_cb(void * arg, struct tcp_pcb * tpcb)
{
  http_sock_t * sock = (http_sock_t *)arg;
  DEBUG("timeout reached, closing connection");
  sock->state= HTTP_IDLE;
  tcp_poll(tpcb, _dummy2, 20);/*remove idle callback*/
  /*BUG: set closing state before closing, else circular list loop*/
  tpcb->state=8;
  tcp_close(tpcb); DEBUG("tcp_close");
  sock->callback(0,sock);
  return ERR_OK;
}


//for lissening PCB's -- unused for now
err_t _accept(void *arg, struct tcp_pcb * newpcb, err_t err)
{
  DEBUG("tcp accept configured");
  return err;
}

void handle_http(void) {
  // pooling to check received packets and timeouts
  LwIP_Periodic_Handle();
  uint32_t timeNow = sys_now();
  uint32_t ip = gnetif.dhcp->offered_ip_addr.addr;

  char tmp[100];
  if (timeNow >= tmr + 2000 || timeNow < tmr) {
    DEBUGF("IP: %lu.%lu.%lu.%lu %16llu\t %9d",
            ip >> 0 & 0xFF,
            ip >> 8 & 0xFF,
            ip >> 16 & 0xFF,
            ip >> 24 & 0xFF,
            rtc_get64(),
            gnetif.dhcp->state);
  tmr = timeNow;
  }
}


#if  0


void handle_http(void) {
// pooling to check received packets and timeouts
LwIP_Periodic_Handle();
uint32_t timeNow = sys_now();
char tmp[100];
unsigned int written;
static uint32_t tmr;
if (timeNow >= tmr + 5000 || timeNow < tmr) {
uint32_t ip = gnetif.dhcp->offered_ip_addr.addr;
written=snprintf(tmp, 100, "\nIP: %lu.%lu.%lu.%lu %16llu %9d\n",
ip >> 0 & 0xFF,
ip >> 8 & 0xFF,
ip >> 16 & 0xFF,
ip >> 24 & 0xFF,
rtc_get64(), gnetif.dhcp->state);
tmp[written] = '\0';
DEBUG(tmp);
tmr = timeNow;
}
}


void handle_http(void) {
  //LwIP_Periodic_Handle();

  /*
  #ifdef DEBUG
  #define PRINT_INTERVAL 2000
  uint32_t timeNow = sys_now();
  static uint32_t tmr=!tmr?0:tmr;
  char tmp[100];
  if (timeNow >= tmr + PRINT_INTERVAL)
  {
    uint32_t ip = gnetif.dhcp->offered_ip_addr.addr;
    //uint32_t ip = gnetif.ip_addr.addr;
    snprintf(tmp, 100, "\nIP: %lu.%lu.%lu.%lu %16llu %9d\r\n",
            ip >> 0 & 0xFF,
            ip >> 8 & 0xFF,
            ip >> 16 & 0xFF,
            ip >> 24 & 0xFF,
            rtc_get64(),
            gnetif.dhcp->state);

    DEBUG(tmp);
    tmr[1] = timeNow;
  }
  #undef PRINT_INTERVAL
#endif // DEBUG
  */
}
#endif
