#include "io_control.hpp"

void msg_log(message_type type,string msg_source,string msg){
    fd_set fds;
    struct timeval time;
    FD_ZERO(&fds);
    FD_SET(STDOUT_FILENO, &fds); // 监听标准输出池
    time.tv_sec = 3; //设置接受数据等待时间
    time.tv_usec = 0; //等待时间的微妙
    fflush(stdout); // 刷新缓冲区，确保输出到标准输出池中

    // 等待标准输出池变为可读状态
    int ret = select(STDOUT_FILENO + 1, NULL , &fds, NULL, &time);
    if (ret == -1) {
        perror("select");
        return;
    }
    switch(type){
        case msg_common:
            cout << GREEN  << "[LOG]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
            break;
        case msg_common2:
            cout << MAGENTA  << "[LOG]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
            break;
        case msg_tip:
            cout << BLUE << "[TIP]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
            break;
        case msg_warn:
            cout << YELLOW << "[WARN]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
            break;
        case msg_error:
            cout << RED << "[ERROR]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
            break;
        default:
            cout << GREEN << "[LOG]" << "[FROM:" << msg_source << "]" << msg << WHITE << endl;
		    break;
    }
}
