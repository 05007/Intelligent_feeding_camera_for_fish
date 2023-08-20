#ifndef  _RS485_HPP
#define  _RS485_HPP
#include "std_include.hpp"
#include "io_control.hpp"
#include "uart.hpp"
#include <array>
#include <string>

#define T_H_Sensor_ID 0x08

#define Bait_mixer_ID 0x07
#define Bait_feeder_ID 0x04
#define Sedimentation_tank_aerator_ID 0x06
#define Fish_pond_aerator_ID 0x05
#define Exhaust_fan_ID 0x03

enum switch_gate_status{//智能开关闸的状态
    s_close, //闸关
    s_open, //闸开
    fault_V, //电压故障
    fault_A_overload, //电流过载
    fault_A_leakage, //电流漏电
    fault_lock //远程锁扣
};
enum PLC_device_status{
    p_close, //开启
    p_open, //关闭
    p_read  //读取
};
enum PLC_device_id{
    PLC_RESET,  //plc复位

    ALIYUNMODE,
    XUNHUANSHUI,
    PLANTFILTRATION,
    ACULTIVATION,
    SOLUBLEF,
    FPHEATING,
    REHEATING,
    ROOMLIGHT,

    LIGHTING,
    LED11,
    LED12,
    LED13,
    LED21,
    LED22,
    LED23,
    LED31,
    LED32,
    LED33
};
//CRC16-Modbus校验
unsigned short calculateCRC16Modbus(const Rcode* data, size_t length);
//总长8位的命令包
class Operating_pack_8bit{
public:
    Rcode device_address;
    Rcode function_code;
    Rcode coil_address[2];
    Rcode operate[2];
    int pack_length=6;
    //获取CRC16-Modbus校验命令，需要数组长度为pack_length+2
    void getcmd(Rcode* cmd){
        Rcode data[pack_length];  //未校验前命令
        data[0]=device_address;
        data[1]=function_code;
        data[2]=coil_address[0];
        data[3]=coil_address[1];
        data[4]=operate[0];
        data[5]=operate[1];
        // 计算CRC16校验码
        unsigned short crcValue = calculateCRC16Modbus(data,pack_length);
        // 将校验码与原数据合并为8位数组
        //Rcode cmd[8];
        for (size_t i = 0; i < pack_length; ++i) {
            cmd[i] = data[i];
        }
        cmd[pack_length] = static_cast<Rcode>(crcValue & 0x00FF);
        cmd[pack_length + 1] = static_cast<Rcode>((crcValue >> 8) & 0x00FF);
    }
};
class switch_status{    //开关状态
public:
    Rcode device_id; //设备id
    int V_A;  //实时电压 0-600V
    int V_B;
    int V_C;
    int A_Leakage; //实时漏电 0-1000mA
    switch_gate_status gate_status=s_close; //闸状态
    int A_current; //实时电流 0-0xffff*0.01A
    int W_power; //实时功率 0-0xffff*1W
    int kwh_H; //电度计量H 0-0xffff
    int kwh_L;  //电度计量L 0-0xffff*0.001度
    bool updata_sign;   //数据更新标志
    switch_status(Rcode id){
        this->device_id = id;
        this->updata_sign = false;
        this->init();
    }
    void init(){
        if(updata_sign == false){ //如果连续未更新，初始化值
            this->V_A = 0;
            this->V_B = 0;
            this->V_C = 0;
            this->A_Leakage = 0;
            this->gate_status = s_close;
            this->A_current = 0;
            this->W_power = 0;
            this->kwh_H = 0;
            this->kwh_L = 0;
        }
        this->updata_sign = false;
    }
    bool updata_form_RS485(Rcode* rec_buffdata){   //通过485接受的指令更新数据,输入485的缓冲池
        if(device_id != static_cast<int>(rec_buffdata[0])) return false;
        V_A=((static_cast<int>(rec_buffdata[3]) << 8) | static_cast<int>(rec_buffdata[4]));
        V_B=((static_cast<int>(rec_buffdata[5]) << 8) | static_cast<int>(rec_buffdata[6]));
        V_C=((static_cast<int>(rec_buffdata[7]) << 8) | static_cast<int>(rec_buffdata[8]));
        A_Leakage=((static_cast<int>(rec_buffdata[9]) << 8) | static_cast<int>(rec_buffdata[10]));
        switch (rec_buffdata[12])
        {
        case 0xF0:
            gate_status=s_open;
            break;
        case 0xF:
            gate_status=s_close;
            break;
        case 0xA:
            gate_status=fault_V;
            break;
        case 0x3A:
            gate_status=fault_A_overload;
            break;
        case 0x4A:
            gate_status=fault_A_leakage;
            break;
        case 0xB:
            gate_status=fault_lock;
            break;
        default:
            gate_status=s_close;
            break;
        }
        A_current=((static_cast<int>(rec_buffdata[13]) << 8) | static_cast<int>(rec_buffdata[14]));
        W_power=((static_cast<int>(rec_buffdata[15]) << 8) | static_cast<int>(rec_buffdata[16]));
        kwh_H=((static_cast<int>(rec_buffdata[17]) << 8) | static_cast<int>(rec_buffdata[18]));
        kwh_L=((static_cast<int>(rec_buffdata[19]) << 8) | static_cast<int>(rec_buffdata[20]));
        this->updata_sign = true;
        return true;
    }
    void pri(){
        stringstream msg;
        msg <<"ID: "<< std::hex << static_cast<int>(device_id) <<" V: "<< std::dec << V_A <<" A_L: "<<A_Leakage<<" g_s: "<<gate_status<<" A: "<<A_current
            <<" W: "<< W_power <<" k_h: "<< kwh_H <<" k_l "<<kwh_L<<endl;
        msg_log(msg_common2,__func__,msg.str());
    }
};

