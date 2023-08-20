/***************************************************************
 文件名 : control_system_main.cpp
 版本 : v1.0.0
 作者 : 郭辰磊
 单位 : 中国农业大学-国家数字渔业创新中心-数字养殖网箱项目
 描述 : 网箱智能控制系统主程序
 其他 : 无
 日志 : 初版v1.0.0 2023/3/10 郭辰磊创建
 ***************************************************************/
#include "std_include.hpp"
#include "json/json.h"  //jsoncpp
#include "sc_npu.h"     //npu
#include "opencv2/opencv.hpp"
#include "net_link.hpp"
#include "io_control.hpp"
#include "run_solo.hpp"
#include "RS485.hpp"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
// #include "sc_type.h"
// #include "sample_comm_svp.h"
// #include "sample_comm_npu.h4
// #include "sample_npu_main.h"
// #include "sample_svp_npu_software.h"

using namespace cv;

// void usend(){
//     while(1){
// 		// Rcode pack_reDATA_BUFFER_SIZEset[8]={0x01,0x01,0xC2,0xF5,0x00,0x18,0x11,0x8A};
// 		Rcode rcv_buff[DATA_BUFFER_SIZE];

// 		int len = UART_Recv(opened_uart->get_uart_fd(uart1),rcv_buff, DATA_BUFFER_SIZE);
// 		if(len > 0)
// 			UART_Send(opened_uart->get_uart_fd(uart1),rcv_buff,len);
// 		sleep(3);
// 	}
// }

int main(int argc, char *argv[])
{
	
	net_link_init();    //连接网络
	msg_log(msg_common,__func__,"Net Link!");
	net_mqtt_init();	//初始化mqtt网络连接
	init_RS485_thread();
	msg_log(msg_common,__func__,"Netcage control system start V1.0.0-beta.15");
	// msg_log(Rcode pack_reset[8]={0x01,0x05,0xC1,0x13,0x00,0x00,0x01,0xF3};msg_common,__func__,"start netcage_control_system! V 1.2.1=99");

	// cout<<"uart test V1.0.0=50"<<endl;
	// uart_freshen(uart1,19200,0,8,1,'E');
	// sleep(1);
	
	// Rcode pack_reset_2[8]={0x01,0x01,0xC2,0xF5,0x00,0x18,0x11,0x8A};
	// UART_Send(opened_uart->get_uart_fd(uart2),pack_reset,8);
	// cout<<"rec_4"<<endl;
	// thread _u_send(usend);
	// thread _u_send_2(usend_2);
	// _u_send.detach();
	while(1){
		sleep(1000);
	}
	// Rcode rec_buffdata[DATA_BUFFER_SIZE];//接受缓冲池，最大接受长度为50的数据

	// _u_send_2.detach();
	// sleep(8);
	// while(1){
	// 	// UART_Recv(opened_uart->get_uart_fd(uart1),rec_buffdata,DATA_BUFFER_SIZE);
	// 	// add_command_pack_read_switch_info_9(0x03);
	// 	sleep(8);
	// };
	// SC_CHAR *pcSrcFile ="./images/1.jpg";
	// if(access(pcSrcFile, F_OK) != 0)
    // {
    //     cout<<"Error,pcSrcFile not exist!\n"<<endl;

    // }

    // if(access(pcModelName, F_OK) != 0)
    // {
	// 	cout<<"Error,pcModelName not exist !\n"<<endl;
    // }
	// run_solo_4(pcModelName, pcSrcFile);
	// SC_CHAR *pcSrcFile ="./images/1.jpg";
    // SC_CHAR *pcModelName = "./bin_hex/count.npubin";
	// msg_log(msg_tip,__func__,"Control_system start!");
	// if(open_uart(uart1,115200,0,8,1,'N')==FALSE){//打开串口1
    //     msg_log(msg_error,__func__,"uart_"+to_string(uart1)+" is not open.");
    // }
	// uart_log(uart1,"Control_system start!");
	// uart_log(uart1,"First, test the algorithm using images.");
	// // SC_CHAR *pcModelName = "./bin_hex/yolov3_caffe_yuv420p.npubin";
	// if(access(pcSrcFile, F_OK) != 0)
    // {
    //     cout<<"Error,pcSrcFile not exist!\n"<<endl;
    // }

    // if(access(pcModelName, F_OK) != 0)
    // {
	// 	cout<<"Error,pcModelName not exist !\n"<<endl;
    // }
	// run_solo(pcModelName, pcSrcFile);//图像测试
	// uart_log(uart1,"Please wait NET init!");
	// sleep(10);	//开启等待10s
	// net_link_init();    //连接网络
	// uart_log(uart1,"Net Link!");
	// net_mqtt_init();	//初始化mqtt网络连接
	// uart_log(uart1,"MQTT Link!");

	// run_solo_5(pcModelName, pcSrcFile);
	// close_uart(uart1);
	return 0;
}


	// cout<<"test2"<<endl;
    // // 打开默认的摄像头（设备编号为 0）
    // cv::VideoCapture cap(0);

    // if (!cap.isOpened()) {
    //     std::cerr << "Failed to open camera!" << std::endl;
    //     return -1;
    // }
 

	// // 从摄像头读取一帧数据
	// cv::Mat frame;
	// cap >> frame;
	// cout<<frame<<endl;
	// sleep(1);
    
 
    // // 释放资源
    // cap.release();

	// imwrite("./test.jpg", frame);
    // return 0;

	// net_link_init();    //连接网络
	// net_mqtt_init();	//初始化mqtt网络连接
	// // msg_log(msg_common,__func__,"test");
	// // Json::Value va;
 	// // va["cpp"]="!!!Hello World!!!";
	// // msg_log(msg_common,__func__,va["cpp"].asString());
 	// // float a[4] = {1,2,3,4};
    // // //将数组转换为矩阵,2行2列的矩阵
 	// // Mat a_mat = Mat(2,2,CV_32F,a);
	// // msg_log(msg_mat,__func__,a_mat);
 	// // cout<<a_mat<<endl;
 	// // Mat kernel = Mat::ones(5, 5, CV_32F) / 25;//5*5平均模板
 	// // cout<<kernel<<endl;
	// // msg_log(msg_common,__func__,"testend");
	// while(1){
	// 	cout<<"test0_v1.0.0"<<endl;
	// 	std::this_thread::sleep_for(std::chrono::hours(1));//线程休眠一个小时
	// }	
