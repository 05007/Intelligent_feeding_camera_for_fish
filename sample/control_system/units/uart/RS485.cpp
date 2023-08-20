#include "RS485.hpp"
#include <map>
queue<Operating_pack_8bit*> switch_control_command;
queue<Operating_pack_8bit*> T_H_sensor_control_command;
queue<PLC_operate_pack_8bit*> PLC_control_command;

switch_status* Bait_mixer = new switch_status(Bait_mixer_ID); //搅饵机
switch_status* Bait_feeder = new switch_status(Bait_feeder_ID);  //投饵机
switch_status* Sedimentation_tank_aerator = new switch_status(Sedimentation_tank_aerator_ID); //沉淀池增氧机
switch_status* Fish_pond_aerator = new switch_status(Fish_pond_aerator_ID); //鱼池增氧机
switch_status* Exhaust_fan = new switch_status(Exhaust_fan_ID); //排气扇

temperature_and_humidity_status* T_H_Sensor= new temperature_and_humidity_status(T_H_Sensor_ID);

std::map<PLC_device_id,PLC_device_status> get_PCL_device_status_byID;
std::map<Rcode,bool> _control_sign; //一个设备是否正在被控制的标志
std::map<PLC_device_id,bool> PLC_control_sign;//一个设备是否正在被控制

void copy_numa_to_numb(Rcode* a,Rcode* b,int length){
    for (int i = 0; i < length; i++) {
        b[i] = a[i];
    }
}
PLC_device_status get_PCL_device_status_byID_fun(PLC_device_id _id){
    return get_PCL_device_status_byID[_id];
}
switch_status* get_switch_status_by_id(Rcode id){
    switch (id)
    {
    case Bait_mixer_ID:
        return Bait_mixer;
        break;
    case Bait_feeder_ID:
        return Bait_feeder;
        break;
    case Sedimentation_tank_aerator_ID:
        return Sedimentation_tank_aerator;
        break;
    case Fish_pond_aerator_ID:
        return Fish_pond_aerator;
        break;
    case Exhaust_fan_ID:
        return Exhaust_fan;
        break;
    default:
        return NULL;
        break;
    }
}
switch_status* get_PLC_device_status_by_id(Rcode id){
    switch (id)
    {
    case Bait_mixer_ID:
        return Bait_mixer;
        break;
    case Bait_feeder_ID:
        return Bait_feeder;
        break;
    case Sedimentation_tank_aerator_ID:
        return Sedimentation_tank_aerator;
        break;
    case Fish_pond_aerator_ID:
        return Fish_pond_aerator;
        break;
    case Exhaust_fan_ID:
        return Exhaust_fan;
        break;
    default:
        return NULL;
        break;
    }
}


