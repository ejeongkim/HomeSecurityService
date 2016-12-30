#include <EEPROM.h>
#include <MsTimer2.h>

#define Node_ID 7
#define Server_ID 50
#define Port_Num 7777

int pin_Light = A1, pin_TEMP = A2;
int pin_OPEN_DETECT = 3;
int pin_MOTION = 2;

uint8_t ID = 0;

uint32_t timer_check = 0;

uint8_t RX_flag = 0, TX_flag = 0, Timer_flag = 0, Motion_flag = 0;

uint8_t EEPROM_buf[2] = {0xAA, 0};

char RX_buf[17];

uint8_t TX_buf[17] = {0xA0, 0x0A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0xA0};

int RX_count = 0;

void setup() {
  static int RX_count = 0;
  static char Check_buf[3] = {0, 0, 0};
  
  pinMode(pin_OPEN_DETECT,INPUT);
  pinMode(pin_Light,INPUT);
  pinMode(pin_TEMP,INPUT);
  pinMode(pin_MOTION,INPUT);
  
  Serial.begin(115200);
  Serial1.begin(9600);
  
  delay(1000);
  
  timer_check = millis();
  
  Serial.println("Start Wifi Setting");
  
  Serial1.print("AT\r\n");
  delay(100);
  
  Serial1.print("AT\r\n");
  delay(100);
  
  Serial1.print("AT+WAUTO=0,anygate\r\n"); 
  delay(10);
  
  Serial1.print("AT+NDHCP=0\r\n");
  delay(10);

  Serial1.print("AT+NAUTO=0,1,192.168.10.");
  Serial1.print(Server_ID);
  Serial1.print(",");
  Serial1.print(Port_Num);
  Serial1.print("\r\n");
  delay(10);
  
  Serial1.print("AT+NSET=192.168.10.");
  Serial1.print(Node_ID);
  Serial1.print(",255.255.255.0,192.168.10.1\r\n");
  delay(10);
  
  Serial1.print("AT&W0\r\n");
  delay(10);
  
  Serial1.print("ATC0\r\n");
  delay(10);
  
  Serial1.print("ATA\r\n");
  delay(10);
  
  Serial.println("Wifi Setting Finish");
  
  attachInterrupt(0, Motion_ISR, FALLING);
  
  MsTimer2::set(200, TIMER_ISR);

  MsTimer2::start();
}

void loop() {
  uint16_t i, tmp = 0;
  
  if(((timer_check+4000) < millis()) && (RX_flag == 0)) {
    if(RX_count < 1) {
      if(EEPROM.read(0) == 0xAA) {
        ID = EEPROM.read(1);
        RX_flag = 1;
        
        Serial.print("\n\rID : ");
        Serial.println(ID);
        
        TX_buf[3] = ID;
      } else {
        RX_flag = 2;
        
        Serial.println("\n\rWifi Connected Error");
        Serial.print("\n\rPlease reset the ADK-2560"); 
      }
    }
  }
  
  if(Timer_flag && TX_flag) {
    tmp = analogRead(pin_Light);
    
    TX_buf[4] = tmp>>8;
    TX_buf[5] = tmp & 0xFF;
    TX_buf[6] = Motion_flag;
    TX_buf[7] = digitalRead(pin_OPEN_DETECT);
    TX_buf[8] = TEMP_read();
    Motion_flag = 0;
    TX_buf[14] = TX_buf[2];
    
    for(i=3; i<14; i++) {
      TX_buf[14] += TX_buf[i];
    }
    
    Serial.println("\n\r TX Packet data");
    
    for(i=0; i<17; i++) {
      Serial.write(' ');
      Serial.print(TX_buf[i],HEX);
      Serial1.write(TX_buf[i]);
    }
    
    Timer_flag = 0;
  }
}

void serialEvent1(void) {
  static char Check_buf[4] = {0, 0, 0, };
  uint8_t i,check_sum = 0, RX_cnt = 0;
  
  if(RX_flag == 0) {
    char da = Serial1.read();
    Serial.write(da);
    
    Check_buf[0] = Check_buf[1];
    Check_buf[1] = Check_buf[2];
    Check_buf[2] = da;
    
    if((Check_buf[0] == 'A') && (Check_buf[1] == 'T') && (Check_buf[2] == 'A') && (RX_count == 0)) {
      RX_count = 1;
    } else if(RX_count == 4) {
      if(Check_buf[2] != ':') {
        ID = ID*10 + (Check_buf[2]-'0');
      } else {
        RX_count++;
      }
    } else if(RX_count == 5) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'K')) {
          RX_flag = 1;
          delay(1000);
          
          RX_cnt = Serial1.available();
          Serial.println(RX_cnt);
          
          while(1) {
            Serial.write(Serial1.read());
            RX_cnt--;
            
            if(RX_cnt == 0)
              break;
          }
    
          Serial.print("\n\rID : ");
          Serial.print(ID);
          
          EEPROM_buf[1] = ID;
          
          if(EEPROM.read(1) != ID) {
            for(i=0; i<2; i++) {
              EEPROM.write(i,EEPROM_buf[i]);
            }
          }
          
          TX_buf[3] = ID;
        } else {
          RX_flag = 2;
          
          Serial.print("\n\rWifi Connected Error");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    } else if((Check_buf[2] == '.') && ((RX_count == 1) || (RX_count == 2) || (RX_count == 3))) {
      RX_count++;
    } else if(RX_count == 1) {
      if(Check_buf[2] == ']') {
        if((Check_buf[0] == 'O') && (Check_buf[1] == 'R')) {
          RX_flag = 2;
          
          Serial.print("\n\rWifi Connected fail");
          Serial.print("\n\rPlease reset the ADK-2560"); 
        }
      }
    }
    
  } else if(RX_flag == 1) {
    if(Serial1.available() > 16) {
      Serial1.readBytes(RX_buf, 17);
      
      if(((uint8_t)RX_buf[0] == 0xA0) && ((uint8_t)RX_buf[1] == 0x0A)) {
        for(i=2; i<14; i++) {
          check_sum += (uint8_t)RX_buf[i];
        }
        
        if(check_sum == (uint8_t)RX_buf[14]) {
          Serial.println("\n\r RX Packet data");
          
          for(i=0; i<17; i++) {
            Serial.write(' ');
            Serial.print((uint8_t)RX_buf[i],HEX);
          }
          
          TX_flag = RX_buf[4];
          
          if(!TX_flag) {
            for(i=4; i<14; i++) {
              TX_buf[i] = 0;
            }
            
            TX_buf[14] = TX_buf[2];
            
            for(i=3; i<14; i++) {
              TX_buf[14] += TX_buf[i];
            }
            
            Serial.println("\n\r TX Packet data");
            
            for(i=0; i<17; i++) {
              Serial.write(' ');
              Serial.print(TX_buf[i],HEX);
              Serial1.write(TX_buf[i]);
            }
          }
        }
      }
    }
  }
}

void serialEvent(void) {
  Serial1.write(Serial.read());
}

void Motion_ISR(void) {
  Motion_flag = 1;
}

char TEMP_read(void) {
  char da;
  int temp = 0;
  float value;
  
  temp = analogRead(pin_TEMP);
  value = (float)(temp/2.048);
  da = value - 50;
  
  return da;
}

void TIMER_ISR(void) {
  Timer_flag = 1;
}



