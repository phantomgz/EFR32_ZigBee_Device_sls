
零火开关基础工程

Base SDK 6.6.3.0

Endpoint 1 		HA On/Off Output

Stack type		ZigBee 3.0 Security
CLI	list		Core / Application (full)


bug fix:
1. MICRO define EMBER_AF_API_FIND_AND_BIND_INITIATOR 
2. MICRO define EMBER_AF_API_ZLL_PROFILE
3. emberAfPluginZllCommissioningClientTouchLinkFailedCallback() para error


// API network-steering from Network Steering plugin
#define EMBER_AF_API_NETWORK_STEERING "../../../protocol/zigbee/app/framework/plugin/network-steering/network-steering.h"

// API zll-profile from ZLL Commissioning Common plugin
#define EMBER_AF_API_ZLL_PROFILE "../../../protocol/zigbee/app/framework/plugin/zll-commissioning-common/zll-commissioning.h"


