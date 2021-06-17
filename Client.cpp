#include "mbed.h"
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>

UnbufferedSerial pc(CONSOLE_TX, CONSOLE_RX, 230400);
UnbufferedSerial wifi(ARDUINO_UNO_D8, ARDUINO_UNO_D2, 115200);
int VER = 0;
Thread sock_thread;
char buffer[80];
char receive[20];

char barricade[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x06, 0x00, 0x04, 0x00, 0x01};
char illuminance[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x04, 0x00, 0x00, 0x00, 0x02};


void Char2Hex(unsigned char ch, char* szHex)
{
 static unsigned char saucHex[] = "0123456789ABCDEF";
 szHex[0] = saucHex[ch >> 4];
 szHex[1] = saucHex[ch&0xF];
 szHex[2] = 0;
}

void check_status (){

    sprintf(buffer, "AT+CWMODE=1\r\n");
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(3000ms);

    sprintf(buffer, "AT+CWJAP=\"tp_4ahnssu\",\"01051611257a\"\r\n");
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(10000ms);

    sprintf(buffer, "AT+CIPMUX=1\r\n");
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(3000ms);

    sprintf(buffer, "AT+CIPSTART=0,\"TCP\",\"192.168.0.141\",50000\r\n"); //RTU server
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(10000ms);

    sprintf(buffer, "AT+CIPSTART=1,\"TCP\",\"192.168.0.200\",502\r\n"); //TCP server
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(10000ms);

    sprintf(buffer, "AT+CIPSTATUS\r\n");
    wifi.write(buffer, strlen(buffer));
    ThisThread::sleep_for(3000ms);
}

int main(){
    sock_thread.start(&check_status);

    char ch_pc;
    char ch_wifi;
    char hex[3];

    int flag = 0;
    int read = 0;
    int cnt = 0;

    sprintf(buffer, "\r\n---- TRAFFIC CONTROL ----\r\n");
    pc.write(buffer, strlen(buffer));


    while(true){
        if(VER == 0){
            if(pc.readable()){
                pc.read(&ch_pc, 1);
                pc.write(&ch_pc,1);
                if(ch_pc==0x5B){ // Input = '['

                    //open led
                    sprintf(buffer, "AT+CIPSEND=0,%d\r\n",2);
                    wifi.write(buffer,strlen(buffer));
                    ThisThread::sleep_for(500ms);

                    sprintf(buffer,"[\n");
                    wifi.write(buffer, strlen(buffer));
                    ThisThread::sleep_for(200ms);

                    //open barricade
                    barricade[11]=0x01;

                    sprintf(buffer, "AT+CIPSEND=1,%d\r\n",12);
                    wifi.write(buffer,strlen(buffer));
                    ThisThread::sleep_for(500ms);

                    for(int i=0; i<12; ++i)
                        wifi.write(barricade+i,1);

                }
                else if(ch_pc==0x5D){ // Output = ']'

                    //close led
                    sprintf(buffer, "AT+CIPSEND=0,%d\r\n",2);
                    wifi.write(buffer,strlen(buffer));
                    ThisThread::sleep_for(500ms);

                    sprintf(buffer,"]\n");
                    wifi.write(buffer, strlen(buffer));
                    ThisThread::sleep_for(2000ms);

                    //close barricade
                    barricade[11]=0x02;

                    sprintf(buffer, "AT+CIPSEND=1,%d\r\n",12);
                    wifi.write(buffer,strlen(buffer));
                    ThisThread::sleep_for(500ms);

                    for(int i=0; i<12; ++i)
                        wifi.write(barricade+i,1);
                }
                else if(ch_pc==0x2A){ // change ver = '*'
                    VER = 1;
                    sprintf(buffer, "version changed: %d\n\n",VER);
                    pc.write(buffer, strlen(buffer));
                }

            }

            if(wifi.readable()){
                wifi.read(&ch_wifi,1);
                pc.write(&ch_wifi,1);
            }
        }
        else if(VER == 1){
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
                else if(ch_pc==0x2A){ // change ver = '*'
                    VER = 0;
                    sprintf(buffer, "version changed: %d\n\n",VER);
                    pc.write(buffer, strlen(buffer));
                }
            }
            if(wifi.readable()){
                wifi.read(&ch_wifi,1);
                if(ch_wifi==0x32)
                    flag=1;
                if(flag){
                    if(ch_wifi == 0x3A||cnt>0){
                        receive[cnt] = ch_wifi;
                        char hex[3];

                        cnt++;
                        if(cnt==14) {
                            flag = 0;
                            cnt=0;
                            for(int i=0; i<14; ++i){
                                Char2Hex(receive[i], hex);

                                sprintf(buffer, "*%s\n", hex);
                                pc.write(buffer, 5);
                            }
                            if(receive[10] < 10){
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
}
