#include "uart.hpp"
#include "RS485.hpp"
// queue<Rcode*> UART_Send_PLCCqueue;
uart_type* opened_uart=new uart_type;
/*******************************************************************
*名称：             open_port
*功能：             打开串口并返回串口设备文件描述
*入口参数：         fd      文件描述符
                   comport 想要打开的串口号
*出口参数：正确返回为非负，错误返回为-1
*******************************************************************/

int open_uart(uart_num num,int speed,int flow_ctrl,int databits,int stopbits,int parity){
    int uart_fd;
    switch (num)
    {
    case uart0:
        uart_fd=open("/dev/ttyS0", O_RDWR | O_NOCTTY);
        UART_Set(uart_fd,speed,flow_ctrl,databits,stopbits,parity);
        opened_uart->uart_0=uart_fd;
        break;
    case uart1:
        uart_fd=open("/dev/ttyS1", O_RDWR | O_NOCTTY);
        UART_Set(uart_fd,speed,flow_ctrl,databits,stopbits,parity);
        opened_uart->uart_1=uart_fd;
        break;
    case uart2:
        uart_fd=open("/dev/ttyS2", O_RDWR | O_NOCTTY);
        UART_Set(uart_fd,speed,flow_ctrl,databits,stopbits,parity);
        opened_uart->uart_2=uart_fd;
        break;
    case uart3:
        uart_fd=open("/dev/ttyS3", O_RDWR | O_NOCTTY);
        UART_Set(uart_fd,speed,flow_ctrl,databits,stopbits,parity);
        opened_uart->uart_3=uart_fd;
        break;
    case uart4:
        uart_fd=open("/dev/ttyS4", O_RDWR | O_NOCTTY);
        UART_Set(uart_fd,speed,flow_ctrl,databits,stopbits,parity);
        opened_uart->uart_4=uart_fd;
        break;
    default:
        uart_fd=-1;
        break;
    }
    if (uart_fd < 0) {
        perror("uart");
    }
    return uart_fd;
}


/*******************************************************************
*名称：             UART_Close
*功能：             关闭串口并返回串口设备文件描述
*入口参数：         fd          文件描述符
                    port        串口号(ttyS0,ttyS1,ttyS2)
*出口参数：void
*******************************************************************/

void close_uart(uart_num num){
    int uart_fd = opened_uart->get_uart_fd(num);
    if(uart_fd == FALSE){
        return;
    }
    close(uart_fd);
}

//刷新串口
void uart_freshen(uart_num num,int speed,int flow_ctrl,int databits,int stopbits,int parity){
    int uart_fd = opened_uart->get_uart_fd(num);
    if(uart_fd == FALSE){
        open_uart(num,speed,flow_ctrl,databits,stopbits,parity);
    }else{
        close(uart_fd);
        sleep(2);
        open_uart(num,speed,flow_ctrl,databits,stopbits,parity);
        sleep(2);
    }
}
/*******************************************************************
*名称：             UART_Set
*功能：             设置串口数据位，停止位和效验位
*入口参数：         fd          串口文件描述符
*                   speed       串口速度
*                   flow_ctrl   数据流控制
*                   databits    数据位   取值为 7 或者8
*                   stopbits    停止位   取值为 1 或者2
*                   parity      效验类型 取值为N,E,O,,S
*出口参数：正确返回为1，错误返回为0
*******************************************************************/
int UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{

    int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};

    struct termios options;

    /*  tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，
        该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.  */
    if( tcgetattr( fd,&options)  !=  0)
    {
        perror("SetupSerial 1");
        return(FALSE);
    }

    //设置串口输入波特率和输出波特率
    for ( unsigned int i = 0;  i < (sizeof(speed_arr) / sizeof(unsigned int));  i++)
    {
        if  (speed == name_arr[i])
        {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
        }
    }

    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;

    //设置数据流控制
    switch(flow_ctrl)
    {

        case 0 ://不使用流控制
              options.c_cflag &= ~CRTSCTS;
              break;

        case 1 ://使用硬件流控制
              options.c_cflag |= CRTSCTS;
              break;
        case 2 ://使用软件流控制
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
        case 5    :
                     options.c_cflag |= CS5;
                     break;
        case 6    :
                     options.c_cflag |= CS6;
                     break;
        case 7    :
                 options.c_cflag |= CS7;
                 break;
        case 8:
                 options.c_cflag |= CS8;
                 break;
        default:
                msg_log(msg_error,__func__,"Unsupported data size");
                //  fprintf(stderr,"Unsupported data size\n");
                return (FALSE);
    }
    //设置校验位
    switch (parity)
    {
        case 'n':
        case 'N': //无奇偶校验位。
                 options.c_cflag &= ~PARENB;
                 options.c_iflag &= ~INPCK;
                 break;
        case 'o':
        case 'O'://设置为奇校验
                 options.c_cflag |= (PARODD | PARENB);
                 options.c_iflag |= INPCK;
                 break;
        case 'e':
        case 'E'://设置为偶校验
                 options.c_cflag |= PARENB;
                 options.c_cflag &= ~PARODD;
                 options.c_iflag |= INPCK;
                 break;
        case 's':
        case 'S': //设置为空格
                 options.c_cflag &= ~PARENB;
                 options.c_cflag &= ~CSTOPB;
                 break;
        default:
                msg_log(msg_error,__func__,"Unsupported parity");
                //fprintf(stderr,"Unsupported parity\n");
                return (FALSE);
    }
    // 设置停止位
    switch (stopbits)
    {
        case 1:
                 options.c_cflag &= ~CSTOPB; break;
        case 2:
                 options.c_cflag |= CSTOPB; break;
        default:
                msg_log(msg_error,__func__,"Unsupported stop bits");
                //fprintf(stderr,"Unsupported stop bits\n");
                return (FALSE);
    }

    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);
    // 全双工模式设置
    options.c_cflag |= (CLOCAL | CREAD);
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 0; /* 读取一个字符等待*(1/10)s */
    options.c_cc[VMIN] = 0; /* 读取字符的最少个数 */

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    tcflush(fd,TCIOFLUSH);

    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("com set error!\n");
        return (FALSE);
    }
    return (TRUE);
}


