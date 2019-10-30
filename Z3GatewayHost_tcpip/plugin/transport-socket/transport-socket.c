// Copyright 2018 Silicon Laboratories, Inc.                                *80*

/**
 * @file
 * \brief The transportation layer based on socket. Note that it only
 *        supports TCP/IPv4.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "transport-socket.h"

#ifndef INT_TEST
  #include PLATFORM_HEADER
  #include "stack/include/ember-types.h"
  #include "stack/include/event.h"
  #include "hal/hal.h"
  #include "app/framework/include/af.h"
#else
  #include <assert.h>
#endif

// Error codes.
#define SOCKET_SUCCESS     (0)
#define SOCKET_LOST_CONN   (0)
#define SOCKET_ERROR       (-1)
#define RAW_DATA_ERROR     (-2)
#define BUFFER_ERROR       (-3)
#define INVALID_RD_BYTES   (-4)
#define INVALID_SD_BYTES   (-5)
// Delays and intervals.
#define CONNECTION_THREAD_DELAY_S   (2)
#define WRITE_SOCK_THREAD_DELAY_MS  (50)
#define RECV_SOCK_THREAD_DELAY_MS   (10)
#define LONG_LINK_ITVL_MS           (2000)
// Buffer lengths.
#define RECV_BUF_MAX       (1000)
#define WRITE_BUF_MAX      (1000)
// Others.
#define RETRY_MAX                   (5)
#define MSG_CNT_DISCONN_MAX         (10)
#define EISCONN_STANDBY_MAX         (5)

#define ARRAY_LEN(x)       (sizeof(x) / sizeof((x)[0]))
#ifdef INT_TEST
  #define TEST_ARRT static
#else
  #define TEST_ARRT
#endif
#ifdef INT_TEST
  #define DEBUG_PRT printf
#elif EMBER_AF_PRINT_CORE
  #define DEBUG_PRT emberAfCorePrint
#else
  #define DEBUG_PRT
#endif

typedef enum _SocketStatus {
  CONNECTED = 0,   // Connected
  CON_IN_PROG,     // Connetion in progress
  DISCONNECTED,    // Disconnected
} SocketStatus;

typedef struct _SocketInfo {
  int  tcpSocketFd;
  uint16_t longLinkItvl;
  SocketStatus sockStatus;
  struct sockaddr_in servAddr;
} SocketInfo;

typedef struct _EmberAfPluginLinkedListElement {
  struct _EmberAfPluginLinkedListElement * next;
  struct _EmberAfPluginLinkedListElement * previous;
  void * content;
} EmberAfPluginLinkedListElement;

typedef struct {
  EmberAfPluginLinkedListElement * head;
  EmberAfPluginLinkedListElement * tail;
  uint32_t count;
} EmberAfPluginLinkedList;

TEST_ARRT const char * defPort   = "50000";
TEST_ARRT const char * defHostIP = "127.0.0.1";
TEST_ARRT char connPort[10]   = { 0 };
TEST_ARRT char connHostIP[20] = { 0 };

static SocketInfo sockInfo = {
  .tcpSocketFd    = SOCKET_ERROR,
  .longLinkItvl   = LONG_LINK_ITVL_MS,
  .sockStatus     = DISCONNECTED,
};

static pthread_t connectionThreadId;
static pthread_t writeSocketThreadId;
static pthread_t recvSocketThreadId;
static pthread_mutex_t sockInfoMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sendBufMutex  = PTHREAD_MUTEX_INITIALIZER;
#ifndef INT_TEST
EmberEventControl emberAfPluginTransportSocketLongLinkEventControl;
#endif
TEST_ARRT void emberAfPluginTransportSocketLongLinkEventHandler(void);
static char recvBuf[RECV_BUF_MAX];
static EmberAfPluginLinkedList * sendBuf = NULL;

// Forward declaration.
static void connectionThread(void * param);
static void recvSocketThread(void * param);
static void writeSocketThread(void * param);
#ifdef INT_TEST
TEST_ARRT bool emberAfPluginTransportSocketPublish(const char * payload);
#endif

/**********************************************************************
 * The following methods are based the linked-list plugin.            *
 * The original plugin has the limitation of free allocated memories. *
 * The following methods resolve such a limitation.                   *
 **********************************************************************/

