#include <stdio.h> /*标准输入输出定义*/
#include <stdlib. h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat. h>
#include <fcntl. h> /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h> /*错误号定义*/
#include <string.h>

enum parameter_type {
PT_PROGRAM_NAME = 0,
PT_DEV_NAME,
PT_CYCLE,

1PT_NUM
};

#define DBG(string, args...) \
do { \
printf ( "%s, %s()%u‑‑‑", __FILE__, __FUNCTION__, __LINE__); \
printf ( string, ##args); \
printf ( "\n"); \
} while (0)

void usage(void)
{
printf ( "You should input as: \n");
printf ( "\t select_test [/ dev/name] [Cycle Cnt]\n");
}

int OpenDev(char *name)
{
int fd = open(name, O_RDWR ); //| O_NOCTTY | O_NDELAY
if (-1 == fd)
DBG("Can't Open(%s)!", name);

return fd;
}

/**
*@brief 设置串口通信速率
*@param fd 类型 int 打开串口的文件句柄
*@param speed 类型 int 串口速度
*@return void
*/
void set_speed(int fd, int speed){
    int i;
int status;
struct termios Opt = {0};
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400, 19200, 9600, 4800, 2400, 1200, 300, 38400,
19200, 9600, 4800, 2400, 1200, 300, };
tcgetattr ( fd, &Opt);

for ( i = 0; i < sizeof( speed_arr) / sizeof(int); i ++) {
if ( speed == name_arr[i])
break;
}

tcflush( fd, TCIOFLUSH);
cfsetispeed(&Opt, speed_arr[i]);
cfsetospeed(&Opt, speed_arr[i]);

Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
Opt.c_oflag &= ~OPOST; /*Output*/

status = tcsetattr ( fd, TCSANOW, &Opt);
if ( status != 0) {
    DBG("tcsetattr fd");
return;
}
tcflush( fd, TCIOFLUSH);
}

/**
*@brief 设置串口数据位，停止位和效验位
*@param fd 类型 int 打开的串口文件句柄
*@param databits 类型 int 数据位 取值 为 7 或者8
*@param stopbits 类型 int 停止位 取值为 1 或者2
*@param parity 类型 int 效验类型 取值为N,E,O,,S
*/
int set_Parity(int fd, int databits,int stopbits,int parity)
{
struct termios options;

if ( tcgetattr ( fd,&options) != 0) {
perror("SetupSerial 1");
return ‑1;
}
options.c_cflag &= ~CSIZE;

switch (databits) /*设置数据位数*/
{
case 7:
options.c_cflag |= CS7;
break;
case 8:
options.c_cflag |= CS8;
break;
default:
fprintf ( stderr, "Unsupported data size\n");
return -1;
}