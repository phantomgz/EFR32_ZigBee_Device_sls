// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#ifndef SILABS_GATEWAY_RELAY_SOCKET_COMMANDS_H
#define SILABS_GATEWAY_RELAY_SOCKET_COMMANDS_H

typedef struct {
  uint8_t * commandString;
  void (*functionPtr)(const uint8_t *);
} EmAfPluginGatewayRelaySocketCommand;

#define COMMAND_STRING_START_NETWORK \
  "plugin network-creator start\0"
#define COMMAND_STRING_FORM_NETWORK \
  "plugin network-creator form\0"
#define COMMAND_STRING_OPEN_NETWORK \
  "plugin network-creator-security open-network\0"
#define COMMAND_STRING_CLOSE_NETWORK \
  "plugin network-creator-security close-network\0"
#define COMMAND_STRING_SET_KEY \
  "plugin network-creator-security set-joining-link-key\0"
#define COMMAND_STRING_CLEAR_KEY \
  "plugin network-creator-security clear-joining-link-keys\0"
#define COMMAND_STRING_OPEN_WITH_KEY \
  "plugin network-creator-security open-with-key\0"
#define COMMAND_STRING_ZCL_SEND \
  "plugin device-table send\0"
#define COMMAND_STRING_DEVICE_TABLE_CLEAR \
  "plugin device-table clear\0"
#define COMMAND_STRING_COMMAND_RElAY_ADD \
  "plugin command-relay add\0"
#define COMMAND_STRING_COMMAND_RELAY_REMOVE \
  "plugin command-relay remove\0"
#define COMMAND_STRING_COMMAND_RELAY_CLEAR \
  "plugin command-relay clear\0"
#define COMMAND_STRING_OTA_RELOAD \
  "plugin ota-storage-common reload\0"
#define COMMAND_STRING_OTA_POLICY_QUERY \
  "plugin ota-server policy query\0"
#define COMMAND_STRING_OTA_NOTIFY \
  "plugin ota-server notify\0"
#define COMMAND_STRING_IAS_ZONE_CLEAR \
  "plugin ias-zone-client clear-all\0"
#define COMMAND_STRING_ZDO_LEAVE \
  "zdo leave\0"
#define COMMAND_STRING_ZDO_BIND \
  "zdo bind\0"
#define COMMAND_STRING_BROAD_PJOIN \
  "network broad-pjoin\0"
#define COMMAND_STRING_NETWORK_LEAVE \
  "network leave\0"
#define COMMAND_STRING_NETWORK_FORM \
  "network form\0"
#define COMMAND_STRING_ZCL_GLOBAL_DIRECTION \
  "zcl global direction\0"
#define COMMAND_STRING_ZCL_GLOBAL_READ \
  "zcl global read\0"
#define COMMAND_STRING_ZCL_GLOBAL_WRITE \
  "zcl global write\0"
#define COMMAND_STRING_ZCL_ON_OFF \
  "zcl on-off\0"
#define COMMAND_STRING_ZCL_ON_MOVE_TO_LEVEL \
  "zcl level-control o-mv-to-level\0"
#define COMMAND_STRING_ZCL_MOVE_TO_COLORTEMP \
  "zcl color-control movetocolortemp\0"
#define COMMAND_STRING_ZCL_MOVE_TO_HUESAT \
  "zcl color-control movetohueandsat\0"
#define COMMAND_STRING_ZCL_SEND_ME_A_REPORT \
  "zcl global send-me-a-report\0"
#define COMMAND_STRING_ZCL_GROUP \
  "zcl groups\0"
#define COMMAND_STRING_OPTION_SECURITY_ALLOW_UNSECURE_REJOIN \
  "option security set-allow-trust-center-rejoin-using-well-known-key\0"
#define COMMAND_STRING_OPTION_SECURITY_SET_UNSECURE_REJOIN_TIMEOUT \
  "option security set-allow-trust-center-rejoin-using-well-known-key-timeout\0"

//forward declaration of command functions
void emAfPluginGatewayRelaySocketStartNetwork(const uint8_t *);
void emAfPluginGatewayRelaySocketStartNetworkWithParameters(const uint8_t *);
void emAfPluginGatewayRelaySocketCommandRelayAdd(const uint8_t *);
void emAfPluginGatewayRelaySocketCommandRelayRemove(const uint8_t *);
void emAfPluginGatewayRelaySocketCommandRelayClear(const uint8_t *);
void emAfPluginGatewayRelaySocketZdoLeave(const uint8_t *);
void emAfPluginGatewayRelaySocketZdoBind(const uint8_t *);
void emAfPluginGatewayRelaySocketNetworkBoardPjoin(const uint8_t *);
void emAfPluginGatewayRelaySocketNetworkLeave(const uint8_t *);
void emAfPluginGatewayRelaySocketNetworkForm(const uint8_t *);
void emAfPluginGatewayRelaySocketZclGlobalDirection(const uint8_t *);
void emAfPluginGatewayRelaySocketZclGlobalRead(const uint8_t *);
void emAfPluginGatewayRelaySocketZclGlobalWrite(const uint8_t *);
void emAfPluginGatewayRelaySocketZclOnOff(const uint8_t *);
void emAfPluginGatewayRelaySocketZclOnAndMoveToLevel(const uint8_t *);
void emAfPluginGatewayRelaySocketZclMoveToColorTemp(const uint8_t *);
void emAfPluginGatewayRelaySocketZclMoveToHueAndSat(const uint8_t *);
void emAfPluginGatewayRelaySocketZclGlobalSendMeAReport(const uint8_t *);
void emAfPluginGatewayRelaySocketZclGroup(const uint8_t *);
void emAfPluginGatewayRelaySocketOpenNetwork(const uint8_t *);
void emAfPluginGatewayRelaySocketCloseNetwork(const uint8_t *);
void emAfPluginGatewayRelaySocketZclSend(const uint8_t *);
void emAfPluginGatewayRelaySocketDeviceTableClear(const uint8_t *);
void emAfPluginGatewayRelaySocketSetJoiningKey(const uint8_t *);
void emAfPluginGatewayRelaySocketClearJoiningKeys(const uint8_t *);
void emAfPluginGatewayRelaySocketOpenWithKey(const uint8_t *);
void emAfPluginGatewayRelaySocketOtaStorageCommonReload(const uint8_t *);
void emAfPluginGatewayRelaySocketOtaServerPolicyQuery(const uint8_t *);
void emAfPluginGatewayRelaySocketOtaServerNotify(const uint8_t *);
void emAfPluginGatewayRelaySocketIasZoneClientClearAll(const uint8_t *);
void emAfPluginGatewayRelaySocketAllowRejoin(const uint8_t *);
void emAfPluginGatewayRelaySocketSetRejoinTimeout(const uint8_t *);
void emAfPluginGatewayRelaySocketCommandsProccessing(uint8_t * inputString);

#endif // SILABS_GATEWAY_RELAY_SOCKET_COMMANDS_H