static EmberAfPluginLinkedList * emberAfPluginLinkedListInit(void)
{
  EmberAfPluginLinkedList *list =
    (EmberAfPluginLinkedList *)malloc(sizeof(EmberAfPluginLinkedList));
  if (list != NULL) {
    memset(list, 0, sizeof(EmberAfPluginLinkedList));
  }
  return list;
}

static void emberAfPluginLinkedListPushBack(
  EmberAfPluginLinkedList *list,
  void* content)
{
  EmberAfPluginLinkedListElement *element =
    (EmberAfPluginLinkedListElement *)malloc(sizeof(
                                    EmberAfPluginLinkedListElement));
  if (element != NULL) {
    element->content = content;
    element->next = NULL;
    element->previous = list->tail;
    if (list->head == NULL) {
      list->head = element;
    } else {
      list->tail->next = element;
    }
    list->tail = element;
    ++(list->count);
  }
}

static void emberAfPluginLinkedListPopFront(
                                       EmberAfPluginLinkedList *list)
{
  if (list->count > 0) {
    EmberAfPluginLinkedListElement* head = list->head;
    if (list->tail == head) {
      list->tail = NULL;
    }
    list->head = list->head->next;
    if (head->content) free(head->content);
    free(head);
    --(list->count);
  }
}

static bool emberAfPluginLinkedListRemoveElement(
  EmberAfPluginLinkedList *list,
  EmberAfPluginLinkedListElement *element)
{
  if ((element != NULL) && (list->head != NULL)) {
    if (element == list->head) {
      if (list->head == list->tail) {
        list->head = NULL;
        list->tail = NULL;
      } else {
        list->head = element->next;
        element->next->previous = NULL;
      }
    } else if (element == list->tail) {
      list->tail = element->previous;
      element->previous->next = NULL;
    } else {
      element->previous->next = element->next;
      element->next->previous = element->previous;
    }
    --(list->count);
    if (element->content) free(element->content);
    free(element);
    return true;
  }
  return false;
}

static EmberAfPluginLinkedListElement * emberAfPluginLinkedListNextElement(
  EmberAfPluginLinkedList *list,
  EmberAfPluginLinkedListElement *elementPosition)
{
  if (elementPosition == NULL) {
    return list->head;
  } else {
    return elementPosition->next;
  }
}

/* Helper methods */
static EmberAfPluginLinkedListElement * findElementInList(char ** buf)
{
  EmberAfPluginLinkedListElement *curElement = NULL;

  do {
    curElement = emberAfPluginLinkedListNextElement(sendBuf,
                                                    curElement);
    if (curElement != NULL &&
        (char *)curElement->content == *buf) {
      return curElement;
    }
  } while (curElement != NULL);

  return NULL;
}

static void freeBufInList(char ** buf)
{
  EmberAfPluginLinkedListElement *element = NULL;

  pthread_mutex_lock(&sendBufMutex);

  element = findElementInList(buf);
  if (element) {
    emberAfPluginLinkedListRemoveElement(sendBuf, element);
  }

  pthread_mutex_unlock(&sendBufMutex);
}

static void disconnectionEventHandler(void)
{
  return;
}

static void socketRawDataRecvdEventHandler(char * buf, size_t recvBytes)
{
  if (!buf) return;
  buf[recvBytes - 1] = '\0';

#ifdef INT_TEST
  static uint32_t msgCnt = 0;

  DEBUG_PRT("Recv'd recvBytes-%lu, msgLen-%lu, msg-{%s} [seq#: %u]\r\n",
            recvBytes,
            strlen(buf),
            buf,
            msgCnt);
  msgCnt++;
#else
  emberAfPluginTransportSocketMessageArrivedCallback(buf);
#endif
}

static void writeSocketErrEventHandler(char ** buf)
{
  // Basically does nothing. The payload should stay in the list.
  (void)buf;
}

static void partialWriteEventHandler(char ** buf, size_t actualBytes)
{
  static char * preBuf = NULL;
  static uint8_t attempts = 0;

  // After attempting to resend several times, remove it from the list.
  if (!preBuf && attempts == 0) {
    preBuf = *buf;
  } else {
    if (preBuf == *buf && attempts < RETRY_MAX) {
      attempts++;
    } else {
      attempts = 0;
      preBuf = NULL;
      freeBufInList(buf);
    }
  }
}