// }
// #include "uart.hpp"
// #include <iostream>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/ioctl.h>
// using namespace std;
// #define BAUDRATE B115200
// #define _POSIX_SOURCE 1
// //==========================================
// //多线程控制
// //==========================================
// int send_time=20;
// bool Send_sign=true;
// void theard_uart_send(int fd){
// 	this_thread::sleep_for(std::chrono::seconds(1));
// 	while(1){
// 		if(Send_sign){
// 			char cmd_[8]={0x00,0x04,0x00,0x01,0x00,0x02,0x20,0x0B};
// 			cout<<"sendbbool:"<<UART_Send(fd,cmd_,8)<<endl;
// 			cout<<endl<<BOLDYELLOW<<"[Send]Commend To SHT20,"<<"the next time in "<<send_time<<" seconds.";
// 			cout<<endl<<GREEN<<endl;
// 		}
// 		this_thread::sleep_for(std::chrono::seconds(send_time));

// 	}
// }
// void theard_uart_cmd(){
// 	string cmd;
// 	while(1){
// 		cout<<GREEN<<"Enter [help] to get the command"<<endl;
// 		cin>>cmd;
// 		if(cmd=="help"){
// 			cout<<"-stop: Stop Program"<<endl;
// 			cout<<"-set: Set parameters"<<endl;
// 		}else if(cmd=="stop"){
// 			cout<<RED<<"Program stop"<<RESET<<endl;
// 			exit(0);
// 		}else if(cmd=="set"){
// 			cout<<"Please enter the parameters to be set:"<<endl;
// 			cout<<"-sendtime: Set Sampling Interval."<<endl;
// 			cout<<"-stopsend: Stop Sensor Sampling."<<endl;
// 			cout<<"-opensend: Start Sensor Sampling."<<endl;
// 			string cmd_set;
// 			cin>>cmd_set;
// 			if(cmd_set=="sendtime"){
// 				cout<<"Please enter the set time."<<endl;
// 				cin >> send_time;
// 				cout<<"Sendtime is "<<send_time<<endl;
// 			}else if(cmd_set=="stopsend"){
// 				Send_sign=false;
// 				cout<<"Sensor is stop!"<<endl;
// 			}else if(cmd_set=="opensend"){
// 				Send_sign=true;
// 				cout<<"Sensor is start!"<<endl;
// 			}
// 		}else{
// 			cout<<"command: "<<cmd<<" is Unknown."<<endl;
// 		}
// 	}

