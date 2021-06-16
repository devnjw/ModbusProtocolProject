#include "mbed.h"
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>

UnbufferedSerial pc(CONSOLE_TX, CONSOLE_RX, 230400);
UnbufferedSerial wifi(ARDUINO_UNO_D8, ARDUINO_UNO_D2, 115200);
Thread sock_thread;
char buffer[80];
char buffer_wifi[80];

char command_out[20];
char command_in[20];
int pointer = 0;
int flag = 0;
char illuminance[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x04, 0x00, 0x00, 0x00, 0x02};

void Char2Hex(unsigned char ch, char* szHex)
{
 static unsigned char saucHex[] = "0123456789ABCDEF";
 szHex[0] = saucHex[ch >> 4];
 szHex[1] = saucHex[ch&0xF];
 szHex[2] = 0;
}

int main(){

    char ch_pc;
    char ch_wifi;

    int flag = 0;
    int cnt = 0;

    sprintf(buffer, "\r\n----STREET LIGHT CONTROL ----\r\n");
    pc.write(buffer, strlen(buffer));

    //sock_thread.start(&check_status);

    while(true){

        if(pc.readable()){
            pc.read(&ch_pc, 1);
            pc.write(&ch_pc,1);
            if(ch_pc==0x49){ // I: illuminance sensor
                sprintf(buffer, "AT+CIPSEND=1,%d\r\n",12);
                wifi.write(buffer,strlen(buffer));
                ThisThread::sleep_for(500ms);

                for(int i=0; i<12; ++i){
                    wifi.write(illuminance+i,1);
                }
            }
        }
        if(wifi.readable()){
            wifi.read(&ch_wifi,1);
            if(ch_wifi==0x32)
                flag=1;
            if(flag){
                if(ch_wifi == 0x3A||cnt>0){
                    buffer_wifi[cnt] = ch_wifi;
                    char hex[3];

                    cnt++;
                    if(cnt==14) {
                        flag = 0;
                        cnt=0;
                        for(int i=0; i<14; ++i){
                            Char2Hex(buffer_wifi[i], hex);

                            sprintf(buffer, "*%s\n", hex);
                            pc.write(buffer, 5);
                        }
                        if(buffer_wifi[10] < 10){
                            // dark -> open street light
                            sprintf(buffer, "AT+CIPSEND=0,%d\r\n",2);
                            wifi.write(buffer,strlen(buffer));
                            ThisThread::sleep_for(500ms);

                            sprintf(buffer,"@\n");
                            wifi.write(buffer, strlen(buffer));
                            ThisThread::sleep_for(2000ms);
                        }

                        else{
                            // bright -> close street light
                            sprintf(buffer, "AT+CIPSEND=0,%d\r\n",2);
                            wifi.write(buffer,strlen(buffer));
                            ThisThread::sleep_for(500ms);

                            sprintf(buffer,"#\n");
                            wifi.write(buffer, strlen(buffer));
                            ThisThread::sleep_for(2000ms);
                        }
                    }
                }
                else
                    pc.write(&ch_wifi,1);
            }
            else
                pc.write(&ch_wifi,1);
        }
    }
}
