#include "net_link.hpp"
int LINK_SIGN=FALSE;//网络连接标志
net_mqtt mqtt(_CLIENTID,_USERNAME,_PASSWORD,_WILL_TOPIC,_PRODUCTKEY,_DEVICENAME,_DEVICESECRET);//mqtt连接
//检测是否有网络连接
int Check_NetLink()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // 初始化OpenSSL库
    SSL_library_init();
    SSL_load_error_strings();

    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        msg_log(msg_error,__func__,"Socket creation error.");
        return FALSE;
    }

    // 设置服务器地址和端口号
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(80);
    servaddr.sin_addr.s_addr = inet_addr("114.114.114.114"); // 用于检测的DNS服务器的IP地址

    // 连接服务器
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        msg_log(msg_error,__func__,"Connection error.");
        return FALSE;
    }
    // 关闭socket
    close(sockfd);

    // 清除OpenSSL错误队列
    ERR_clear_error();

    return TRUE;
}
int fd;
//通过ECM连接互联网,test_times：最大尝试连接的次数。
int ECM_LINK_NET()
{
    LINK_SIGN=TRUE;
    int ret;
    do{
        ret = system("echo -e 'AT+ZECMCALL=1\r\n' > /dev/ttyUSB2");
        if (ret != TRUE) {
            msg_log(msg_error,__func__,"Cmd fail: echo -e 'AT+ZECMCALL=1\r\n' > /dev/ttyUSB2");
        }
        sleep(5);
        ret = system("udhcpc -i usb0 -s /etc/udhcpc.script");
        if (ret != TRUE) {
            msg_log(msg_error,__func__,"Cmd fail: udhcpc -i usb0 -s /etc/udhcpc.script.");
        }
        sleep(5);
    }while(ret != TRUE);
    while(Check_NetLink()==FALSE);//连接网络直到测试通过
    // ret = system("nohup ffmpeg -video_size 1200x720 -framerate 10 -i /dev/video1 -q 10 -f flv rtmp://47.94.253.117/live/mytest > /dev/null 2>&1 &");    //连接视频
    // if (ret != TRUE) {
    //     msg_log(msg_error,__func__,"Cmd fail: ffmpeg.");
    // }
    // msg_log(msg_tip,__func__,"open ffmpeg.");
    LINK_SIGN=FALSE;
    return TRUE;
}
//网络连接初始化
void net_link_init(){
    thread _thread_maintain_net_link(thread_maintain_net_link);//开启维护网络连接线程
    _thread_maintain_net_link.detach();
    thread _thread_send_status_timer(thread_send_status_timer); //定时给云平台发送信息的定时器
    _thread_send_status_timer.detach();
}
//维护网络连接线程
void thread_maintain_net_link(){
    msg_log(msg_common,__func__,"Start Link NET.");
    if(ECM_LINK_NET()==TRUE)
        msg_log(msg_tip,__func__,"NET Connection Success.");
    int check_times=1;    
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::minutes(10));  //10分钟检测一次网络连接
        if(check_times >= 60){  //每十小时强制重新连接
            msg_log(msg_common,__func__,"Start Link NET.");
            if(ECM_LINK_NET()==TRUE)
                msg_log(msg_tip,__func__,"NET Connection Success.");
            check_times=1;
        }else{
            Detect_network_connection();
            ++check_times;
        }
    }
}
void thread_send_status_timer(){
    sleep(60);
    while(1){
        if(Check_NetLink()==TRUE){
            sleep(20);
            TYPE_1_send_sensor_status();
            sleep(1);
            TYPE_2_send_PLC_status();
            sleep(1);
            TYPE_3_send_LED_status();
            sleep(1);
            TYPE_5_send_switch_status();
        }
        sleep(MQTT_SEND_TIMER_INTERVAL_SECOND);
    }
}
int add_send_mqtt_cmd_pack(string cmd){
        int rc;
        MQTTClient_message tempmsg = MQTTClient_message_initializer;
        char* temp_str=const_cast<char*>(cmd.c_str());  //const char* to char*
		/* 发布温度信息 */
		tempmsg.payload = temp_str;	//消息的内容
		tempmsg.payloadlen = strlen(temp_str);		//内容的长度
		tempmsg.qos = 0;				//QoS等级
		tempmsg.retained = 1;		//保留消息
		if (MQTTCLIENT_SUCCESS != (rc = MQTTClient_publishMessage(mqtt.client, _PUSH_TOPIC, &tempmsg, NULL))) {
            msg_log(msg_error,__func__,"Failed to publish message, return code: "+to_string(rc));
            return FALSE;
		}
        msg_log(msg_common,__func__,"Mqtt send: "+cmd);
        return TRUE;

}
void TYPE_1_send_sensor_status(){
    // Json::StreamWriterBuilder writer;
    Json::FastWriter writer;    //无格式写入

    Json::Value res_params;
    Json::Value sensor_status;

    sensor_status["TYPE"] = 1;
    sensor_status["RTEMP"] = T_H_Sensor->temperature; //室温
    sensor_status["PH"] = 0;    //PH
    sensor_status["WATERT"] = 0;    //水温
    sensor_status["OXYGEN"] = 0;    //溶解氧
    sensor_status["EC"] = 0;    //电导率
    sensor_status["SALINITY"] = 0.0; //盐度
    sensor_status["TURBIDITY"] = 0; //浊度
    sensor_status["RHUMIDITY"] = T_H_Sensor->humidity;   //湿度

    res_params["params"]=sensor_status;
    
    // add_send_mqtt_cmd_pack(Json::writeString(writer, res_params));
    add_send_mqtt_cmd_pack(writer.write(res_params));
    // {params:{TYPE:1,RTEMP:1000.000000,PH:6,WATERT:10,OXYGEN:5,EC:10,SALINITY:0.000000,TURBIDITY:5,RHUMIDITY:0}}
}
void TYPE_2_send_PLC_status(){
    // Json::StreamWriterBuilder writer;
    switch_status* SS1=get_switch_status_by_id(Bait_mixer_ID);
    switch_status* SS2=get_switch_status_by_id(Sedimentation_tank_aerator_ID);
    switch_status* SS3=get_switch_status_by_id(Fish_pond_aerator_ID);
    switch_status* SS4=get_switch_status_by_id(Bait_feeder_ID);
    switch_status* SS5=get_switch_status_by_id(Exhaust_fan_ID);

    Json::FastWriter writer;    //无格式写入

    Json::Value res_params;
    Json::Value PLC_status;

    PLC_status["TYPE"] = 2;
    PLC_status["LIGHTING"] = static_cast<int>(get_PCL_device_status_byID_fun(LIGHTING));
    PLC_status["ALIYUNMODE"] = static_cast<int>(get_PCL_device_status_byID_fun(ALIYUNMODE));
    PLC_status["XUNHUANSHUI"] = static_cast<int>(get_PCL_device_status_byID_fun(XUNHUANSHUI));
    PLC_status["PLANTFILTRATION"] = static_cast<int>(get_PCL_device_status_byID_fun(PLANTFILTRATION));
    PLC_status["ACULTIVATION"] = static_cast<int>(get_PCL_device_status_byID_fun(ACULTIVATION));
    PLC_status["SOLUBLEF"] = static_cast<int>(get_PCL_device_status_byID_fun(SOLUBLEF));
    PLC_status["FPHEATING"] = static_cast<int>(get_PCL_device_status_byID_fun(FPHEATING));
    PLC_status["REHEATING"] = static_cast<int>(get_PCL_device_status_byID_fun(REHEATING));
    PLC_status["ROOMLIGHT"] = static_cast<int>(get_PCL_device_status_byID_fun(ROOMLIGHT));
    PLC_status["SS1STATE"] = static_cast<int>(SS1->gate_status);
    PLC_status["SS2STATE"] = static_cast<int>(SS2->gate_status);
    PLC_status["SS3STATE"] = static_cast<int>(SS3->gate_status);
    PLC_status["SS4STATE"] = static_cast<int>(SS4->gate_status);;
    PLC_status["SS5STATE"] = static_cast<int>(SS5->gate_status);;
    res_params["params"]=PLC_status;
    
    // add_send_mqtt_cmd_pack(Json::writeString(writer, res_params));
    add_send_mqtt_cmd_pack(writer.write(res_params));
    // {params:{TYPE:2,LIGHTING:1,ALIYUNMODE:0,XUNHUANSHUI:0,PLANTFILTRATION:0,ACULTIVATION:1,SOLUBLEF:0,FPHEATING:0,REHEATING:0,ROOMLIGHT:1,SS1STATE:0,SS2STATE:0,SS3STATE:0,SS4STATE:0,SS5STATE:240}}
}
void TYPE_3_send_LED_status(){
    // Json::StreamWriterBuilder writer;
    Json::FastWriter writer;    //无格式写入

    Json::Value res_params;
    Json::Value sensor_status;

    sensor_status["TYPE"] = 3;
    sensor_status["LED11"] = static_cast<int>(get_PCL_device_status_byID_fun(LED11));
    sensor_status["LED12"] = static_cast<int>(get_PCL_device_status_byID_fun(LED12));
    sensor_status["LED13"] = static_cast<int>(get_PCL_device_status_byID_fun(LED13));
    sensor_status["LED21"] = static_cast<int>(get_PCL_device_status_byID_fun(LED21));
    sensor_status["LED22"] = static_cast<int>(get_PCL_device_status_byID_fun(LED22));
    sensor_status["LED23"] = static_cast<int>(get_PCL_device_status_byID_fun(LED23));
    sensor_status["LED31"] = static_cast<int>(get_PCL_device_status_byID_fun(LED31));
    sensor_status["LED32"] = static_cast<int>(get_PCL_device_status_byID_fun(LED32));
    sensor_status["LED33"] = static_cast<int>(get_PCL_device_status_byID_fun(LED33));

    res_params["params"]=sensor_status;
    
    // add_send_mqtt_cmd_pack(Json::writeString(writer, res_params));
    add_send_mqtt_cmd_pack(writer.write(res_params));
    // {params:{TYPE:3,LED11:1,LED12:1,LED13:1,LED21:1,LED22:1,LED23:1,LED31:1,LED32:0,LED133:1}}
}
void TYPE_5_send_switch_status(){
    Json::FastWriter writer;    //无格式写入
    // Json::StreamWriterBuilder writer;
    switch_status* SS1=get_switch_status_by_id(Bait_mixer_ID);
    switch_status* SS2=get_switch_status_by_id(Sedimentation_tank_aerator_ID);
    switch_status* SS3=get_switch_status_by_id(Fish_pond_aerator_ID);
    switch_status* SS4=get_switch_status_by_id(Bait_feeder_ID);
    switch_status* SS5=get_switch_status_by_id(Exhaust_fan_ID);

    Json::Value res_params;
    Json::Value _switch_status;
    _switch_status["TYPE"] = 5;

    _switch_status["SS1V1"] = SS1->V_A;
    _switch_status["SS1A1"] = SS1->A_current;
    _switch_status["SS1W1"] = SS1->W_power;
    _switch_status["SS1U1"] = SS1->kwh_H*1000+SS1->kwh_L;

    _switch_status["SS2V2"] = SS2->V_A;
    _switch_status["SS2A2"] = SS2->A_current;
    _switch_status["SS2W2"] = SS2->W_power;
    _switch_status["SS2U2"] = SS2->kwh_H*1000+SS2->kwh_L;

    _switch_status["SS3V3"] = SS3->V_A;
    _switch_status["SS3A3"] = SS3->A_current;
    _switch_status["SS3W3"] = SS3->W_power;
    _switch_status["SS3U3"] = SS3->kwh_H*1000+SS3->kwh_L;

    _switch_status["SS4V4"] = SS4->V_A;
    _switch_status["SS4A4"] = SS4->A_current;
    _switch_status["SS4W4"] = SS4->W_power;
    _switch_status["SS4U4"] = SS4->kwh_H*1000+SS4->kwh_L;

    _switch_status["SS5V5"] = SS5->V_A;
    _switch_status["SS5A5"] = SS5->A_current;
    _switch_status["SS5W5"] = SS5->W_power;
    _switch_status["SS5U5"] = SS5->kwh_H*1000+SS5->kwh_L;
   
    res_params["params"]=_switch_status;
    // add_send_mqtt_cmd_pack(Json::writeString(writer, res_params));
    add_send_mqtt_cmd_pack(writer.write(res_params));
    //{params:{TYPE:5,SS1V1:227,SS1A1:0,SS2V2:227,SS2A2:0,SS3V3:227,SS3A3:0,SS4V4:227,SS4A4:0,SS5V5:227,SS5A5:0,SS1W1:0,SS2W2:0,SS3W3:0,SS4W4:0,SS5W5:0,,SS1U1:1960,SS2U2:45783,SS3U3:45783,SS4U4:749,SS5U5:29990}}
}

