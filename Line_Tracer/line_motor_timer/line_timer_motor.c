#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#define SENSOR1 2  // Physical pin 7 
#define SENSOR2 3  // Physical pin 11
#define SENSOR3 0  // Physical pin 15
#define SENSOR4 7  // Physical pin 13 

#define ADDRESS 0x16    // I2C address
static const char *deviceName = "/dev/i2c-1";  // I2C bus

int file_I2C = -1; 

void setup() 
{
    if (wiringPiSetup() == -1) 
    {
        fprintf(stderr, "wiringPi Setup error: %s\n", strerror(errno));
        return;
    }
    
    pinMode(SENSOR1, INPUT);
    pinMode(SENSOR2, INPUT);
    pinMode(SENSOR3, INPUT);
    pinMode(SENSOR4, INPUT);
}

int open_I2C(void) 
{
    int file;

    if ((file = open(deviceName, O_RDWR)) < 0) 
    {
        fprintf(stderr, "I2C: Failed to access %s: %s\n", deviceName, strerror(errno));
        return -1;
    }

    printf("I2C: Connected\n");

    printf("I2C: acquiring bus to 0x%x\n", ADDRESS);

    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0) 
    {
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x: %s\n", ADDRESS, strerror(errno));
        close(file);
        return -1;
    }

    return file;
}

void close_I2C(int fd) 
{
    if (fd != -1) 
    {
        close(fd);
        printf("I2C: Closed\n");
    }
}

void car_control(int l_dir, int l_speed, int r_dir, int r_speed) 
{
    unsigned char data[5] = {0x01, l_dir, l_speed, r_dir, r_speed};

    printf("Sending control command to I2C bus...\n");
    if (write(file_I2C, data, 5) != 5) 
    {
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Control command sent successfully.\n");
    }
}

void car_stop() 
{
    unsigned char data[2] = {0x02, 0x00};

    printf("Sending stop command to I2C bus...\n");
    if (write(file_I2C, data, 2) != 2) 
    {
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Stop command sent successfully.\n");
    }
}

// 전역 변수로 이전 센서 값 저장
int prev_sensor1Value = 0;
int prev_sensor2Value = 0;
int prev_sensor3Value = 0;
int prev_sensor4Value = 0;

void line_trace(int sensor1, int sensor2, int sensor3, int sensor4) 
{
    int l_dir, l_speed, r_dir, r_speed;

    l_dir = 1;  
    r_dir = 1;  
    l_speed = 100;
    r_speed = 100;

    if (sensor4 == 1 && sensor3 == 0 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 130;
        r_speed = 20; 
    } 
    else if (sensor4 == 1 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 130;
        r_speed = 70; 
    }
    else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 90;
        r_speed = 40; 
    } 
    else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 1 && sensor1 == 0) 
    {
        l_speed = 200;
        r_speed = 190; 
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 0) 
    {
        l_speed = 30;
        r_speed = 140;
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 1) 
    {
        l_speed = 30;
        r_speed = 150; 
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 0 && sensor1 == 1) 
    {
        l_speed = 20;
        r_speed = 150; 
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 0 && sensor1 == 0) 
    {
        // 이전 센서 값 유지
        sensor1 = prev_sensor1Value;
        sensor2 = prev_sensor2Value;
        sensor3 = prev_sensor3Value;
        sensor4 = prev_sensor4Value;

        if (sensor4 == 1 && sensor3 == 0 && sensor2 == 0 && sensor1 == 0) 
        {
            l_speed = 130;
            r_speed = 20; 
        } 
        else if (sensor4 == 1 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
        {
            l_speed = 110;
            r_speed = 30; 
        }
        else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
        {
            l_speed = 130;
            r_speed = 70; 
        } 
        else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 1 && sensor1 == 0) 
        {
            l_speed = 200;
            r_speed = 190; 
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 0) 
        {
            l_speed = 30;
            r_speed = 140;
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 1) 
        {
            l_speed = 30;
            r_speed = 150; 
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 0 && sensor1 == 1) 
        {
            l_speed = 20;
            r_speed = 150; 
        }
    }

    car_control(l_dir, l_speed, r_dir, r_speed);

    // 현재 센서 값을 이전 센서 값으로 저장
    prev_sensor1Value = sensor1;
    prev_sensor2Value = sensor2;
    prev_sensor3Value = sensor3;
    prev_sensor4Value = sensor4;
}

void loop() 
{
    int sensor1Value = digitalRead(SENSOR1);
    int sensor2Value = digitalRead(SENSOR2);
    int sensor3Value = digitalRead(SENSOR3);
    int sensor4Value = digitalRead(SENSOR4);

    printf("Sensor Values: %d %d %d %d\n", sensor1Value, sensor2Value, sensor3Value, sensor4Value);

    line_trace(sensor1Value, sensor2Value, sensor3Value, sensor4Value);
}

int main(void) 
{
    setup();

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
    
		///////////// 타이머 추가 //////////////
    unsigned long rateTimer = micros();
    unsigned long now;
    unsigned long controlPeriod = 10000;  // controlPeriod in microseconds

    while (1) 
    {
        now = micros();
        while ((now - rateTimer) < controlPeriod) 
        {
            now = micros();
        }

        loop();

        // Update the timer
        rateTimer = micros();
    }
		//////////////////////////////////////
		
		close_I2C(file_I2C);

    return 0;
}
