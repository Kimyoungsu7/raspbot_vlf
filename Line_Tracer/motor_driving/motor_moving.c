#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//i2c address
#define ADDRESS 0x16

//I2C bus
static const char *deviceName = "/dev/i2c-1";

#define RAD2DEG(x) ((x)*180./M_PI)

int file_I2C;

int open_I2C(void)
{
    int file;
    if ((file = open(deviceName, O_RDWR)) < 0)
    {
        fprintf(stderr, "I2C: Failed to access %s\n", deviceName);
        exit(1);
    }
    printf("I2C: Connected\n");

    printf("I2C: acquiring bus to 0x%x\n", ADDRESS);

    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0)
    {
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x\n", ADDRESS);
        exit(1);
    }
    return file;
}

void close_I2C(int fd)
{
    close(fd);
}

void car_control(int l_dir, int l_speed, int r_dir, int r_speed)
{
    unsigned char data[5] = {0x01, l_dir, l_speed, r_dir, r_speed};
    if (write(file_I2C, data, 5) != 5)
    {
        printf("Failed to write to the i2c bus.\n");
    }
}

void car_stop()
{
    unsigned char data[2] = {0x02, 0x00};
    if (write(file_I2C, data, 2) != 2)
    {
        printf("Failed to write to the i2c bus.\n");
    }
}

int main(void)
{
    char buf[2]; // sonar data
    float sonar_data;

    file_I2C = open_I2C();
    if (file_I2C < 0)
    {
        printf("Unable to open I2C\n");
        return -1;
    }
    else
    {
        printf("I2C is Connected\n");
    }

    while (1)
    {
        car_control(1, 100, 1, 100); // 자동차를 앞으로 움직임
        sleep(2);                    // 2초간 움직임
        car_control(1, 0, 1, 0);     // 자동차를 멈춤
        sleep(2);                    // 2초간 멈춤
    }

    close_I2C(file_I2C);
    return 0;
}