class temperature_and_humidity_status{  //温湿度传感器状态
public:
    Rcode device_id;
    float temperature; //温度*100
    float humidity;  //湿度*10
    bool updata_sign;   //数据更新标标志，表示这条数据是否被更新过，内部逻辑控制
    temperature_and_humidity_status(Rcode id){
        this->device_id=id;
        this->updata_sign = false;
        this->init();
    }
    void init(){
        if(updata_sign == false){ //如果连续未更新，初始化值
            this->temperature = 0;
            this->humidity = 0;
        }
        this->updata_sign = false;
    }
    bool updata_form_RS485_ALL(Rcode* rec_buffdata){    //通过传感器一块更新温湿度
        if(device_id != static_cast<int>(rec_buffdata[0])) return false;
        int t_temp=((static_cast<int>(rec_buffdata[3]) << 8) | static_cast<int>(rec_buffdata[4]));
        int h_temp=((static_cast<int>(rec_buffdata[5]) << 8) | static_cast<int>(rec_buffdata[6]));
        this->temperature=float(t_temp*10);
        this->humidity=float(h_temp);
        this->updata_sign=true;
    }
    void pri(){
        stringstream msg;
        msg <<"ID: "<< std::hex << static_cast<int>(device_id) <<" T: " << std::dec << temperature <<" H: "<< humidity <<endl;
        msg_log(msg_common2,__func__,msg.str());
    }   
};
void copy_numa_to_numb(Rcode* a,Rcode* b,int length);