static void writeSocketSucEventHandler(char ** buf)
{
  freeBufInList(buf);
}

static void brokenPipeEventHandler(int sig)
{
  (void)sig;
}

/**
 * @brief: Set non-blocking mode to a socket.
 * @param[Out]: sockfd - sockect to be set.
 * @return: -1 indicates error and EAGAIN will be set.
 */
static int setNonBlocking(int * sockfd)
{
  int ret;
  int flags;

  flags = fcntl(*sockfd, F_GETFL, 0);
  ret = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);

  return ret;
}

/**
 * @brief: Create a new TCPv4 socket.
 * @param[Out]: sockfd - sockect to be set.
 * @return: -1 indicates error. Other number indicates a fd.
 */
static int newTcpSocket(int * sockfd)
{
  return *sockfd = socket(AF_INET, SOCK_STREAM, 0);
}

/**
 * @brief: Validate the server IP and port specified by users.
 * @param[Out]: servAddr - server address to be set.
 * @return: -1 indicates error and EAFNOSUPPORT will be set.
 */
static int validateServerAddr(struct sockaddr_in * servAddr)
{
  int ret = SOCKET_ERROR;
  int port;

  bzero((void *)servAddr, sizeof(*servAddr));

#ifdef EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_PORT
  port = atoi(EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_PORT);
  strncpy(connPort,
          EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_PORT,
          sizeof(connPort));
  connPort[sizeof(connPort) - 1] = '\0';
#else
  port = atoi(defPort);
  strncpy(connPort,
          defPort,
          sizeof(connPort));
  connPort[sizeof(connPort) - 1] = '\0';
#endif

#ifdef EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_IP
  ret = inet_pton(AF_INET,
                  EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_IP,
                  &(servAddr->sin_addr));
  strncpy(connHostIP,
          EMBER_AF_PLUGIN_TRANSPORT_SOCKET_SERVER_IP,
          sizeof(connHostIP));
  connHostIP[sizeof(connHostIP) - 1] = '\0';
#else
  ret = inet_pton(AF_INET, defHostIP, &(servAddr->sin_addr));
  strncpy(connHostIP,
          defHostIP,
          sizeof(connHostIP));
  connHostIP[sizeof(connHostIP) - 1] = '\0';
#endif

  servAddr->sin_port = htons(port);
  servAddr->sin_family = AF_INET;
  return ret;
}

/**
 * @brief: Connect to the server IP:port.
 * @param[In]: sockfd - socket fd.
 * @param[In]: servAddr - server address structure.
 * @return: -1 indicates error.
 */
static int connectTcpSocket(const int * sockfd,
                            const struct sockaddr_in * servAddr)
{
  return connect(*sockfd, (struct sockaddr *)servAddr, sizeof(*servAddr));
}

/**
 * @brief: Close a specified socket.
 * @param[In]: sockfd - socket fd.
 * @return: -1 indicates error.
 */
static int closeTcpSocket(const int * sockfd)
{
  return close(*sockfd);
}

/**
 * @brief: Tries to read bytes from a specified socket. As the commands
 *         from upper layers are not large, we use a fixed buffer with
 *         reasonable size to hold the commands.
 * @param[In]: sockfd - socket fd.
 * @param[Out]: buf - a pointer to the buffer to be filled with.
 * @param[In]: bytes - No. of bytes to be read.
 * @param[In]: disconnetCb - a callback for socket disconnection.
 * @param[In]: rawRecvdCb - a callback for received raw data.
 * @return: 0 indicates success. < 0 values indicate error code.
 */
static int getSocketData(const int * sockfd,
                         char * buf,
                         size_t bytes,
                         void (*disconnetCb)(void),
                         void (*rawRecvdCb)(char * buf, size_t recvBytes))
{
  int ret;

  if (!buf) {
    ret = BUFFER_ERROR;
    goto exit;
  } else if (bytes == 0) {
    ret = INVALID_RD_BYTES;
    goto exit;
  }

  if ((ret = recv(*sockfd, buf, bytes, 0)) == SOCKET_ERROR) {
    goto exit;
  } else if (ret == 0) {
    if (disconnetCb) disconnetCb();
    goto exit;
  } else {
    if (rawRecvdCb) rawRecvdCb(buf, ret);
  }

exit:
  return ret;
}

