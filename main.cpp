/******** Kurione2 main program ********/


#include "mbed.h"
#include "Communication.hpp"    // 通信ライブラリ

#define SERIAL_DEBUG
//#define DISABLE_WDT     // WDTを使わない場合（標準でON）

#define THRUSTER_HH 1700;
#define THRUSTER_HL 1600;
#define THRUSTER_NN 100;
#define THRUSTER_LH 1400;
#define THRUSTER_LL 1300;
#define SERVO_H 2000;
#define SERVO_N 1500;
#define SERVO_L 1000;

//Serial pc(USBTX, USBRX);
PinName rs485_txd = PA_9;
PinName rs485_rxd = PA_10;
//PinName rs485_nre = PA_12;
//PinName rs485_de = PA_11;
UnbufferedSerial debugger(PA_9, PA_10);
//Serial debugger(USBTX, USBRX);

PwmOut motors[] ={
    PwmOut (PA_6_ALT0), // TIM16_CH1
    PwmOut (PA_7_ALT1), // TIM17_CH1
    PwmOut (PA_8), // TIM1_CH1
    PwmOut (PB_5), // TIM3_CH2
    PwmOut (PA_3), // TIM15_CH2
    PwmOut (PB_0_ALT0)  // TIM3_CH3
};

DigitalOut LED_right_green(PB_7);
DigitalOut LED_left_red(PB_6);

DigitalOut emergency_signal(PA_1);
AnalogIn v_sense(PA_4);
AnalogIn i_sense(PA_5); // フィルタ：1k, 0.1uF, 10k 

const float ISENSE_OFFSET_VOLTAGE = 0.47f;
const float VSENSE_OFFSET_VOLTAGE = 3.0f;
float voltage,current;          // 電圧，電流値（float計算値）
int voltage_int, current_int;   // 電圧電流値（int修正値）

int wdt_cnt;                    // ウオッチドックタイマ変数（地上から信号無かったら強電供給を止める）
const int WDT_MAX = 100;//50;         // WDT発動値 → 高速か

//Communication communication(rs485_txd,rs485_rxd,rs485_de,rs485_nre);
//Communication communication(rs485_txd,rs485_rxd,PF_0,PF_1);
Communication communication(USBTX,USBRX,PF_0,PF_1);

int u[6];
//bool is_upd[6];

void updateMotors();
void updateCommunication();
int measureBattery();  // バッテリーの電圧，電流を測定
void sendBatteryData(void);     // バッテリーの各種情報をRS485で送る
void pcFlip();

int main()
{
    u[0] = THRUSTER_NN;
    u[1] = THRUSTER_NN;
    u[2] = THRUSTER_NN;
    u[3] = SERVO_N;
    u[4] = SERVO_N;
    u[5] = SERVO_N;
    //is_upd[6] = {true,true,true,true,true,true};    //信号アップデートが必要か
    //wait_ms(5000);
    communication.init(Communication::ROLL_MAIN);
    //pc.baud(115200);
    //pc.printf("start !\n");
    /*
    motors[0].pulsewidth_us(1000);
    motors[1].pulsewidth_us(1100);
    motors[2].pulsewidth_us(1200);
    motors[3].pulsewidth_us(1300);
    motors[4].pulsewidth_us(1400);
    motors[5].pulsewidth_us(1500);
    */
    updateMotors();
    //debugger.putc(int(communication.freshSerialBuffer()));
    wait_us(10000);
    int cnt = 0;
    LED_left_red = 1;
    LED_right_green = 0;
    //LED_right_green = 1;

#ifdef SERIAL_DEBUG
    debugger.attach(pcFlip, UnbufferedSerial::RxIrq);
#endif
    wdt_cnt = 0;
    //emergency_signal = 1;
    emergency_signal = 0;
    while(1) {
        measureBattery();
        updateCommunication();
        updateMotors();
#ifdef SERIAL_DEBUG
        cnt++; // シリアルデバッグ
        if (cnt>=80){
            if (cnt<=85){
                //debugger.printf("u[%d]:%4d, ",cnt-80, u[cnt-80]);
            }
            if (cnt >= 100){
                //debugger.printf("\n");
                cnt = 0;
            }
        }
#endif
#ifndef DISABLE_WTD
        wdt_cnt++;
#endif
        if (wdt_cnt>WDT_MAX) {
            wdt_cnt = 0;
            //emergency_signal = 1;   // WDT発動し，強電供給を停止
            LED_right_green = 0;
            u[0] = THRUSTER_NN;
            u[1] = THRUSTER_NN;
            u[2] = THRUSTER_NN;
            u[3] = SERVO_N;
            u[4] = SERVO_N;
            u[5] = SERVO_N;
            updateMotors();
        }
        //wait_ms(20);
        wait_us(10000);
    }
}

