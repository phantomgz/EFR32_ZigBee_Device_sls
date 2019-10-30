// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#ifndef SILABS_COMMAND_RELAY_LITE_LOCAL_H
#define SILABS_COMMAND_RELAY_LITE_LOCAL_H

void emAfPluginCommandRelayLiteAddBind(EmberEUI64 inEui,
                                       uint8_t inEndpoint,
                                       EmberEUI64 outEui,
                                       uint8_t outEndpoint);

void emAfPluginCommandRelayLitePrint(void);
void emAfPluginCommandRelayLiteRemove(EmberAfPluginCommandRelayLiteDeviceEndpoint* inDeviceEndpoint,
                                      EmberAfPluginCommandRelayLiteDeviceEndpoint* outDeviceEndpoint);
void emAfPluginCommandRelayLiteRemoveDeviceByEui64(EmberEUI64 eui64);

#endif //SILABS_COMMAND_RELAY_LITE_LOCAL_H