//操作PLC的包
class PLC_operate_pack_8bit{
public:
    PLC_device_status oper_type;
    PLC_device_id device_id;
    PLC_operate_pack_8bit(PLC_device_id _id,PLC_device_status _type){
        oper_type = _type;
        device_id = _id;
    }
    void getcmd_ALIYUNMODE(Rcode* cmd1,Rcode* cmd2,Rcode* cmd3){   //打开或关闭上位机模式命令
        //ALIYUNMODE
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x29,0xFF,0x00,0x60,0x0E};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x29,0x00,0x00,0x21,0xFE};
        Rcode cmd_0_open[8] = {0x01,0x05,0xC1,0x01,0xFF,0x00,0xE0,0x06};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x01,0x00,0x00,0xA1,0xF6};
        Rcode cmd_read[8] = {0,0,0,0,0,0,0,0};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_close,cmd2,8);
            copy_numa_to_numb(cmd_0_open,cmd3,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_close,cmd2,8);
            copy_numa_to_numb(cmd_1_open,cmd3,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_XUNHUANSHUI(Rcode* cmd1,Rcode* cmd2){
        //XUNHUANSHUI
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x02,0xFF,0x00,0x10,0x06};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x02,0x00,0x00,0x51,0xF6};
        Rcode cmd_0_open[8] = {0x01,0x05,0xC1,0x03,0xFF,0x00,0x41,0xC6};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x03,0x00,0x00,0x00,0x36};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xCA,0x00,0x02,0xA0,0x09};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_PLANTFILTRATION(Rcode* cmd1,Rcode* cmd2){
        //PLANTFILTRATION
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x04,0xFF,0x00,0xF0,0x07};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x04,0x00,0x00,0xB1,0xF7};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x05,0xFF,0x00,0xA1,0xC7};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x05,0x00,0x00,0xE0,0x37};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xCC,0x00,0x02,0x40,0x08};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_ACULTIVATION(Rcode* cmd1,Rcode* cmd2){
        //ACULTIVATION
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x06,0xFF,0x00,0x51,0xC7};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x06,0x00,0x00,0x10,0x37};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x07,0xFF,0x00,0x00,0x07};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x07,0x00,0x00,0x41,0xF7};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xCE,0x00,0x02,0xE1,0xC8};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_SOLUBLEF(Rcode* cmd1,Rcode* cmd2){
        //SOLUBLEF
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x08,0xFF,0x00,0x30,0x04};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x08,0x00,0x00,0x71,0xF4};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x09,0xFF,0x00,0x61,0xC4};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x09,0x00,0x00,0x20,0x34};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD0,0x00,0x02,0x81,0xCE};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_FPHEATING(Rcode* cmd1,Rcode* cmd2){
        //FPHEATING
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x20,0xFF,0x00,0xB0,0x0C};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x20,0x00,0x00,0xF1,0xFC};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x21,0xFF,0x00,0xE1,0xCC};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x21,0x00,0x00,0xA0,0x3C};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xE8,0x00,0x02,0x00,0x03};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_REHEATING(Rcode* cmd1,Rcode* cmd2){
        //REHEATING
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x22,0xFF,0x00,0x11,0xCC};
        Rcode cmd_1_close[8] = {0x01,0x05,0xC1,0x22,0x00,0x00,0x50,0x3C};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x23,0xFF,0x00,0x40,0x0C};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x23,0x00,0x00,0x01,0xFC};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xEA,0x00,0x02,0xA1,0xC3};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_ROOMLIGHT(Rcode* cmd1,Rcode* cmd2){
        //ROOMLIGHT
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x24,0xFF,0x00,0xF1,0xCD};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x24,0x00,0x00,0xB0,0x3D};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x25,0xFF,0x00,0xA0,0x0D};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x25,0x00,0x00,0xE1,0xFD};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xEC,0x00,0x02,0x41,0xC2};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    
    void getcmd_LIGHTING(Rcode* cmd1,Rcode* cmd2){
        //LIGHTING
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x0A,0xFF,0x00,0x91,0xC4};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x0A,0x00,0x00,0xD0,0x34};
        Rcode cmd_0_open[8] ={0x01,0x05,0xC1,0x0B,0xFF,0x00,0xC0,0x04};
        Rcode cmd_0_close[8] = {0x01,0x05,0xC1,0x0B,0x00,0x00,0x81,0xF4};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD2,0x00,0x02,0x20,0x0E};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
            copy_numa_to_numb(cmd_0_open,cmd2,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_0_close,cmd1,8);
            copy_numa_to_numb(cmd_1_open,cmd2,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED11(Rcode* cmd1){
        //LED11
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x0C,0xFF,0x00,0x71,0xC5};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x0C,0x00,0x00,0x30,0x35};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD4,0x00,0x02,0xC0,0x0F};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED12(Rcode* cmd1){
        //LED12
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x0D,0xFF,0x00,0x20,0x05};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x0D,0x00,0x00,0x61,0xF5};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD5,0x00,0x02,0x91,0xCF};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }

    }
    void getcmd_LED13(Rcode* cmd1){
        //LED13
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x0E,0xFF,0x00,0xD0,0x05};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x0E,0x00,0x00,0x91,0xF5};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD6,0x00,0x02,0x61,0xCF};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }

    void getcmd_LED21(Rcode* cmd1){
        //LED21
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x0F,0xFF,0x00,0x81,0xC5};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x0F,0x00,0x00,0xC0,0x35};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD7,0x00,0x02,0x30,0x0F};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED22(Rcode* cmd1){
        //LED22
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x10,0xFF,0x00,0xB0,0x03};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x10,0x00,0x00,0xF1,0xF3};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD8,0x00,0x02,0x00,0x0C};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED23(Rcode* cmd1){
        //LED23
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x11,0xFF,0x00,0xE1,0xC3};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x11,0x00,0x00,0xA0,0x33};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xD9,0x00,0x02,0x51,0xCC};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }

    void getcmd_LED31(Rcode* cmd1){
        //LED31
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x12,0xFF,0x00,0x11,0xC3};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x12,0x00,0x00,0x50,0x33};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xDA,0x00,0x02,0xA1,0xCC};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED32(Rcode* cmd1){
        //LED32
        Rcode cmd_1_open[8] = {0x01,0x05,0xC1,0x13,0xFF,0x00,0x40,0x03};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x13,0x00,0x00,0x01,0xF3};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xDB,0x00,0x02,0xF0,0x0C};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
    void getcmd_LED33(Rcode* cmd1){
        //LED33
        Rcode cmd_1_open[8] ={0x01,0x05,0xC1,0x14,0xFF,0x00,0xF1,0xC2};
        Rcode cmd_1_close[8] ={0x01,0x05,0xC1,0x14,0x00,0x00,0xB0,0x32};
        Rcode cmd_read[8] = {0x01,0x01,0xC1,0xDC,0x00,0x02,0x41,0xCD};
        if(oper_type==p_close){
            copy_numa_to_numb(cmd_1_close,cmd1,8);
        }else if(oper_type==p_open){
            copy_numa_to_numb(cmd_1_open,cmd1,8);
        }else{
            copy_numa_to_numb(cmd_read,cmd1,8);
        }
    }
};