int measureBattery(void) {
    float i,v;
    v = v_sense;    // 測定生値
    i = i_sense;
    voltage = v/0.011f+VSENSE_OFFSET_VOLTAGE;
    current = 25*(3.3f*i-ISENSE_OFFSET_VOLTAGE);
    voltage_int = (int)(voltage*10);    // 10*V
    current_int = (int)(current*10);    // 10*I
    if((voltage_int<0)||(voltage_int>1000)||(current_int<-100)||(current_int>2000)){
        return -1;  // 値が外れている場合
    }else{
        return 0;
    }
}

void updateMotors(void) {
    for(int i = 0; i<6; i++){
        //if (is_upd[i])
        motors[i].pulsewidth_us(u[i]);
    }
}

void updateCommunication(void) {
    //debugger.printf("update! \n");
    if (communication.flag_rdat_check){
        int s = communication.decode();
        //debugger.printf("here: %d \n", s);
        //if ((s>10)||(s<-10)){
        //    debugger.printf("reset !! \n", s);
        //    NVIC_SystemReset();// ないと動かない(2022/08/07)
        //}
        communication.flag_rdat_check = false;
        switch (communication.receive_command_dat){
            case Command::SIGNAL_LAND_TO_MAIN:
                for (int i = 0; i<6; i++){
                    u[i] = communication.receive_num_dat.front();
                    communication.receive_num_dat.erase(communication.receive_num_dat.begin());
                    //communication.receive_num_dat.pop_back();
                }
                wdt_cnt = 0;                    // wdtカウントをリセット
                //wait_ms(10);
                wait_us(2000);     // 待ち時間高速化
                sendBatteryData();              // 10ms後にバッテリー情報送信
                LED_right_green = 1;
                break;
            case Command::POWER_SUPPLY_STOP:
                //emergency_signal = 1;           // 強電供給停止
                break;
            case Command::POWER_SUPPLY_START:
                //emergency_signal = 0;           // 強電供給開始
                break;
            case Command::BATT_RESET:
                NVIC_SystemReset();
                break;
            default:
                break;
        }
    }
}

void sendBatteryData(void) {
    communication.freshSendNumDat();
    communication.send_command_dat = Command::SIGNAL_BATT_TO_LAND;
    communication.send_num_dat.push_back(voltage_int);   // 電圧情報登録
    communication.send_num_dat.push_back(current_int);   // 電流情報登録
    communication.send_num_dat.push_back(emergency_signal==0);  // 強電供給状況
    communication.encode();
    communication.sendDat();
}


void pcFlip(void) {
    char c ;
    debugger.read(&c,1);
    
    switch(c) {
        case '0':
        u[0] = THRUSTER_HL;
        break;
        case '1':
        u[1] = THRUSTER_HL;
        break;
        case '2':
        u[2] = THRUSTER_HL;
        break;
        case '3':
        u[3] = SERVO_H;
        break;
        case '4':
        u[4] = SERVO_H;
        break;
        case '5':
        u[5] = SERVO_H;
        break;
        default:
        u[0] = THRUSTER_NN;
        u[1] = THRUSTER_NN;
        u[2] = THRUSTER_NN;
        u[3] = SERVO_N;
        u[4] = SERVO_N;
        u[5] = SERVO_N;
        break;
    }
}
