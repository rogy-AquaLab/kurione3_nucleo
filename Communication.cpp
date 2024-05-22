
    #include "Communication.hpp"
    
    Communication::Communication(PinName txd, PinName rxd, PinName de_pin, PinName nre_pin) : serial(txd,rxd), de(de_pin), nre(nre_pin), myled(PB_3) {
        serial.baud(9600);
        status = STATUS_SLEEPING; // 初期化前状態に戻る
        flag_rdat_check = false;
        is_highspeed_mode = false;  // 標準では低速モード
    }
    
    void Communication::init(int r) {
        role = r;
        status = STATUS_IDLING;
        serial.attach(callback(this, &Communication::receive), UnbufferedSerial::RxIrq);
        de = 0;
        nre = 0;    // 受信許可
    }
    
    void Communication::receive(void){  // データ受け取り関数
        int cnt = 0;
        while(serial.readable()) {
            unsigned char c = getc();
            
            if (status == STATUS_RECEIVING) { // 受信中
                if ( c==Command::END ){   // 終了コマンドなら，データ読み取り要求
                //printf("size:%d\n",receive_dat.size());
                    flag_rdat_check = true;
                    status = STATUS_IDLING; // 待機状態に戻る
                }else{
                   receive_dat.push_back(c);    // データ格納
                   // これ消すと動く 2023/06/18
                   //receive_dat.push(0);
                }
            }
            
            if (c==Command::START) {    // スタートコマンドなら，一旦受信バッファクリアして受け取りモード
                status = STATUS_RECEIVING;
                freshReceiveDat();
                break;
            }
            
            cnt++;
            if (cnt>30){
                break;
            }
        }
        
    }

    /*
    * @func extractData
    * @blief  変数に受信データを格納
    * @
    */
    unsigned char Communication::extractData(int data_size, int * buf){
        return 0;
    }
    
    void Communication::sendDat(void){  // 格納されているデータを送信
        de = 1;     // 送信許可
        status = STATUS_SENDING;
        putc(Command::START);
        //wait_ms(1);
        dataWait();
        while(!send_dat.empty()) {
            putc(send_dat.front());
            send_dat.erase(send_dat.begin());
            //wait_ms(1);
            dataWait();
        }
        putc(Command::END);
        //wait_ms(1);
        dataWait();
    }
    
    int Communication::decode(void) {
        
        if (receive_dat.empty()){
            return 0;
        }
        receive_command_dat = receive_dat.front();
        receive_dat.erase(receive_dat.begin());

        if ((receive_dat.size()>30)||(receive_dat.size()<=0)){
            return -1;
        }
        
        if (receive_num_dat.size()>10){
            NVIC_SystemReset();// ないと動かない(2022/08/07)
            return receive_num_dat.size();
        }
        freshReceiveNumDat();
        //receive_num_dat.clear();
        
        
        while(!receive_dat.empty()){
            int dh = receive_dat.front();   // 上位桁が必ず先
            receive_dat.erase(receive_dat.begin());
            if(receive_dat.empty()){
                return -1;                  // データ個数エラー
            }
            int dl = receive_dat.front();   //　下位桁が必ず後
            receive_dat.erase(receive_dat.begin());
            
            int d = 1;
            unsigned char dhp = 0;
            dhp = dh;
            if (dh>=100){   // 符号マイナス
                dhp = dhp-100;
                d = -1;
            }
            d = d*(100*dhp+dl);
            receive_num_dat.push_back(d);
            //receive_num_dat.push_back(d);
        }
        return 0;
    }
    
    int Communication::encode(void) {
        freshSendDat();
        send_dat.push_back(send_command_dat);
        while(!send_num_dat.empty()){
            int d = send_num_dat.front();
            send_num_dat.erase(send_num_dat.begin());
            if((d>9999)||(d<-9999))
                return -1;  // 値範囲エラー
            unsigned char dh,dl;
            dh = abs(d)/100;
            dl = abs(d)-100*dh;
            if(d<0)
                dh = dh+100;
            send_dat.push_back(dh);  // 上位が先
            send_dat.push_back(dl);  // 下位が後
        }
        return 0;
    }
    
    
    void Communication::freshReceiveDat(void) {
        while(!receive_dat.empty()) {
            receive_dat.erase(receive_dat.begin());
        }
    }
    
    void Communication::freshSendDat(void) {
        while(!receive_dat.empty()) {
            receive_dat.erase(receive_dat.begin());
        }
    }
    
    void Communication::freshSendNumDat(void) {
        while(!send_num_dat.empty()) {
            send_num_dat.erase(send_num_dat.begin());
        }
    }
    
    void Communication::freshReceiveNumDat(void) {
        while(!receive_num_dat.empty()) {
            receive_num_dat.erase(receive_num_dat.begin());
        }
        //receive_num_dat.clear();
    }
    
    int Communication::freshSerialBuffer(void) {
        int cnt = 0;
        while(serial.readable()){
            char c = getc();
            cnt++;
        }
        return cnt;
    }
    
    int Communication::abs(int x) {
        if (x>0){
        return x;
        }
        else{
            return -x;
        }
    }
    
    void Communication::dataWait(void) {
        if (is_highspeed_mode) {
            wait_us(100);   // 高速モードの待ち時間
        }else{
            wait_us(1000);     // 通常モードの待ち時間
        }
    }
    
    void Communication::setHighSpeedMode() {
        is_highspeed_mode = true;
        serial.baud(115200);
    }
    
    unsigned char Communication::getc(){       // 1文字受信
        unsigned char ret = 0;
        int read_size_ = serial.read(&ret, 1);
            //RCLCPP_INFO("read_size : %d, stat : %s", read_size, std::strerror(errno));
        if (ret==201){
            myled =! myled;
        }
        //ret = 0;
        return ret;
    }

    void Communication::putc(unsigned char dat){   // 1文字送信
        int status = serial.write(&dat, 1);
    }
    