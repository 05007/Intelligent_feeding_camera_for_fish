//常用标准库文件，宏定义
#ifndef  _STD_INCLUDE_HPP
#define  _STD_INCLUDE_HPP
#include<sys/mman.h>
#include<stdio.h>      /*标准输入输出定义*/
#include<stdlib.h>     /*标准函数库定义*/
#include<unistd.h>     /*Unix 标准函数定义*/
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>      /*文件控制定义*/
#include<termios.h>    /*PPSIX 终端控制定义*/
#include<errno.h>      /*错误号定义*/
#include<string.h>
#include<iostream>      /*C++IO*/
#include<fstream>
#include<thread>
#include<pthread.h>
#include<vector>
#include<queue>
#include<map>
#include<array>
#include <sstream>

#define FALSE  -1
#define TRUE   0
using namespace std;    //为方便使用标准命名空间，不推荐常用
#endif