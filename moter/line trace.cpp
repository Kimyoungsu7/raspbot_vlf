#include <stdio.h>         // 표준 입출력 라이브러리
#include <wiringPi.h>      // wiringPi 라이브러리, GPIO 제어를 위해 사용
#include <unistd.h>        // POSIX API 접근을 위한 라이브러리
#include <errno.h>         // 오류 번호를 위한 라이브러리
#include <linux/i2c-dev.h> // I2C 통신을 위한 라이브러리
#include <sys/ioctl.h>     // I/O 제어를 위한 라이브러리
#include <fcntl.h>         // 파일 제어를 위한 라이브러리
#include <string.h>        // 문자열 처리를 위한 라이브러리

// 센서 핀 정의 (wiringPi 핀 번호)
#define SENSOR1 2  
#define SENSOR2 3  
#define SENSOR3 0 
#define SENSOR4 7 

// I2C 장치 주소
#define ADDRESS 0x16    
static const char *deviceName = "/dev/i2c-1";  

int file_I2C = -1; // I2C 파일 디스크립터 초기화

// 초기 설정 함수
void setup() 
{
    // wiringPi 초기화
    if (wiringPiSetup() == -1) 
    {
        fprintf(stderr, "wiringPi Setup error: %s\n", strerror(errno));
        return;
    }
    
    // 각 센서 핀을 입력 모드로 설정
    pinMode(SENSOR1, INPUT);
    pinMode(SENSOR2, INPUT);
    pinMode(SENSOR3, INPUT);
    pinMode(SENSOR4, INPUT);
}

// I2C를 열고 장치와 연결
int open_I2C(void) 
{
    int file;

    // I2C 장치 파일 열기
    if ((file = open(deviceName, O_RDWR)) < 0) 
    {
        fprintf(stderr, "I2C: Failed to access %s: %s\n", deviceName, strerror(errno));
        return -1;
    }

    printf("I2C: Connected\n");

    // I2C 장치 주소 설정
    printf("I2C: acquiring bus to 0x%x\n", ADDRESS);
    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0) 
    {
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x: %s\n", ADDRESS, strerror(errno));
        close(file);
        return -1;
    }

    return file;
}

// I2C 닫기
void close_I2C(int fd) 
{
    if (fd != -1) 
    {
        close(fd);
        printf("I2C: Closed\n");
    }
}

// 자동차 제어 명령을 I2C로 전송
void car_control(int l_dir, int l_speed, int r_dir, int r_speed) 
{
    // 제어 명령 데이터 배열
    unsigned char data[5] = {0x01, l_dir, l_speed, r_dir, r_speed};

    printf("Sending control command to I2C bus...\n");
    // I2C로 데이터 전송
    if (write(file_I2C, data, 5) != 5) 
    {
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Control command sent successfully.\n");
    }
}

// 자동차 정지 명령을 I2C로 전송
void car_stop() 
{
    // 정지 명령 데이터 배열
    unsigned char data[2] = {0x02, 0x00};

    printf("Sending stop command to I2C bus...\n");
    // I2C로 데이터 전송
    if (write(file_I2C, data, 2) != 2) 
    {
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Stop command sent successfully.\n");
    }
}

// 이전 센서 값 저장 변수 초기화
int prev_sensor1Value = 0;
int prev_sensor2Value = 0;
int prev_sensor3Value = 0;
int prev_sensor4Value = 0;

// 라인트레이싱 로직
void line_trace(int sensor1, int sensor2, int sensor3, int sensor4) 
{
    int l_dir, l_speed, r_dir, r_speed;

    // 기본 설정: 앞으로 전진
    l_dir = 1;  
    r_dir = 1;  
    l_speed = 100;
    r_speed = 100;

    // 센서 값에 따른 속도 조절
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
        // 이전 센서 값을 유지
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

    // 자동차 제어 함수 호출
    car_control(l_dir, l_speed, r_dir, r_speed);

    // 현재 센서 값을 이전 센서 값으로 저장
    prev_sensor1Value = sensor1;
    prev_sensor2Value = sensor2;
    prev_sensor3Value = sensor3;
    prev_sensor4Value = sensor4;
}

// 메인 루프 함수
void loop() 
{
    // 각 센서의 값을 읽어옴
    int sensor1Value = digitalRead(SENSOR1);
    int sensor2Value = digitalRead(SENSOR2);
    int sensor3Value = digitalRead(SENSOR3);
    int sensor4Value = digitalRead(SENSOR4);

    // 센서 값을 출력
    printf("Sensor Values: %d %d %d %d\n", sensor1Value, sensor2Value, sensor3Value, sensor4Value);

    // 라인트레이싱 함수 호출
    line_trace(sensor1Value, sensor2Value, sensor3Value, sensor4Value);
}

// 메인 함수
int main(void) 
{
    // 초기 설정 함수 호출
    setup();

    // I2C 열기
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
    
    // 타이머 초기화
    unsigned long rateTimer = micros(); // 현재 마이크로초 단위 시간
    unsigned long now; // 현재 시간 저장 변수
    unsigned long controlPeriod = 10000;  // 제어 주기 (마이크로초 단위, 10ms)

    // 메인 루프 실행
    while (1) 
    {
        now = micros();
        while ((now - rateTimer) < controlPeriod) 
        {
            now = micros();
        }

        // 루프 함수 호출
        loop();

        // 타이머 갱신
        rateTimer = micros();
    }
    
    // I2C 닫기
    close_I2C(file_I2C);

    return 0;
}