//注，此函数会阻塞线程，用来检测当前网络错误是否是由于网络未连接，如果是则尝试重新连接. 返回,TRUE:表示是网络未连接，FALSE：表示不是由于网络未连接
int Detect_network_connection(){
    if(Check_NetLink()==FALSE){
        if(LINK_SIGN==FALSE){
            msg_log(msg_warn,__func__,"Detect network connection errors, attempting to reconnect.");
            ECM_LINK_NET();
        }
        return TRUE;
    }else{
        //msg_log(msg_tip,__func__,"Network connected.");
        return FALSE;
    }
    
}
//===mqtt
int net_mqtt::msgarrvd(void *context, char *topicName, int topicLen,
			MQTTClient_message *message)
{
    if(string(topicName)==_SET_TOPIC){
        string msg=string((char *)(message->payload));
        parse_set_msg(msg); //解析mqtt发来的消息
    }
    
    // std::stringstream ss;
    // ss << "topicName: " << topicName;
    // msg_log(msg_common, __func__, ss.str());

    // ss.str(""); // Clear the stringstream for reusing
    // ss << "message->payload: " << static_cast<char*>(message->payload);
    // msg_log(msg_common, __func__, ss.str());
    // uart_log(uart1,"topicName: "+string(topicName)+"; message->payload: "+string((char *)(message->payload)));
	/* 释放占用的内存空间 */
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	/* 退出 */
	return 1;
}

