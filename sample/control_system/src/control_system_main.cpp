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
#include "net_link.hpp"
#include "io_control.hpp"
#include "run_solo.hpp"
#include "RS485.hpp"
// #include "libswscale/swscale.h"
// #include "libavcodec/avcodec.h"
// #include "libavformat/avformat.h"
// #include "sc_type.h"
// #include "sample_comm_svp.h"
// #include "sample_comm_npu.h4
// #include "sample_npu_main.h"
// #include "sample_svp_npu_software.h"

using namespace cv;

int main(int argc, char *argv[])
{
	
	net_link_init();    //连接网络
	msg_log(msg_common,__func__,"Net Link!");
	net_mqtt_init();	//初始化mqtt网络连接
	init_RS485_thread();	//初始化RS485控制
	msg_log(msg_common,__func__,"Netcage control system start V1.0.0-beta.15");
	while(1){
		//run_solo(); 算法还在进一步优化中，部署
		//主线程暂时空着，未来对接LCD屏控制
		sleep(1000);
	}
	return 0;
}

