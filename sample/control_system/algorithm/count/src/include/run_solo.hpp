/*
 * run_classfication.h
 *
 *  Created on: Dec 29, 2020
 *      Author: Smart Software Team
 */
#include "std_include.hpp"
#ifndef INCLUDE_NPUSAMPLECLASSIFICATION_H_
#define INCLUDE_NPUSAMPLECLASSIFICATION_H_
#include "common.h"
#include "sc_npu.h"
#include "mpi_npu.h"
#include "postProcess.h"
#include "sample_svp_postprocess.h"
#include "sample_vio.h"
#include "sample_comm.h"
#include "sample_comm_npu.h"
#include "sample_comm_svp.h"
#include "sample_svp_npu_software.h"
// #include "sample_comm_vpss.h"
#include "sc_type.h"
#include "sc_comm_ive.h"
#include "sc_common.h"
#include "sample_comm_ive.h"
#include "sample_comm_audio.h"
// #include "sample_vo.h"
#include "mpi_vo.h"
#include "mpi_sys.h"
#include "opencv2/opencv.hpp"

#include "io_control.hpp"
#include "uart.hpp"
class Trajectory_solving{
public:
    bool sign; //是否有轨迹的标志
    int people_num; //当前轨迹人数
    int Trajectory_loss_num; //轨迹丢失数，累积到一定数量当前轨迹结束
    int MAX_LOSS_NUM; //最大空帧数
    SC_FLOAT start_x; //起始轨迹中心点
    SC_FLOAT start_y;
    SC_FLOAT end_x; //终止轨迹中心点
    SC_FLOAT end_y;
    Trajectory_solving(int max_loss_num){
        this->MAX_LOSS_NUM=max_loss_num;    //设置最大空帧数
        this->init();
    }
    void init(){//初始化参数
        this->sign = false;
        this->Trajectory_loss_num = 0;
        this->people_num = 0;
        this->start_x = 0;
        this->start_y = 0;
        this->end_x = 0; 
        this->end_y = 0;
    }
    void Tr_start(SC_FLOAT sx,SC_FLOAT sy,int pnum){ //开始进人
        msg_log(msg_common2,__func__,"Detected personnel entry!");
        this->sign = true;
        this->start_x = sx; 
        this->start_y = sy;
        this->end_x = sx; 
        this->end_y = sy;
        this->people_num = pnum;
    }
    void Tr_refresh(SC_FLOAT rx,SC_FLOAT ry,int pnum){ //轨迹刷新
        this->end_x = rx; 
        this->end_y = ry;
        if(pnum > this->people_num) this->people_num = pnum;
    }
    void Tr_end(){  //轨迹结束
        sc_float dx; //距原点距离
        sc_float dy; //距原点距离
        string Tr_end_ms;
        msg_log(msg_common2,__func__,"Detected personnel leave!");
        dx=this->end_x - this->start_x;
        dy=this->end_y - this->start_y;
        Tr_end_ms = "Trajectory analysis results:\n direction: (" + (to_string(dx)+","+to_string(dy)) + ")," +
                    "people_num: " + to_string(this->people_num)+ ".";
        msg_log(msg_common,__func__,Tr_end_ms);
        this->init();
    }
    void next_frame(SC_FLOAT x,SC_FLOAT y,int pnum){ //有轨迹的帧处理
        this->Trajectory_loss_num = 0; //检测到轨迹取消丢帧计数
        if(!this->sign) Tr_start(x,y,pnum); //首帧
        else Tr_refresh(x,y,pnum); //持续刷新轨迹
    }

    void loss_frame(){  //空帧处理
        if(this->sign){
            if(Trajectory_loss_num < MAX_LOSS_NUM) ++this->Trajectory_loss_num; //丢帧计数增加
            else Tr_end();
        }
    }
};
int run_solo(char *npuFile, char * imgFile);
int run_solo_2(char *npuFile, char * imgFile);
int run_solo_3(char *npuFile, char * imgFile);
int run_solo_4(char *npuFile, char * imgFile);
int run_solo_5(char *npuFile, char * imgFile);
#endif /* INCLUDE_NPUSAMPLECLASSIFICATION_H_ */