void net_mqtt::connlost(void *context, char *cause)
{
    msg_log(msg_error,__func__,"Connection lost");
    // msg_log(msg_common,__func__,"Try connecting to the network again");
	// Detect_network_connection();
}

int net_mqtt::net_mqtt_link()
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc;
	/* 设置回调 */
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_setCallbacks(this->client, NULL, connlost,
			msgarrvd, NULL))) {
        msg_log(msg_error,__func__,"Failed to set callbacks, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
		goto exit;
        //return rc;
	}

	/* 连接MQTT服务器 */
	will_opts.topicName = WillTopic;	//遗嘱主题
	will_opts.message = "Unexpected disconnection";//遗嘱消息
	will_opts.retained = 1;	//保留消息
	will_opts.qos = 0;		//QoS0

	conn_opts.will = &will_opts;
	conn_opts.keepAliveInterval = 30;	//心跳包间隔时间
	conn_opts.cleansession = 0;			//cleanSession标志
	conn_opts.username = username;		//用户名
	conn_opts.password = password;		//密码
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_connect(this->client, &conn_opts))) {
        msg_log(msg_error,__func__,"Failed to connect, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
		goto exit;
        //return rc;
	}
    msg_log(msg_tip,__func__,"MQTT LINK!");

	/* 发布上线消息 */
	pubmsg.payload = static_cast<void*>(const_cast<char*>("Online"));	//消息的内容
	pubmsg.payloadlen = 6;		//内容的长度
	pubmsg.qos = 0;				//QoS等级
	pubmsg.retained = 1;		//保留消息
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_publishMessage(this->client, WillTopic, &pubmsg, NULL))) {
        msg_log(msg_error,__func__,"Failed to publish message, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
		goto disconnect_exit;
        //return rc;
	}

	/* 订阅主题 dt_mqtt/led */
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_subscribe(this->client, _SET_TOPIC, 0))) {
        msg_log(msg_error,__func__,"Failed to subscribe, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
		goto unsubscribe_exit;
        //return rc;
	}
	return rc;

unsubscribe_exit:
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_unsubscribe(this->client, _SET_TOPIC))) {
        msg_log(msg_error,__func__,"Failed to unsubscribe, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
	}
disconnect_exit:
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_disconnect(this->client, 10000))) {
        msg_log(msg_error,__func__,"Failed to disconnect, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
	}
exit:
	return rc;
}
void net_mqtt::net_mqtt_unlink(){
    int rc;
    if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_unsubscribe(this->client, _SET_TOPIC))) {
        msg_log(msg_error,__func__,"Failed to unsubscribe, return code: "+to_string(rc));
		rc = EXIT_FAILURE;
	}
	if (MQTTCLIENT_SUCCESS != (rc = MQTTClient_disconnect(this->client, 10000))) 
        msg_log(msg_error,__func__,"Failed to disconnect, return code: "+to_string(rc));
	//MQTTClient_destroy(&this->client);
}
void net_mqtt_init(){
    thread _thread_net_mqtt(thread_net_mqtt);//开启维护网络连接线程
    _thread_net_mqtt.detach();
}
void thread_net_mqtt(){
    
    int rc;//,len,rec_check;
    float watert=0;
    // char rec[9]={0,0,0,0,0,0,0,0,0};
    while(Detect_network_connection()==TRUE){
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }  //等待至联网完成
    do{
        rc = mqtt.net_mqtt_link();
        if(MQTTCLIENT_SUCCESS != rc){
            msg_log(msg_error,__func__,"Failed to net_mqtt_link, will try connecting again.");
            std::this_thread::sleep_for(std::chrono::minutes(1));   //线程休眠1分钟
        }
    }while(MQTTCLIENT_SUCCESS != rc);
	while (true) {
        if(Detect_network_connection()==TRUE){  //发送前检测连接状态，如果网络未连接则无限等待
            do{
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }while(Detect_network_connection()==TRUE);  //等待至联网完成
            //mqtt.net_mqtt_unlink();
            do{
                rc = mqtt.net_mqtt_link();
                if(MQTTCLIENT_SUCCESS != rc){
                    msg_log(msg_error,__func__,"Failed to net_mqtt_link,will try connecting again.");
                    std::this_thread::sleep_for(std::chrono::minutes(1));   //线程休眠1分钟
                }
            }while(MQTTCLIENT_SUCCESS != rc);
        }
        // uart_log(uart1,"Mqtt send: "+temp_string);
        sleep(30);		//每隔30秒 更新一次数据
	}
}