unsigned short calculateCRC16Modbus(const Rcode* data, size_t length) {
    unsigned short crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<unsigned short>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else {
                crc >>= 1;
            }
        }
    }

    return crc;
}
void init_RS485_thread(){
    _control_sign[Bait_mixer_ID] = false;
    _control_sign[Bait_feeder_ID] = false;
    _control_sign[Sedimentation_tank_aerator_ID] = false;
    _control_sign[Fish_pond_aerator_ID] = false;
    _control_sign[Exhaust_fan_ID] = false;
    _control_sign[T_H_Sensor_ID] = false;

    PLC_control_sign[PLC_RESET] = false;

    PLC_control_sign[ALIYUNMODE] = false;
    PLC_control_sign[XUNHUANSHUI] = false;
    PLC_control_sign[PLANTFILTRATION] = false;
    PLC_control_sign[ACULTIVATION] = false;
    PLC_control_sign[SOLUBLEF] = false;
    PLC_control_sign[FPHEATING] = false;
    PLC_control_sign[REHEATING] = false;
    PLC_control_sign[ROOMLIGHT] = false;

    PLC_control_sign[LIGHTING] = false;
    PLC_control_sign[LED11] = false;
    PLC_control_sign[LED12] = false;
    PLC_control_sign[LED13] = false;
    PLC_control_sign[LED21] = false;
    PLC_control_sign[LED22] = false;
    PLC_control_sign[LED23] = false;
    PLC_control_sign[LED31] = false;
    PLC_control_sign[LED32] = false;
    PLC_control_sign[LED33] = false;

    //plc设备状态
    get_PCL_device_status_byID[PLC_RESET] = p_close;

    get_PCL_device_status_byID[ALIYUNMODE] = p_close;
    get_PCL_device_status_byID[XUNHUANSHUI] = p_close;
    get_PCL_device_status_byID[PLANTFILTRATION] = p_close;
    get_PCL_device_status_byID[ACULTIVATION] = p_close;
    get_PCL_device_status_byID[SOLUBLEF] = p_close;
    get_PCL_device_status_byID[FPHEATING] = p_close;
    get_PCL_device_status_byID[REHEATING] = p_close;
    get_PCL_device_status_byID[ROOMLIGHT] = p_close;
    
    get_PCL_device_status_byID[LIGHTING] = p_close;
    get_PCL_device_status_byID[LED11] = p_close;
    get_PCL_device_status_byID[LED12] = p_close;
    get_PCL_device_status_byID[LED13] = p_close;
    get_PCL_device_status_byID[LED21] = p_close;
    get_PCL_device_status_byID[LED22] = p_close;
    get_PCL_device_status_byID[LED23] = p_close;
    get_PCL_device_status_byID[LED31] = p_close;
    get_PCL_device_status_byID[LED32] = p_close;
    get_PCL_device_status_byID[LED33] = p_close;
    uart_freshen(uart1,9600,0,8,1,'N');
    sleep(1);
    uart_freshen(uart2,19200,0,8,1,'E');
    sleep(1);
    thread _Power_line_carrier_control_thread(Power_line_carrier_control_thread);  //电力线载波控制线程
    sleep(1);
    thread _PLC_control_thread(PLC_control_thread);
    sleep(1);
    thread _PLC_data_update_thread(PLC_data_update_thread);
    _Power_line_carrier_control_thread.detach();
    _PLC_control_thread.detach();
    _PLC_data_update_thread.detach();
}
//========================控制线程======================================================
void Power_line_carrier_control_thread(){
    msg_log(msg_tip,__func__,"thread open!");
    int data_len = 0;
    Rcode rec_buffdata[DATA_BUFFER_SIZE];//接受缓冲池，最大接受长度为50的数据
    int plcC_fd = opened_uart->get_uart_fd(uart1);
    while(1){
        uart_clear_rec(plcC_fd);
        check_send_pack(plcC_fd);  
        usleep(500);
        data_len = UART_Recv(plcC_fd, rec_buffdata,DATA_BUFFER_SIZE); //接受一次数据
        parse_receive_pack_switch(rec_buffdata); //解析智能开关数据
        parse_receive_pack_T_H_Sensor(rec_buffdata); //解析温湿度传感器数据
	}
}