// }
// void theard_uart_rec(int fd){
//     char rec[DATA_BUFFER_SIZE];
// 	while(1){
// 		//char rec[9]={0,0,0,0,0,0,0,0,0};
// 		//read(fd,rec,9);
//         UART_Recv(fd,rec,DATA_BUFFER_SIZE);
//         cout<<"rec"<<endl;
// 		if(rec[0]==0x01){
// 			cout<<endl<<BLUE<<"[Rec]Message From SHT20"<<endl;
// 			if(rec[1]==0x04){
// 				time_t now = time(0); // 把 now 转换为字符串形式
// 				char* dt = ctime(&now);
// 				cout << "[Time]" << dt;
// 				float temperature=(rec[3]<<8)+(rec[4]);
// 				cout<<"[Current temperature] "<<(float)temperature/10<<"℃"<<endl;
// 				float humidity=(rec[5]<<8)+(rec[6]);
// 				cout<<"[Current humidity] "<<(float)humidity/10<<"%";
//                 for(char i:rec)
// 				    cout<<i<<" ";
// 			}
// 			cout<<endl<<GREEN<<endl;
// 		}else{
// 			for(char i:rec)
// 				cout<<i<<" ";
//             cout<<endl;
// 		}
// 	}
// };
// int main() {
// 	int fd=open_port(1);
// 	if(UART_Set(fd,9600,0,8,1,'N')== TRUE)
// 	{;
// 		thread thread1(theard_uart_rec,fd);
// 		thread thread2(theard_uart_send,fd);
// 		thread thread3(theard_uart_cmd);
// 		thread1.detach();
// 		thread2.detach();
// 		thread3.detach();
// 		while(1){
// 			this_thread::sleep_for(std::chrono::seconds(100));
// 		}

// 	}
// 	else
// 	{
// 		cout<<RED<<"Uart set false!"<<endl;
// 	}


// 	return 0;
// }

//=============AT
// void theard_uart_cmd(int fd){
//     string cmd;
//     cout<<"please input msg"<<endl;
//     while (true){  
//         cin>>cmd;
//         if(cmd.find("stop") != string::npos) {
//             cout<<"stop"<<endl;
//             exit(0);
//             break;
//         }else {
//             push_send_queue(cmd);
//         }
//     }
        
    
// }

// int main(int argc, char *argv[]) {
//     if(argc<2){
//         cout<<"./control_system <cmd>"<<endl;
//         return 0;
//     }
//     char* MODEMDEVICE=argv[1];
// 	int fd=open(MODEMDEVICE, O_RDWR | O_NOCTTY);
//     cout<<"open:"<<argv[1]<<endl;
//     if (fd < 0) {
//         perror(MODEMDEVICE);
//         exit(-1);
//     }
// 	if(UART_Set(fd,115200,0,8,1,'N')== TRUE)
// 	{
// 		thread thread1(theard_uart_report,fd);
//         thread thread2(theard_uart_cmd,fd);
//         thread1.detach();
// 		thread2.detach();
// 		while(1){
// 			this_thread::sleep_for(std::chrono::seconds(100));
// 		}

// 	}
// 	else
// 	{
// 		cout<<RED<<"Uart set false!"<<endl;
// 	}
// 	return 0;
// }