//开关状态
extern switch_status* Bait_mixer; //搅饵机
extern switch_status* Bait_feeder;  //投饵机
extern switch_status* Sedimentation_tank_aerator; //沉淀池增氧机
extern switch_status* Fish_pond_aerator; //鱼池增氧机
extern switch_status* Exhaust_fan; //排气扇

extern temperature_and_humidity_status* T_H_Sensor;
//plc设备状态


PLC_device_status get_PCL_device_status_byID_fun(PLC_device_id _id);
switch_status* get_switch_status_by_id(Rcode id);
void init_RS485_thread();
void Power_line_carrier_control_thread();
void PLC_control_thread();
void PLC_data_update_thread();

void check_send_pack(int plcC_fd);

void parse_receive_pack_switch(Rcode* rec_buffdata);
void parse_receive_pack_T_H_Sensor(Rcode* rec_buffdata);
void core_PLC_send_read_pack(int PLC_fd, PLC_operate_pack_8bit* send_pack, Rcode *rec_buffdata);
void core_PLC_send_control_pack(int PLC_fd, PLC_operate_pack_8bit* send_pack, Rcode *rec_buffdata); 

void add_command_pack_switch_control(Rcode device_id,switch_gate_status operate);
void add_command_pack_read_switch_info_9(Rcode device_id);
void add_command_pack_read_T_H();
void add_command_pack_PLC_control(PLC_device_id device_id, PLC_device_status oper_type);
#endif