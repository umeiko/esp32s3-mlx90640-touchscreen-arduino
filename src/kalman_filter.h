// 卡尔曼滤波器
// By Umeko 2024.08.03
#ifndef _KALMAN_H_
#define _KALMAN_H_

#include <Arduino.h>

typedef struct 
{
    float P; //估算协方差
    float G; //卡尔曼增益
    float Output; //卡尔曼滤波器输出 
}KFPTypeS; //Kalman Filter parameter type Struct

const float Q = 1.0;
const float R = 1.5;


float KalmanFilter(KFPTypeS *kfp, float input)
{
    //估算协方差方程：当前 估算协方差 = 上次更新 协方差 + 过程噪声协方差
    kfp->P = kfp->P + Q;
    //卡尔曼增益方程：当前 卡尔曼增益 = 当前 估算协方差 / （当前 估算协方差 + 测量噪声协方差）
    kfp->G = kfp->P / (kfp->P + R);
    //更新最优值方程：当前 最优值 = 当前 估算值 + 卡尔曼增益 * （当前 测量值 - 当前 估算值）
    kfp->Output = kfp->Output + kfp->G * (input - kfp->Output); //当前 估算值 = 上次 最优值
    //更新 协方差 = （1 - 卡尔曼增益） * 当前 估算协方差。
    kfp->P = (1 - kfp->G) * kfp->P;
 
     return kfp->Output;
}
#endif