/*******************************************************************
* 名称：            UART_Recv
* 功能：            接收串口数据
* 入口参数：        fd         文件描述符
*                   rcv_buf    接收串口中数据存入rcv_buf缓冲区中
*                   data_len   缓冲区的长度
* 出口参数：        返回接受到数据的长度
*******************************************************************/
int UART_Recv(int fd, Rcode *rcv_buf,int buf_len)
{
    fd_set fs_read;
    struct timeval time;
    int len = 0;
    int res = 0; 
    memset(rcv_buf,0,buf_len);
    int fs_length=1;
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
    if (fd==opened_uart->get_uart_fd(uart2))
    {
        time.tv_sec = 2; //设置接受数据等待时间
        time.tv_usec = 0; //等待时间的微妙
    }else{
        time.tv_sec = 1; //设置接受数据等待时间
        time.tv_usec = 0; //等待时间的微妙
    }
    
   
    fs_length = 8;
    Rcode fs_data[fs_length]; //一帧数据缓冲
    memset(fs_data,0,fs_length);
    // tcflow(fd,TCOOFF);
    //tcflush(fd, TCIOFLUSH);
    while (len<buf_len)
    {
        select(fd+1,&fs_read,NULL,NULL,&time);
        if(FD_ISSET(fd,&fs_read)){
            res = read(fd,fs_data,fs_length);
            if(res <= 0){     //无数据时停止
                break;
            }
            if((len + res) >= buf_len){     //数据溢出
                msg_log(msg_error,__func__,"Accept data overflow!");
                do{ res = read(fd,fs_data,fs_length);}while(res > 0);
                //cout<<"Accept data overflow!"<<endl;
                break;
            }
            for(int i=0; i<res ;++i){
                rcv_buf[len]=fs_data[i]; 
                ++len;
            }
        }else  break;
    }
    if(len>0){
        cout << "rev_cmd:" << std::endl;
        for(int i=0;i<len;++i)
            std::cout << "0x" << std::hex << static_cast<int>(rcv_buf[i])<<" ";
        cout << endl;
    }
    tcflush(fd, TCIFLUSH);
    //tcflow(fd,TCOON);
    // msg_log(msg_common2,__func__,msg.str());
    return len;
}
/********************************************************************
* 名称：            UART_Send
* 功能：            发送数据
* 入口参数：        fd           文件描述符
*                   send_buf     存放串口发送数据
*                   data_len     一帧数据的个数
* 出口参数：        正确返回为1，错误返回为0
*******************************************************************/
int UART_Send(int fd, Rcode* send_buf,int data_len)
{
    int len = 0;
    fd_set fs_write;
    int res = 0;
    Rcode fs_data[1];
    FD_ZERO(&fs_write);
    FD_SET(fd,&fs_write);
    struct timeval time;
    time.tv_sec = 1;
    time.tv_usec = 0;

    select(fd+1,NULL,&fs_write,NULL,&time);
    //使用select实现串口的多路通信
    if (FD_ISSET(fd,&fs_write))
    {   
        cout << "send_cmd:" << std::endl;   
        for(int i=0;i<data_len;++i)
            cout << "0x" << std::hex << static_cast<int>(send_buf[i])<<" ";
        cout<<endl;
        len = write(fd,send_buf,data_len);
        cout << "send_end:" << std::endl;   
        //tcdrain(fd);
        //tcflow(fd,TCION);
    }
    tcflush(fd, TCOFLUSH);
    return FALSE;
}
void uart_clear_rec(int fd){//清空读取数据
    int res;
    Rcode fs_data[1];
    fs_data[0] = 0;
    do{ res = read(fd,fs_data,1);}while(res > 0); 
}