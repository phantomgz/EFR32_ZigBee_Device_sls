// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#include "af.h"
#include "command-relay-lite.h"
#include "command-relay-lite-local.h"

static void parseDeviceEndpointsFromArguments(EmberAfPluginCommandRelayLiteDeviceEndpoint *inDeviceEndpoint,
                                              EmberAfPluginCommandRelayLiteDeviceEndpoint *outDeviceEndpoint);

void emAfPluginCommandRelayLiteAddCommand(void)
{
  EmberAfPluginCommandRelayLiteDeviceEndpoint inDeviceEndpoint;
  EmberAfPluginCommandRelayLiteDeviceEndpoint outDeviceEndpoint;
  parseDeviceEndpointsFromArguments(&inDeviceEndpoint, &outDeviceEndpoint);

  emberAfPluginCommandRelayLiteAdd(&inDeviceEndpoint, &outDeviceEndpoint);
}

void emAfPluginCommandRelayLiteRemoveCommand(void)
{
  EmberAfPluginCommandRelayLiteDeviceEndpoint inDeviceEndpoint;
  EmberAfPluginCommandRelayLiteDeviceEndpoint outDeviceEndpoint;
  parseDeviceEndpointsFromArguments(&inDeviceEndpoint, &outDeviceEndpoint);

  emberAfPluginCommandRelayLiteRemove(&inDeviceEndpoint, &outDeviceEndpoint);
}

void emAfPluginCommandRelayLiteClearCommand(void)
{
  emberAfPluginCommandRelayLiteClear();
}

void emAfPluginCommandRelayLiteSaveCommand(void)
{
  emberAfPluginCommandRelayLiteSave();
}

void emAfPluginCommandRelayLiteLoadCommand(void)
{
  emberAfPluginCommandRelayLiteLoad();
}

void emAfPluginCommandRelayLitePrintCommand(void)
{
  emAfPluginCommandRelayLitePrint();
}

static void parseDeviceEndpointsFromArguments(EmberAfPluginCommandRelayLiteDeviceEndpoint *inDeviceEndpoint,
                                              EmberAfPluginCommandRelayLiteDeviceEndpoint *outDeviceEndpoint)
{
  emberCopyBigEndianEui64Argument(0, inDeviceEndpoint->eui64);
  inDeviceEndpoint->endpoint = (uint8_t)emberUnsignedCommandArgument(1);
  inDeviceEndpoint->clusterId = (uint16_t)emberUnsignedCommandArgument(2);

  emberCopyBigEndianEui64Argument(3, outDeviceEndpoint->eui64);
  outDeviceEndpoint->endpoint = (uint8_t)emberUnsignedCommandArgument(4);
  outDeviceEndpoint->clusterId = (uint16_t)emberUnsignedCommandArgument(5);
}
