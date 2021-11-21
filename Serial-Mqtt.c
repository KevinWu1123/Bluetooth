#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#define BUFFER_SIZE 100

void serial_init(int fd)
{
    struct termios options;
    tcgetattr(fd, &options);
    options.c_cflag |= ( CLOCAL | CREAD );
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CSTOPB;
    options.c_iflag |= IGNPAR;
    options.c_iflag &= ~(ICRNL | IXON);
    options.c_oflag = 0;
    options.c_lflag = 0;
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    tcsetattr(fd,TCSANOW,&options);
}

bool at_command_send(int fd, const char *command, const char *src)
{
    bool status = false;
    char *psrc = NULL;
    char get_buffer[BUFFER_SIZE];
    int read_num = 0;  

    write(fd,command,strlen(command));
    sleep(2);
    memset(get_buffer,0,sizeof(get_buffer));
    if (strcmp(src,"None") == 0) {
        return true;
    }
    read_num = read(fd, get_buffer, BUFFER_SIZE);
    if (read_num > 0) {
        psrc = strstr(get_buffer,src);
        if (psrc != NULL) {
            printf("%s\n",get_buffer);
            status = true;
        } else {
            printf("AT command back error\n");
        }
    } else {
        printf("read error\n");
    }
    return status;
}

int main(int argc, int argv[])
{
    int fd;    
    static int state = 0;
    static recommand_cnt = 0;
    fd = open("/dev/ttyUSB2",O_RDWR|O_NDELAY|O_NOCTTY);
    if (fd < 0) {
        printf("Open Failed\r\n");
    } else {
        printf("Open Successed\r\n");
        serial_init(fd);
        while (1) {
            recommand_cnt++;
            switch (state) {
                case 0:
                    if (at_command_send(fd,"AT\r\n","OK")) {
                        recommand_cnt = 0;
                        state = 1;
                    } else {
                        state = 0;                        
                        printf("WCDMA is not ready!\n");
                    }
                    break;
                case 1:
                    if (at_command_send(fd,"AT+QMTCFG=\"recv/mode\",0,0,1\r\n","OK")) { 
                        recommand_cnt = 0;
                        state = 2;
                    } else {
                        state = 7;
                        printf("recv/mode is not ready!\n");
                    }
                    break;
                case 2:
                    if (at_command_send(fd,"AT+QMTCFG=\"aliauth\",0,\"gj8rhsgbA1e\",\"test01\",\"549b3933e86da71a1eca01c18e3ae6da\"\r\n","OK")) { 
                        recommand_cnt = 0;
                        state = 3;
                    } else {
                        state = 7;
                        printf("aliauth is not ready!\n");
                    }
                    break;
                case 3:
                    if (at_command_send(fd,"AT+QMTOPEN=0,\"iot-06z00ge2b3rk219.mqtt.iothub.aliyuncs.com\",1883\r\n","+QMTOPEN: 0,0")) { 
                        recommand_cnt = 0;
                        state = 4;
                    } else {
                        state = 7;
                        printf("AT+QMTOPEN is not ready!\n");
                    }
                    break;
                case 4:
                    if (at_command_send(fd,"AT+QMTCONN=0,\"test01\"\r\n","+QMTCONN: 0,0,0")) { 
                        recommand_cnt = 0;
                        state = 5;
                        sleep(1);
                    } else {
                        state = 7;
                        printf("AT+QMTCONN is not ready!\n");
                    }
                    break;
                case 5:
                    if (at_command_send(fd,"AT+QMTPUB=0,0,0,0,\"/sys/gj8rhsgbA1e/test01/thing/event/property/post\"\r\n",">")) {
                        at_command_send(fd,"{params:{IndoorTemperature:17.3}}","None");
                        recommand_cnt = 0;
                        state = 6;
                    } else {
                        state = 6;
                        printf("AT+QMTPUB is failed!\n");
                    }
                    break;
                case 6:
                    if (at_command_send(fd,"\x1a\r\n","OK")) {
                        recommand_cnt = 0;
                        state = 8;
                        sleep(1);
                    } else {
                        state = 6;
                        printf("AT exit is not ready!\n");
                    }
                    break;
                case 7:
                    if (at_command_send(fd,"AT+QMTDISC=0\r\n","+QMTDISC: 0,0")) {
                        recommand_cnt = 0;
                        state = 0;
                        sleep(1);
                    } else {
                        state = 8;
                        printf("AT exit is not ready, need reset!\n");
                    }
                    break;
                default:
                    break;
            }
            if (recommand_cnt > 3) {
                recommand_cnt = 0;
                printf("WCDMA is error, need reset!\n");
            }
            if (state == 8) {
                state = 0;
                break;
            }
        }
    }
    if (isatty(fd)==0){
        printf("this is not  a terminal device\n");
    }
    return 0;
}