void PLC_control_thread(){
    msg_log(msg_tip,__func__,"thread open!");
    Rcode rec_buffdata[DATA_BUFFER_SIZE];//接受缓冲池，最大接受长度为50的数据
    int PLC_fd = opened_uart->get_uart_fd(uart2);
    while (1)
    {
        if(!PLC_control_command.empty()){
            uart_clear_rec(PLC_fd);
            PLC_operate_pack_8bit* send_pack=PLC_control_command.front();
            if(send_pack->oper_type == p_read)core_PLC_send_read_pack(PLC_fd,send_pack,rec_buffdata); //读取包处理
            else core_PLC_send_control_pack(PLC_fd,send_pack,rec_buffdata); //处理控制包
            PLC_control_sign[send_pack->device_id] = false ;
            PLC_control_command.pop();
            uart_clear_rec(PLC_fd);
            delete send_pack;   //防止内存泄漏
        }
        uart_clear_rec(PLC_fd);
        usleep(500);
        // UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE); //接受一次数据
        
    }
}
void PLC_data_update_thread(){    //进行持续的数据跟新与读取，取得实时状态
    msg_log(msg_tip,__func__,"thread open!");
    int s_wait = 15;
    int ss=0;
    while (1)
    {
        for(int id=PLC_RESET;id<=LED32;++id){
            add_command_pack_PLC_control(static_cast<PLC_device_id>(id) , p_read);
            sleep(s_wait);
            switch (ss)
            {
            case 0:
                add_command_pack_read_T_H();
                break;
            case 1:
                add_command_pack_read_switch_info_9(Bait_mixer_ID);
                break;
            case 2:
                add_command_pack_read_switch_info_9(Bait_feeder_ID);
                break;
            case 3:
                add_command_pack_read_switch_info_9(Sedimentation_tank_aerator_ID);
                break;
            case 4:
                add_command_pack_read_switch_info_9(Fish_pond_aerator_ID);
                break;
            case 5:
                add_command_pack_read_switch_info_9(Exhaust_fan_ID);
                break;
            case 6:
                add_command_pack_PLC_control(PLC_RESET, p_read);//plc复位
                break;
            default:
                ss = 0;
                break;
            }
            ++ss;
            if(ss >= 7) ss = 0;            
        }
        // add_command_pack_read_T_H();
        // usleep(500);
        // add_command_pack_read_switch_info_9(Bait_mixer_ID);
        // usleep(500);
        // add_command_pack_read_switch_info_9(Bait_feeder_ID);
        // usleep(500);
        // add_command_pack_read_switch_info_9(Sedimentation_tank_aerator_ID);
        // usleep(500);
        // add_command_pack_read_switch_info_9(Fish_pond_aerator_ID);
        // usleep(500);
        // add_command_pack_read_switch_info_9(Exhaust_fan_ID);
    }
}
void core_PLC_send_read_pack(int PLC_fd, PLC_operate_pack_8bit* send_pack, Rcode *rec_buffdata){
    int time,max_times=1;
    Rcode pack_cmd1[8];
    Rcode pack_cmd2[8];
    memset(pack_cmd1,0,8);
    memset(pack_cmd2,0,8);
    PLC_device_id _id = send_pack->device_id;
    int data_len = 0;
    Rcode pack_reset[8]={0x01,0x01,0xC2,0xF5,0x00,0x18,0x11,0x8A}; //PLC复位 
    switch (_id)
    {
    case PLC_RESET://plc复位
        
        UART_Send(PLC_fd,pack_reset,8);
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        break;
    case XUNHUANSHUI:
        send_pack->getcmd_XUNHUANSHUI(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case PLANTFILTRATION:
        send_pack->getcmd_PLANTFILTRATION(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case ACULTIVATION:
        send_pack->getcmd_ACULTIVATION(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case SOLUBLEF:
        send_pack->getcmd_SOLUBLEF(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case FPHEATING:
        send_pack->getcmd_FPHEATING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case REHEATING:
        send_pack->getcmd_REHEATING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case ROOMLIGHT:
        send_pack->getcmd_ROOMLIGHT(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
        
    case LIGHTING:
        send_pack->getcmd_LIGHTING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case LED11:
        send_pack->getcmd_LED11(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case LED12:
        send_pack->getcmd_LED12(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break; 
    case LED13:
        send_pack->getcmd_LED13(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;   
    case LED21:
        send_pack->getcmd_LED21(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED22:
        send_pack->getcmd_LED22(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED23:
        send_pack->getcmd_LED23(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED31:
        send_pack->getcmd_LED31(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED32:
        send_pack->getcmd_LED32(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED33:
        send_pack->getcmd_LED33(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);  
        break;     
    default:
        break;
    }
    time=0;
    do{
        ++time;
        data_len = UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE); //接受一次数据
    }while (data_len <=0 && time < max_times);
    
    if(rec_buffdata[0]==0x01 && rec_buffdata[1]==0x01 && rec_buffdata[2]==0x01 ){
        if(rec_buffdata[3] == 0x01 || rec_buffdata[3] == 0x03)
            get_PCL_device_status_byID[_id] = p_open;
        else get_PCL_device_status_byID[_id] = p_close;
    }
}
void core_PLC_send_control_pack(int PLC_fd,PLC_operate_pack_8bit* send_pack,Rcode* rec_buffdata){
    int time,max_times=1;
    Rcode pack_cmd1[8];
    Rcode pack_cmd2[8];
    Rcode pack_cmd3[8];
    memset(pack_cmd1,0,8);
    memset(pack_cmd2,0,8);
    memset(pack_cmd3,0,8);
    PLC_device_id _id = send_pack->device_id;
    Rcode pack_reset[8]={0x01,0x01,0xC2,0xF5,0x00,0x18,0x11,0x8A}; //PLC复位 
    switch (_id)
    {
    case PLC_RESET://plc复位
        
        UART_Send(PLC_fd,pack_reset,8);
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        break;
    case ALIYUNMODE:
        send_pack->getcmd_ALIYUNMODE(pack_cmd1,pack_cmd2,pack_cmd3);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd3,8);
        break;
    case XUNHUANSHUI:
        send_pack->getcmd_XUNHUANSHUI(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case PLANTFILTRATION:
        send_pack->getcmd_PLANTFILTRATION(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case ACULTIVATION:
        send_pack->getcmd_ACULTIVATION(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case SOLUBLEF:
        send_pack->getcmd_SOLUBLEF(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case FPHEATING:
        send_pack->getcmd_FPHEATING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case REHEATING:
        send_pack->getcmd_REHEATING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case ROOMLIGHT:
        send_pack->getcmd_ROOMLIGHT(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;

    case LIGHTING:
        send_pack->getcmd_LIGHTING(pack_cmd1,pack_cmd2);
        UART_Send(PLC_fd,pack_cmd1,8);
        time=0;
        do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
        UART_Send(PLC_fd,pack_cmd2,8);
        break;
    case LED11:
        send_pack->getcmd_LED11(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;
    case LED12:
        send_pack->getcmd_LED12(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break; 
    case LED13:
        send_pack->getcmd_LED13(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;   
    case LED21:
        send_pack->getcmd_LED21(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED22:
        send_pack->getcmd_LED22(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED23:
        send_pack->getcmd_LED23(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED31:
        send_pack->getcmd_LED31(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED32:
        send_pack->getcmd_LED32(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;  
    case LED33:
        send_pack->getcmd_LED33(pack_cmd1);
        UART_Send(PLC_fd,pack_cmd1,8);
        break;     
    default:
        break;
    }
    time=0;
    do{++time;}while(UART_Recv(PLC_fd, rec_buffdata,DATA_BUFFER_SIZE) <= 0 && time < max_times); //接受一次数据
    get_PCL_device_status_byID[send_pack->device_id] = send_pack->oper_type;    //控制完默认改变状态
}
//=====================================================================================
//检查是否有控制包需要发送
void check_send_pack(int plcC_fd){
    if(!switch_control_command.empty()){
        //如果发送队列有消息,则发送
        Operating_pack_8bit* send_pack=switch_control_command.front();
        Rcode pack_cmd[8];
        memset(pack_cmd,0,8);
        send_pack->getcmd(pack_cmd);
        UART_Send(plcC_fd, pack_cmd,8);
        _control_sign[pack_cmd[0]]=false; //表示控制结束
        switch_control_command.pop();
        delete send_pack;   //删除包，防止内存泄露
    }else if(!T_H_sensor_control_command.empty()){
        //如果发送队列有消息,则发送
        Operating_pack_8bit* send_pack=T_H_sensor_control_command.front();
        Rcode pack_cmd[8];
        memset(pack_cmd,0,8);
        send_pack->getcmd(pack_cmd);
        UART_Send(plcC_fd, pack_cmd,8);
        _control_sign[pack_cmd[0]]=false; //表示控制结束
        T_H_sensor_control_command.pop();
        delete send_pack;   //删除包，防止内存泄露
    }
}
//添加智能开关控制命令，需要设备id(16进制)和需要改变的闸状态
void add_command_pack_switch_control(Rcode device_id,switch_gate_status operate){
    if(_control_sign.find(device_id) ==  _control_sign.end() ) return;//如果设备ID没找到则直接返回
    if(_control_sign[device_id]) return; //如果正在控制则结束，否则设为正在控制设备
    else _control_sign[device_id] = true;
    switch_status* sw=get_switch_status_by_id(device_id);
    if(sw != NULL)
        sw->init();    //读取前先初始化
    sw->gate_status = operate;    //改变状态
    Operating_pack_8bit* sinfo=new Operating_pack_8bit();
	sinfo->device_address=device_id;
	sinfo->function_code=0x05;  //操作线圈功能码0x05
	sinfo->coil_address[0]=0x00;
	sinfo->coil_address[1]=0x01;    //线圈地址
    switch (operate)
    {
    case s_open:
        sinfo->operate[0]=0xFF;
	    sinfo->operate[1]=0x00;
        break;
    case s_close:
        sinfo->operate[0]=0x00;
	    sinfo->operate[1]=0x00;
        break;
    default:    //默认关
        sinfo->operate[0]=0x00;
	    sinfo->operate[1]=0x00;
        break;
    }
    switch_control_command.push(sinfo);
};
//读取智能开关9个寄存器命令，需要设备ID
void add_command_pack_read_switch_info_9(Rcode device_id){
    if(_control_sign.find(device_id) ==  _control_sign.end() ) return;//如果设备ID没找到则直接返回
    if(_control_sign[device_id]) return; //如果正在控制则结束，否则设为正在控制设备
    else _control_sign[device_id] = true;
    switch_status* sw=get_switch_status_by_id(device_id);
    if(sw != NULL)
        sw->init();    //读取前先初始化
    Operating_pack_8bit* sinfo=new Operating_pack_8bit();
	sinfo->device_address=device_id;
	sinfo->function_code=0x04;  //读输入寄存器功能码0x04
	sinfo->coil_address[0]=0x00;
	sinfo->coil_address[1]=0x00;    //寄存器地址
    sinfo->operate[0]=0x00;
	sinfo->operate[1]=0x09;     //读全部的9个寄存器
    switch_control_command.push(sinfo);
};
//读取温湿度传感器
void add_command_pack_read_T_H(){
    Rcode device_id = T_H_Sensor->device_id;
    if(_control_sign.find(device_id) ==  _control_sign.end() ) return;//如果设备ID没找到则直接返回
    if(_control_sign[device_id] ) return; //如果正在控制则结束，否则设为正在控制设备
    else _control_sign[device_id] = true;
    T_H_Sensor->init(); //读取前初始化
    Operating_pack_8bit* sinfo=new Operating_pack_8bit();
	sinfo->device_address=device_id;
	sinfo->function_code=0x04;  //读输入寄存器功能码0x04
	sinfo->coil_address[0]=0x00;
	sinfo->coil_address[1]=0x01;    //寄存器地址
    sinfo->operate[0]=0x00;
	sinfo->operate[1]=0x02;     //读全部的2个寄存器
    T_H_sensor_control_command.push(sinfo);
};
void add_command_pack_PLC_control(PLC_device_id device_id, PLC_device_status oper_type){
    if(PLC_control_sign[device_id]) return;
    else PLC_control_sign[device_id] = true;
    PLC_operate_pack_8bit* send_pack=new PLC_operate_pack_8bit(device_id, oper_type);
    PLC_control_command.push(send_pack);
}

//解析接收到的开关指令包
void parse_receive_pack_switch(Rcode* rec_buffdata){
    if(rec_buffdata[1]==0x04 && rec_buffdata[2]==0x12){ //如果是9寄存器数据，则进行解析
        int data_length=21;  //除循环校验外数据长度
        Rcode data[data_length];
        for (int i = 0; i < data_length; ++i)
        {
            data[i]=rec_buffdata[i];
        }
        unsigned short crcValue = calculateCRC16Modbus(data,data_length);
        if(crcValue==((static_cast<unsigned short>(rec_buffdata[data_length+1]) << 8) | static_cast<unsigned short>(rec_buffdata[data_length]))){   //CRC校验通过
            switch_status* sw=get_switch_status_by_id(data[0]);
            if(sw != NULL)
                sw->updata_form_RS485(data);    //更新对应开关的状态
                sw->pri();
        }     
    }
}
//解析接收到的温湿度传感器指令包
void parse_receive_pack_T_H_Sensor(Rcode* rec_buffdata){
    if(rec_buffdata[0]==0x08 && rec_buffdata[1]==0x04 && rec_buffdata[2]==0x04){
        int data_length=7;  //除循环校验外数据长度
        Rcode data[data_length];
        for (int i = 0; i < data_length; ++i)
        {
            data[i]=rec_buffdata[i];
        }
        unsigned short crcValue = calculateCRC16Modbus(data,data_length);
        if(crcValue==((static_cast<unsigned short>(rec_buffdata[data_length+1]) << 8) | static_cast<unsigned short>(rec_buffdata[data_length]))){   //CRC校验通过
            T_H_Sensor->updata_form_RS485_ALL(data);    //同时更新温湿度
            T_H_Sensor->pri();
        }    
    }
}