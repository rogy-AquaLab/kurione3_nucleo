
    #include "mbed.h"
    #include "codebook.hpp"
    #include <queue>
    #include <deque>
    #include <vector>
    
    class Communication {
    public:
        std::vector<unsigned char> send_dat;       // 送信データ
        std::vector<int> send_num_dat;             // 送信データの内数値部
        unsigned char send_command_dat;        // 受信データのコマンド部
        std::vector<unsigned char> receive_dat;    // 受信データ
        unsigned char receive_command_dat;        // 受信データのコマンド部
        std::vector<int> receive_num_dat;          // 受信データの内数値部
        //std::deque<int> receive_num_dat;          // 受信データの内数値部

        int role;                       // 自分の役割を明記
        int status;                     // 現在の状態
        bool flag_rdat_check;           // 受信データ貯まったか
        
        UnbufferedSerial serial;
        DigitalOut de;
        DigitalOut nre;
        DigitalOut myled;
        
        enum {
            ROLL_LAND = 0,
            ROLL_BATT,
            ROLL_MAIN,
            ROLL_GIMMICK    // 20230617追加
        };
        enum {
            STATUS_IDLING = 0,
            STATUS_SLEEPING,
            STATUS_RECEIVING,
            STATUS_SENDING,
        };
        
        Communication(PinName, PinName, PinName, PinName);
        void init(int);         // rollを設定．受信処理開始
        void receive(void);    // 割り込み時の1バイト受け取り
        void sendDat(void);     // データ送信
        unsigned char extractData(int, int*);   // データを引き出して変数に食わせる
        int decode(void);
        int encode(void);        
        void freshSendDat();
        void freshSendNumDat();
        void freshReceiveDat();
        void freshReceiveNumDat();
        void setHighSpeedMode();    // 高速モードの指定
        int freshSerialBuffer();
    
    private:
        int abs(int);
        void dataWait();                // データ送信待ち関数
        bool is_highspeed_mode; //　高速モードの指定
        unsigned char getc();       // 1文字受信
        void putc(unsigned char);   // 1文字送信 ちゃんと通信規約読んで複数バイト送受信にした方がいい
    };