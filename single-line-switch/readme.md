
单火开关基础工程

Base SDK 6.6.3.0

Target Chip: EFR32MG21A010f1024IM32

ZigBee Device Type: End Device Sleepy

Long Poll Interval 400ms
#define EMBER_AF_PLUGIN_END_DEVICE_SUPPORT_LONG_POLL_INTERVAL_SECONDS 4

Short Poll Interval 200ms
#define EMBER_AF_PLUGIN_END_DEVICE_SUPPORT_SHORT_POLL_INTERVAL_SECONDS 2



Endpoint 1 		HA On/Off Output

Stack type		ZigBee 3.0 Security
CLI	list		Core / Application (full)


按键功能定义：
BUTTON 0 按下放开后开始扫描入网
BUTTON 1 让设备持续唤醒，再次按下允许休眠，在唤醒状态下，才可以使用CLI指令访问设备

功耗测试结果
入网后, 低功耗    	 	~740uA （含指示灯）
入网后，持续唤醒   		5.11ma (含指示灯）



bug fix:
1. MICRO define EMBER_AF_API_FIND_AND_BIND_INITIATOR 
2. MICRO define EMBER_AF_API_ZLL_PROFILE
3. emberAfPluginZllCommissioningClientTouchLinkFailedCallback() para error


// API network-steering from Network Steering plugin
#define EMBER_AF_API_NETWORK_STEERING "../../../protocol/zigbee/app/framework/plugin/network-steering/network-steering.h"

// API zll-profile from ZLL Commissioning Common plugin
#define EMBER_AF_API_ZLL_PROFILE "../../../protocol/zigbee/app/framework/plugin/zll-commissioning-common/zll-commissioning.h"


#### 问题反馈

#### 问题提交及讨论

1. 帖子  [http://zigbee.newbitstudio.com/](http://www.newbitstudio.com/forum.php?mod=viewthread&tid=1293&extra=)
