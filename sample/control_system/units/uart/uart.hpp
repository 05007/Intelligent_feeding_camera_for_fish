/***************************************************************
 文件名 : uart.hpp
 版本 : v1.0.0
 作者 : 郭辰磊
 单位 : 中国农业大学-国家数字渔业创新中心-数字养殖网箱项目
 描述 : 控制SCA200芯片的UART串口连接
 其他 : 无
 日志 : 初版v1.0.0 2023/3/10 郭辰磊创建
 ***************************************************************/
#ifndef  _USART_HPP
#define  _USART_HPP

#include "std_include.hpp"
#include "io_control.hpp"
typedef unsigned char Rcode;
#define DATA_BUFFER_SIZE 50 //数据缓冲池大小
typedef enum{
    uart0,
    uart1,
    uart2,
    uart3,
    uart4
}uart_num;
class uart_type{
    public:
    int uart_0;
    int uart_1;
    int uart_2;
    int uart_3;
    int uart_4;
    uart_type(){
        uart_0=-1;
        uart_1=-1;
        uart_2=-1;
        uart_3=-1;
        uart_4=-1;
    }
    int get_uart_fd(uart_num num){
        int fd;
        switch (num)
        {
        case uart0:
            fd=uart_0;
            break;
        case uart1:
            fd=uart_1;
            break;
        case uart2:
            fd=uart_2;
            break;
        case uart3:
            fd=uart_3;
            break;
        case uart4:
            fd=uart_4;
            break;
        default:
            fd=-1;
            break;
        }
        return fd;
    }

};

extern uart_type* opened_uart;
int open_uart(uart_num num,int speed,int flow_ctrl,int databits,int stopbits,int parity);
void close_uart(uart_num num);
void uart_freshen(uart_num num,int speed,int flow_ctrl,int databits,int stopbits,int parity);

int UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int UART_Recv(int fd, Rcode* rcv_buf,int data_len);
int UART_Send(int fd, Rcode* send_buf,int data_len);
void uart_clear_rec(int fd);
// void init_uart_thead();
// void theard_uart_Power_Line_Carrier(int fd);
// void theard_uart_report(int fd);
#endif
