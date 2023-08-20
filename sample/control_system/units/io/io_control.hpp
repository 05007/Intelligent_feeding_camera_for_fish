/***************************************************************
 文件名 : io_control.hpp
 版本 : v1.0.0
 作者 : 郭辰磊
 单位 : 中国农业大学-国家数字渔业创新中心-数字养殖网箱项目
 描述 : 统一IO处理机制，
 其他 : 消息、文件处理等
 日志 : 初版v1.0.0 2023/3/10 郭辰磊创建
 ***************************************************************/
#ifndef  _MESSAGE_HPP
#define  _MESSAGE_HPP
#include "std_include.hpp"
#include "uart.hpp"
//the following are UBUNTU/LINUX ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

typedef enum{
    msg_common, //普通log
    msg_common2,
    msg_tip,    //成功类提示
    msg_warn,   //警告
    msg_error,   //错误
}message_type;

void msg_log(message_type type,string msg_source,string msg);  //控制台打印
//LCD_msg_log(message_type type,string msg_source,string msg); //LCD输出-未实现
#endif