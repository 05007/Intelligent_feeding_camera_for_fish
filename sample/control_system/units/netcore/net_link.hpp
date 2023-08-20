/***************************************************************
 文件名 : net_LINK.hpp
 版本 : v1.0.0
 作者 : 郭辰磊
 单位 : 中国农业大学-国家数字渔业创新中心-数字养殖网箱项目
 描述 : 保持SCA200芯片的网络连接
 其他 : 无
 日志 : 初版v1.0.0 2023/3/20 郭辰磊创建
 ***************************************************************/
#ifndef _NET_LINK_HPP
# define _NET_LINK_HPP
#include "std_include.hpp"
#include "MQTTClient.h"		//MQTT客户端库头文件

#include "uart.hpp"
#include "RS485.hpp"
#include "json/json.h"
//网络连接库
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MQTT_SEND_TIMER_INTERVAL_SECOND 20 //定期发送消息的间隔

/* ########################MQTT##################### */
#define BROKER_ADDRESS	"iot-06z00f71f6hab0w.mqtt.iothub.aliyuncs.com:1883"	// mqttHosturl:port
#define _CLIENTID		"h1te3T2G47v.Test1|securemode=2,signmethod=hmacsha256,timestamp=1690355364345|"		//客户端id
#define _USERNAME		"Test1&h1te3T2G47v"		//用户名
#define _PASSWORD		"3a37004cf019219d378e8fdb414150bd0aed8ac883624ea2dc86feaa4c6ab98c"			//密码

//设备证书
#define _PRODUCTKEY     "h1te3T2G47v"
#define _DEVICENAME     "Test1"
#define _DEVICESECRET   "3c654a375b6685dbeedf29a310e7b9d5"

#define _WILL_TOPIC		"Will Message"		//遗嘱主题
#define _SET_TOPIC		"/sys/h1te3T2G47v/Test1/thing/service/property/set"		//订阅  /sys/.._PRODUCTKEY ../.._DEVICENAME../thing/service/property/set
#define _PUSH_TOPIC		"/sys/h1te3T2G47v/Test1/thing/event/property/post"	//发布

class net_mqtt{
public:
    const char* ClientId;    //客户端id
    const char* username;    //用户名
    const char* password;    //密码
    const char* WillTopic;  //遗嘱主题
    //DeviceSecret
    const char* ProductKey;  
    const char* DeviceName;
    const char* DeviceSecret;
    MQTTClient client;
    net_mqtt(const char* _ClientId,const char* _username,const char* _password,const char* _WillTopic){
        this->ClientId = _ClientId;
        this->username = _username;
        this->password = _password;
        this->WillTopic = _WillTopic;
        this->ProductKey = "";
        this->DeviceName = "";
        this->DeviceSecret = "";
        
        int rc;
        /* 创建mqtt客户端对象 */
        if (MQTTCLIENT_SUCCESS !=
                (rc = MQTTClient_create(&this->client, BROKER_ADDRESS, this->ClientId,
                MQTTCLIENT_PERSISTENCE_NONE, NULL))) {
            msg_log(msg_error,__func__,"Failed to create client, return code: "+to_string(rc));
        }
    }
    net_mqtt(const char* _ClientId,const char* _username,const char* _password,const char* _WillTopic,const char* _ProductKey,const char* _DeviceName,const char* _DeviceSecret){
        this->ClientId = _ClientId;
        this->username = _username;
        this->password = _password;
        this->WillTopic = _WillTopic;
        this->ProductKey = _ProductKey;
        this->DeviceName = _DeviceName;
        this->DeviceSecret = _DeviceSecret;
        int rc;
        /* 创建mqtt客户端对象 */
        if (MQTTCLIENT_SUCCESS !=
                (rc = MQTTClient_create(&this->client, BROKER_ADDRESS, this->ClientId,
                MQTTCLIENT_PERSISTENCE_NONE, NULL))) {
            msg_log(msg_error,__func__,"Failed to create client, return code: "+to_string(rc));
        }
    }

    static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);     //连接回调
    static void connlost(void *context, char *cause);   //连接丢失后调用

    int net_mqtt_link();  //连接
    void net_mqtt_unlink();   //断开连接
    ~net_mqtt(){
        msg_log(msg_tip,__func__,"MQTTClient will destroy.");
        MQTTClient_destroy(&this->client);
    }
};
extern int LINK_SIGN;
extern net_mqtt mqtt;

//检测是否有网络连接
int Check_NetLink();
//通过ECM连接互联网,test_times：最大尝试连接的次数。
int ECM_LINK_NET();
//网络连接初始化
void net_link_init();
//维护网络连接线程
void thread_maintain_net_link();
//API，用来检测当前网络错误是否是由于网络未连接，如果是则尝试重新连接. 返回,TRUE:表示是网络未连接，FALSE：表示不是由于网络未连接
int Detect_network_connection();
//mqtt连接初始化
void net_mqtt_init();
//MQTT线程
void thread_net_mqtt();

//传感器状态包发送处理；
void thread_send_status_timer();
int add_send_mqtt_cmd_pack(string cmd);   //添加一个命令发送包
void TYPE_1_send_sensor_status();
void TYPE_2_send_PLC_status();
void TYPE_3_send_LED_status();
void TYPE_5_send_switch_status();

//解析设置消息
void parse_set_msg(string msg);
#endif