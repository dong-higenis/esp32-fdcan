#include <ACAN2517FD.h>
#include <SPI.h>

/**
 * @brief Pin 설정
 */
#define LED_PIN      2
#define BTN_PIN      27
#define CAN_SPI_CS   5
#define CAN_SPI_MISO 19
#define CAN_SPI_MOSI 23
#define CAN_SPI_CLK  18
#define CAN_INT      22

static bool led_state = HIGH;

/**
 * @brief CAN 초기화
 */
ACAN2517FD can(CAN_SPI_CS, SPI, CAN_INT);

CANFDMessage frame;
/**
 * @brief Serial, Pin, SPI, CAN 설정 
 * @attention 보드에 장착된 크리스탈 = 40MHz가 아닐경우 옵션을 바꿔주십시오.
 */
void setup()
{
  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, led_state);

  pinMode(CAN_SPI_CS, OUTPUT);
  digitalWrite(CAN_SPI_CS, HIGH);

  SPI.begin(CAN_SPI_CLK, CAN_SPI_MISO, CAN_SPI_MOSI);
  SPI.setDataMode(SPI_MODE0);
  SPI.setFrequency(1000000);

  Serial.println("Configure ACAN2517FD");

  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 500 * 1000, DataBitRateFactor::x1);
  settings.mRequestedMode = ACAN2517FDSettings::NormalFD;

  uint32_t errorCode = can.begin(settings, []
                                 { can.isr(); });
  if (errorCode != 0)
  {
    Serial.print("Configuration error 0x");
    Serial.println(errorCode, HEX);
    while (1);
  }
  Serial.println("ACAN2517FD Initialized");

  frame.id      = 0x500;
  frame.ext     = false;
  frame.len     = 2;
  frame.data[0] = 'H';
  frame.data[1] = 'I';
}
/**
 * @brief 버튼 채터링관련, 버튼을 누를시 CAN TX, 그외 실시간으로 CAN Message를 받습니다
 */
void loop()
{
  static bool          last_btn_state   = HIGH;
  static bool          stable_btn_state = HIGH;
  static unsigned long last_debounce    = 0;
  static uint32_t      tx_cnt           = 0;
  static uint32_t      rx_cnt           = 0;

  bool btn = digitalRead(BTN_PIN);

  if (btn != last_btn_state)
  {
    last_debounce  = millis();
    last_btn_state = btn;
  }

  if ((millis() - last_debounce) > 50)
  {
    if (btn != stable_btn_state)
    {
      stable_btn_state = btn;

      if (stable_btn_state == LOW)
      {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));

        if (can.tryToSend(frame))
        {
          Serial.printf("TX: %d\n", ++tx_cnt);
        }
        else
        {
          Serial.println("TX Failed");
        }
      }
    }
  }

  if (can.available())
  {
    CANFDMessage rx_frame;
    can.receive(rx_frame);

    Serial.printf("RX: %d | ID: 0x%X | Data: ", ++rx_cnt, rx_frame.id);
    for (uint8_t i = 0; i < rx_frame.len; i++)
    {
      Serial.printf("%02X ", rx_frame.data[i]);
    }
    Serial.println();
  }
}
