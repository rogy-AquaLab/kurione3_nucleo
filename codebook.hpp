
    class Command {
    public:
        enum {
            START = 201,
            END,
            SIGNAL_LAND_TO_MAIN,    // 地上からメイン回路への信号
            SIGNAL_BATT_TO_LAND,    // 電源ボックスから地上への信号
            POWER_SUPPLY_START,     // 強電系への電源供給を開始
            POWER_SUPPLY_STOP,      // 強電系への電源供給を停止
            MAIN_RESET,             // メイン回路のリセット要求
            BATT_RESET,             // 電源ボックスのリセット要求
            GIMMICK_LED_TAPE,       // テープLEDのうちメインNucleoで処理するもの 右緑と左赤  以下20230617追加
            GIMMICK_LED_LR1,        // ギミックNucleoで処理するフルカラーLEDの系統1
            GIMMICK_LED_LR2,        // ギミックNucleoで処理するフルカラーLEDの系統2
            GIMMICK_MOTORS,         // ギミックNucleoで処理するDCモータとサーボ
        };
    };