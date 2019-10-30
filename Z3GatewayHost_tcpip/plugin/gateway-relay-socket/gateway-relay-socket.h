// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#ifndef SILABS_GATEWAY_RELAY_SOCKET_H
#define SILABS_GATEWAY_RELAY_SOCKET_H

/** @brief publish the unicast error status to a node.
 */
void emberPluginGatewayRelaySocketPublishUnicastErrStatus(EmberEUI64 eui64,
                                                          uint8_t endpoint,
                                                          EmberStatus status,
                                                          uint16_t msgLen,
                                                          uint8_t *msgBuf);

#endif // SILABS_GATEWAY_RELAY_SOCKET_H