/**
 * @brief: Tries to send bytes to a specified socket. As long as the
 *         data to be sent is less than PIPE_BUF (normally 65KBytes),
 *         we should be fine using this method.
 * @param[In]: sockfd - socket fd.
 * @param[In]: buf - the buf whose data to be sent.
 * @param[In]: bufLen - the size of data to be sent.
 * @param[In]: writeSocketErrCb - a callback for the socket error
 *                                when writing. Pass the buffer as param.
 * @param[In]: partialWriteCb - a callback for partial bytes written
 *                              error. Pass the buffer and actual written
 *                              bytes as params.
 * @param[In]: writeSocketSucCb - a callback for successful writing to a
 *                                socket. Pass the buffer as param.
 * @return: 0 indicates success. '< 0' values indicate error code.
 */
static int putSocketData(const int * sockfd,
                         char ** buf,
                         size_t bufLen,
                         void (*writeSocketErrCb)(char ** buf),
                         void (*partialWriteCb)(char ** buf,
                                                size_t actualBytes),
                         void (*writeSocketSucCb)(char ** buf))
{
  int rc, ret;

  if (!buf) {
    ret = BUFFER_ERROR;
    goto exit;
  } else if (bufLen == 0) {
    ret = INVALID_SD_BYTES;
    goto exit;
  }

  if ((rc = write(*sockfd, *buf, bufLen)) != SOCKET_ERROR) {
    if (rc < bufLen) {
      ret = INVALID_SD_BYTES;
      if (partialWriteCb) partialWriteCb(buf, (size_t)rc);
    } else {
      ret = SOCKET_SUCCESS;
      if (writeSocketSucCb) writeSocketSucCb(buf);
    }
  } else {
    ret = SOCKET_ERROR;
    if (writeSocketErrCb) writeSocketErrCb(buf);
  }

exit:
  return ret;
}

static void delayMs(uint32_t delayMs)
{
  struct timespec delayTime;

  delayTime.tv_sec = delayMs / 1000;
  delayTime.tv_nsec = (delayMs % 1000) * 1000000;
  nanosleep(&delayTime, NULL);
}

static void delayS(uint32_t delayS)
{
  sleep(delayS);
}

static void printConnStatus(SocketStatus status)
{
  static bool inProgPtrd = false;

  switch(status)
  {
    case CON_IN_PROG:
      if (!inProgPtrd) {
        DEBUG_PRT("TCP client is connecting to %s:%s\r\n",
                  connHostIP, connPort);
        inProgPtrd = true;
      }
      break;
    case CONNECTED:
      DEBUG_PRT("TCP client is connected to %s:%s\r\n",
                connHostIP, connPort);
      inProgPtrd = false;
      break;
    case DISCONNECTED:
      DEBUG_PRT("TCP client is disconnected from %s:%s\r\n",
                connHostIP, connPort);
      inProgPtrd = false;
      break;
    default:
      break;
  }
}

/* Initialize functions */
static void initSignalHandler(void)
{
  signal(SIGPIPE, brokenPipeEventHandler);
}

static void initEventControl(void)
{
#ifndef INT_TEST
  emberEventControlSetActive(
    emberAfPluginTransportSocketLongLinkEventControl);
#endif
}

static void initBuffer(void)
{
  memset((void *)recvBuf, 0, sizeof(recvBuf));
  sendBuf = emberAfPluginLinkedListInit();
}

static void initTcpSocket(void)
{
  assert(newTcpSocket(&sockInfo.tcpSocketFd)   != -1);
  assert(validateServerAddr(&sockInfo.servAddr) == 1);
  assert(setNonBlocking(&sockInfo.tcpSocketFd)  == 0);
}