//解析设置消息
void parse_set_msg(string msg){
    Json::Reader reader;
    Json::Value jsonData;
    cout<<msg<<endl;
    // Check if parsing was successful
    if (reader.parse(msg, jsonData)) {
        if(jsonData.isMember("params"))
        {
            if (jsonData["params"].isObject()){
                Json::Value json_params=jsonData["params"];
                for (Json::Value::iterator it = json_params.begin(); it != json_params.end(); ++it) {
                    string key= it.key().asString();
                    switch_gate_status s_oper;
                    PLC_device_status p_oper;
                    if(json_params[key].asInt()==1){
                        s_oper = s_open;
                        p_oper = p_open;
                    }else{
                        s_oper = s_close;
                        p_oper = p_close;
                    }
                    if(key=="SS1"){//搅饵机
                        add_command_pack_switch_control(Bait_mixer_ID,s_oper);
                    }else if(key=="SS2"){//蓄水池增氧机
                        add_command_pack_switch_control(Sedimentation_tank_aerator_ID,s_oper);
                    }else if(key=="SS3"){//鱼池增氧机
                        add_command_pack_switch_control(Fish_pond_aerator_ID,s_oper);
                    }else if(key=="SS4"){//投饵机
                        add_command_pack_switch_control(Bait_feeder_ID,s_oper);
                    }else if(key=="SS5"){//排气扇
                        add_command_pack_switch_control(Exhaust_fan_ID,s_oper);                        
                    }else if(key=="ALIYUNMODE"){
                        add_command_pack_PLC_control(ALIYUNMODE,p_oper);
                    }else if(key=="LIGHTING"){
                        add_command_pack_PLC_control(LIGHTING,p_oper);
                    }else if(key=="LED11"){
                        add_command_pack_PLC_control(LED11,p_oper);
                    }else if(key=="LED12"){
                        add_command_pack_PLC_control(LED12,p_oper);
                    }else if(key=="LED13"){
                        add_command_pack_PLC_control(LED13,p_oper);
                    }else if(key=="LED21"){
                        add_command_pack_PLC_control(LED21,p_oper);
                    }else if(key=="LED22"){
                        add_command_pack_PLC_control(LED22,p_oper);
                    }else if(key=="LED23"){
                        add_command_pack_PLC_control(LED23,p_oper);
                    }else if(key=="LED31"){
                        add_command_pack_PLC_control(LED31,p_oper);
                    }else if(key=="LED32"){
                        add_command_pack_PLC_control(LED32,p_oper);
                    }else if(key=="LED33"){
                        add_command_pack_PLC_control(LED33,p_oper);
                    }else if(key=="XUNHUANSHUI"){
                        add_command_pack_PLC_control(XUNHUANSHUI,p_oper);
                    }else if(key=="PLANTFILTRATION"){
                        add_command_pack_PLC_control(PLANTFILTRATION,p_oper);
                    }else if(key=="SOLUBLEF"){
                        add_command_pack_PLC_control(SOLUBLEF,p_oper);
                    }else if(key=="ACULTIVATION"){
                        add_command_pack_PLC_control(ACULTIVATION,p_oper);
                    }else if(key=="ROOMLIGHT"){
                        add_command_pack_PLC_control(ROOMLIGHT,p_oper);
                    }else if(key=="FPHEATING"){
                        add_command_pack_PLC_control(FPHEATING,p_oper);
                    }else if(key=="REHEATING"){
                        add_command_pack_PLC_control(REHEATING,p_oper);
                    }
                }
            }
        }
    }
}
