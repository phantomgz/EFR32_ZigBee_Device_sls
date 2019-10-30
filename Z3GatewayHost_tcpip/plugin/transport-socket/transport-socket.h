// Copyright 2018 Silicon Laboratories, Inc.                                *80*

#ifndef __TRANSPORT_SOCKET_H
#define __TRANSPORT_SOCKET_H

#ifdef INT_TEST
  #include <stdbool.h>
#endif

typedef enum {
  EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_CONNECTED,
  EMBER_AF_PLUGIN_TRANSPORT_SOCKET_STATE_DISCONNECTED
} EmberAfPluginTransportSocketState;

#ifndef INT_TEST
  bool emberAfPluginTransportSocketPublish(const char * payload);
#endif

#endif // __TRANSPORT_SOCKET_H