static void initThreads(void)
{
  pthread_mutexattr_t attr;

  assert(pthread_mutexattr_init(&attr) == 0);
  assert(pthread_mutex_init(&sockInfoMutex, &attr) == 0);
  assert(pthread_mutex_init(&sendBufMutex,  &attr) == 0);
  assert(pthread_create(&connectionThreadId,
                        NULL,
                        (void *)connectionThread,
                        NULL) == 0);
  assert(pthread_create(&writeSocketThreadId,
                        NULL,
                        (void *)writeSocketThread,
                        NULL) == 0);
  assert(pthread_create(&recvSocketThreadId,
                        NULL,
                        (void *)recvSocketThread,
                        NULL) == 0);
}

/* Methods called by threads. */
static void connectionThread(void * param)
{
  int rc;
  static uint8_t eiconnCnt = 0;

  for(;;)
  {
    pthread_mutex_lock(&sockInfoMutex);

    if (sockInfo.sockStatus != CONNECTED) {
      if ((rc = connectTcpSocket(&sockInfo.tcpSocketFd,
                                 &sockInfo.servAddr)) == 0) {
        sockInfo.sockStatus = CONNECTED;
      } else {
        if (errno == EINPROGRESS) {
          sockInfo.sockStatus = CON_IN_PROG;
          eiconnCnt = 0;
          printConnStatus(CON_IN_PROG);
        } else if (errno == EISCONN &&
                   eiconnCnt < EISCONN_STANDBY_MAX) {
          // This gives EISCONN_STANDBY_MAX * CONNECTION_THREAD_DELAY_S
          // seconds before the client tries to reconnect.
          sockInfo.sockStatus = CONNECTED;
          eiconnCnt++;
        } else {
          sockInfo.sockStatus = DISCONNECTED;
          closeTcpSocket(&sockInfo.tcpSocketFd);
          initTcpSocket();
          eiconnCnt = 0;
        }
      }
    }

    pthread_mutex_unlock(&sockInfoMutex);
    delayS(CONNECTION_THREAD_DELAY_S);
  }
}

static void writeSocketThread(void * param)
{
#ifdef INT_TEST
  int rc;
  char *buf;
  char **payload;
  EmberAfPluginLinkedListElement *bufHd = NULL;

  for(;;)
  {
    pthread_mutex_lock(&sockInfoMutex);

    buf = (char *)malloc(sizeof("Transport-socket - test data!"));
    strcpy(buf, "Transport-socket - test data!");
    emberAfPluginTransportSocketPublish(buf);

    if (sockInfo.sockStatus == CONNECTED) {

      bufHd = emberAfPluginLinkedListNextElement(sendBuf,
                                                 NULL);
      if (!bufHd) continue;
      payload = (char **)&bufHd->content;

      rc = putSocketData(&sockInfo.tcpSocketFd,
                         payload,
                         strlen(*payload) + 1,
                         writeSocketErrEventHandler,
                         partialWriteEventHandler,
                         writeSocketSucEventHandler);
      if (rc == SOCKET_ERROR) {
        sockInfo.sockStatus = DISCONNECTED;
      }
    }

    pthread_mutex_unlock(&sockInfoMutex);
    delayS(1);
  }

#else
  int rc;
  char **payload;
  EmberAfPluginLinkedListElement *bufHd = NULL;

  for(;;)
  {
    pthread_mutex_lock(&sockInfoMutex);

    if (sockInfo.sockStatus == CONNECTED &&
        sendBuf &&
        sendBuf->count > 0) {

      bufHd = emberAfPluginLinkedListNextElement(sendBuf,
                                                 NULL);

      if (!bufHd) continue;
      payload = (char **)&bufHd->content;
      rc = putSocketData(&sockInfo.tcpSocketFd,
                         payload,
                         strlen(*payload) + 1,
                         writeSocketErrEventHandler,
                         partialWriteEventHandler,
                         writeSocketSucEventHandler);
      if (rc == SOCKET_ERROR) {
        sockInfo.sockStatus = DISCONNECTED;
      }
    }

    pthread_mutex_unlock(&sockInfoMutex);
    delayMs(WRITE_SOCK_THREAD_DELAY_MS);
  }
#endif  // INT_TEST
}

static void recvSocketThread(void * param)
{
  int rc;

  for(;;)
  {
    pthread_mutex_lock(&sockInfoMutex);

    if (sockInfo.sockStatus == CONNECTED) {
      rc = getSocketData(&sockInfo.tcpSocketFd,
                         recvBuf,
                         RECV_BUF_MAX,
                         disconnectionEventHandler,
                         socketRawDataRecvdEventHandler);
      if (rc == SOCKET_LOST_CONN) {
        sockInfo.sockStatus = DISCONNECTED;
      }
    }

    pthread_mutex_unlock(&sockInfoMutex);
    delayMs(RECV_SOCK_THREAD_DELAY_MS);
  }
}

/**********************************************************************
 * Ember Application Framework - Event handlers and callbacks.        *
 **********************************************************************/

/**
 * @brief: This callback should be called when main init is executing.
 */
TEST_ARRT void emberAfPluginTransportSocketInitCallback(void)
{
  initBuffer();
  initSignalHandler();
  initTcpSocket();
	initThreads();
  initEventControl();
}

/**
 * @brief: The event control will be specified by the properties file, and
 *         generated by the app-builder.
 */

TEST_ARRT void emberAfPluginTransportSocketLongLinkEventHandler(void)
{
  char data;
  EmberAfPluginTransportSocketState curStatus;
  static EmberAfPluginTransportSocketState lastStatus = 
              EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_DISCONNECTED;

#ifndef INT_TEST
  emberEventControlSetInactive(
    emberAfPluginTransportSocketLongLinkEventControl);
#endif
  pthread_mutex_lock(&sockInfoMutex);

  if (sockInfo.sockStatus == CONNECTED &&
      recv(sockInfo.tcpSocketFd, &data, 1, MSG_PEEK) == 0) {
    sockInfo.sockStatus = DISCONNECTED;
    closeTcpSocket(&sockInfo.tcpSocketFd);
    initTcpSocket();
  }

  if (sockInfo.sockStatus == CONNECTED) {
    curStatus = EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_CONNECTED;
  } else {
    curStatus = EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_DISCONNECTED;
  }

  pthread_mutex_unlock(&sockInfoMutex);

  if (curStatus != lastStatus) {
#ifndef INT_TEST
    emberAfPluginTransportSocketStateChangedCallback(curStatus);
#endif
    if (curStatus == EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_CONNECTED) {
      printConnStatus(CONNECTED);
    } else {
      printConnStatus(DISCONNECTED);
    }
  }

  lastStatus = curStatus;

#ifndef INT_TEST
  emberEventControlSetDelayMS(
    emberAfPluginTransportSocketLongLinkEventControl,
    sockInfo.longLinkItvl);
#endif
}

/**
 * @brief: This method is meant to be called by gatway-relay-socket.
 * @param[In]: payload - The payload to be inserted to the send-buffer.
 *                       It should be stored in a malloc'd memory. The
 *                       garbage collection is handled at this layer.
 */
TEST_ARRT bool emberAfPluginTransportSocketPublish(const char * payload)
{
  SocketStatus status;

  if (!payload) return false;

  #ifndef INT_TEST
  pthread_mutex_lock(&sockInfoMutex);
  #endif
  status = sockInfo.sockStatus;

  if(status == CONNECTED) {
    pthread_mutex_lock(&sendBufMutex);
    emberAfPluginLinkedListPushBack(sendBuf, (void *)payload);
    pthread_mutex_unlock(&sendBufMutex);
  } else {
    if (sendBuf && sendBuf->count >= MSG_CNT_DISCONN_MAX) {
      pthread_mutex_lock(&sendBufMutex);
      emberAfPluginLinkedListPopFront(sendBuf);
      emberAfPluginLinkedListPushBack(sendBuf, (void *)payload);
      pthread_mutex_unlock(&sendBufMutex);
    } else if (sendBuf && sendBuf->count < MSG_CNT_DISCONN_MAX) {
      pthread_mutex_lock(&sendBufMutex);
      emberAfPluginLinkedListPushBack(sendBuf, (void *)payload);
      pthread_mutex_unlock(&sendBufMutex);
    } else {
      #ifndef INT_TEST
      pthread_mutex_unlock(&sockInfoMutex);
      #endif
      return false;
    }
  }
  #ifndef INT_TEST
  pthread_mutex_unlock(&sockInfoMutex);
  #endif

  return true;
}
