#include <stdio.h>                // 표준 입출력 라이브러리
#include <wiringPi.h>             // WiringPi 라이브러리
#include <unistd.h>               // 유닉스 표준 라이브러리
#include <errno.h>                // 오류 번호 처리 라이브러리
#include <linux/i2c-dev.h>        // I2C 장치 인터페이스
#include <sys/ioctl.h>            // IO 제어 라이브러리
#include <fcntl.h>                // 파일 제어 라이브러리
#include <string.h>               // 문자열 처리 라이브러리

#define SENSOR1 2  // 물리적 핀 번호 7에 해당하는 WiringPi 핀 번호
#define SENSOR2 3  // 물리적 핀 번호 11에 해당하는 WiringPi 핀 번호
#define SENSOR3 0  // 물리적 핀 번호 15에 해당하는 WiringPi 핀 번호
#define SENSOR4 7  // 물리적 핀 번호 13에 해당하는 WiringPi 핀 번호

#define ADDRESS 0x16    // I2C 장치 주소
static const char *deviceName = "/dev/i2c-1";  // I2C 버스 이름

int file_I2C = -1;  // I2C 파일 디스크립터 초기화

// GPIO 및 I2C 설정 함수
void setup() 
{
    // WiringPi 설정 초기화
    if (wiringPiSetup() == -1) 
    {
        // 설정 오류 시 에러 메시지 출력
        fprintf(stderr, "wiringPi Setup error: %s\n", strerror(errno));
        return;
    }
    
    // 각 센서 핀을 입력 모드로 설정
    pinMode(SENSOR1, INPUT);
    pinMode(SENSOR2, INPUT);
    pinMode(SENSOR3, INPUT);
    pinMode(SENSOR4, INPUT);
}

// I2C 장치 열기 함수
int open_I2C(void) 
{
    int file;

    // I2C 장치 파일 열기
    if ((file = open(deviceName, O_RDWR)) < 0) 
    {
        // 파일 열기 실패 시 에러 메시지 출력
        fprintf(stderr, "I2C: Failed to access %s: %s\n", deviceName, strerror(errno));
        return -1;
    }

    printf("I2C: Connected\n");  // I2C 연결 성공 메시지 출력

    printf("I2C: acquiring bus to 0x%x\n", ADDRESS);  // I2C 버스 획득 시도 메시지 출력

    // I2C 버스에 접근하여 슬레이브 장치 설정
    if (ioctl(file, I2C_SLAVE, ADDRESS) < 0) 
    {
        // 버스 접근 실패 시 에러 메시지 출력
        fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x: %s\n", ADDRESS, strerror(errno));
        close(file);  // 파일 닫기
        return -1;
    }

    return file;  // 성공 시 파일 디스크립터 반환
}

// I2C 장치 닫기 함수
void close_I2C(int fd) 
{
    if (fd != -1)  // 파일 디스크립터가 유효할 경우
    {
        close(fd);  // 파일 닫기
        printf("I2C: Closed\n");  // I2C 닫기 성공 메시지 출력
    }
}

// 차량 제어 함수
void car_control(int l_dir, int l_speed, int r_dir, int r_speed) 
{
    unsigned char data[5] = {0x01, l_dir, l_speed, r_dir, r_speed};  // I2C로 보낼 데이터 배열

    printf("Sending control command to I2C bus...\n");  // 제어 명령 전송 메시지 출력
    if (write(file_I2C, data, 5) != 5)  // I2C 버스로 데이터 전송
    {
        // 전송 실패 시 에러 메시지 출력
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Control command sent successfully.\n");  // 전송 성공 메시지 출력
    }
}

// 차량 정지 함수
void car_stop() 
{
    unsigned char data[2] = {0x02, 0x00};  // 정지 명령 데이터 배열

    printf("Sending stop command to I2C bus...\n");  // 정지 명령 전송 메시지 출력
    if (write(file_I2C, data, 2) != 2)  // I2C 버스로 데이터 전송
    {
        // 전송 실패 시 에러 메시지 출력
        fprintf(stderr, "Failed to write to the i2c bus: %s\n", strerror(errno));
    } 
    else 
    {
        printf("Stop command sent successfully.\n");  // 전송 성공 메시지 출력
    }
}

// 이전 센서 값을 저장할 전역 변수
int prev_sensor1Value = 0;
int prev_sensor2Value = 0;
int prev_sensor3Value = 0;
int prev_sensor4Value = 0;

// 라인트레이싱 함수
void line_trace(int sensor1, int sensor2, int sensor3, int sensor4) 
{
    int l_dir, l_speed, r_dir, r_speed;  // 왼쪽, 오른쪽 방향 및 속도 변수

    l_dir = 1;  // 왼쪽 방향 기본 값 설정
    r_dir = 1;  // 오른쪽 방향 기본 값 설정
    l_speed = 100;  // 왼쪽 속도 기본 값 설정
    r_speed = 100;  // 오른쪽 속도 기본 값 설정

    // 센서 값에 따른 차량 제어 로직
    if (sensor4 == 1 && sensor3 == 0 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 130;  // 왼쪽 속도 증가
        r_speed = 20;   // 오른쪽 속도 감소
    } 
    else if (sensor4 == 1 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 130;  // 왼쪽 속도 증가
        r_speed = 70;   // 오른쪽 속도 감소
    }
    else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
    {
        l_speed = 90;  // 왼쪽 속도 감소
        r_speed = 40;  // 오른쪽 속도 감소
    } 
    else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 1 && sensor1 == 0) 
    {
        l_speed = 200;  // 왼쪽 속도 증가
        r_speed = 190;  // 오른쪽 속도 증가
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 0) 
    {
        l_speed = 30;  // 왼쪽 속도 감소
        r_speed = 140;  // 오른쪽 속도 증가
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 1) 
    {
        l_speed = 30;  // 왼쪽 속도 감소
        r_speed = 150;  // 오른쪽 속도 증가
    }
    else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 0 && sensor1 == 1) 
    {
        l_speed = 20;  // 왼쪽 속도 감소
        r_speed = 150;  // 오른쪽 속도 증가
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
            l_speed = 130;  // 왼쪽 속도 증가
            r_speed = 20;   // 오른쪽 속도 감소
        } 
        else if (sensor4 == 1 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
        {
            l_speed = 110;  // 왼쪽 속도 감소
            r_speed = 30;   // 오른쪽 속도 감소
        }
        else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 0 && sensor1 == 0) 
        {
            l_speed = 130;  // 왼쪽 속도 증가
            r_speed = 70;   // 오른쪽 속도 감소
        } 
        else if (sensor4 == 0 && sensor3 == 1 && sensor2 == 1 && sensor1 == 0) 
        {
            l_speed = 200;  // 왼쪽 속도 증가
            r_speed = 190;  // 오른쪽 속도 증가
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 0) 
        {
            l_speed = 30;  // 왼쪽 속도 감소
            r_speed = 140;  // 오른쪽 속도 증가
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 1 && sensor1 == 1) 
        {
            l_speed = 30;  // 왼쪽 속도 감소
            r_speed = 150;  // 오른쪽 속도 증가
        }
        else if (sensor4 == 0 && sensor3 == 0 && sensor2 == 0 && sensor1 == 1) 
        {
            l_speed = 20;  // 왼쪽 속도 감소
            r_speed = 150;  // 오른쪽 속도 증가
        }
    }

    car_control(l_dir, l_speed, r_dir, r_speed);  // 차량 제어 함수 호출

    // 현재 센서 값을 이전 센서 값으로 저장
    prev_sensor1Value = sensor1;
    prev_sensor2Value = sensor2;
    prev_sensor3Value = sensor3;
    prev_sensor4Value = sensor4;
}

// 주기적으로 센서 값을 읽고 라인트레이싱 함수 호출
void loop() 
{
    int sensor1Value = digitalRead(SENSOR1);  // SENSOR1 값 읽기
    int sensor2Value = digitalRead(SENSOR2);  // SENSOR2 값 읽기
    int sensor3Value = digitalRead(SENSOR3);  // SENSOR3 값 읽기
    int sensor4Value = digitalRead(SENSOR4);  // SENSOR4 값 읽기

    printf("Sensor Values: %d %d %d %d\n", sensor1Value, sensor2Value, sensor3Value, sensor4Value);  // 센서 값 출력

    line_trace(sensor1Value, sensor2Value, sensor3Value, sensor4Value);  // 라인트레이싱 함수 호출
}

// 메인 함수
int main(void) 
{
    setup();  // GPIO 및 I2C 설정

    file_I2C = open_I2C();  // I2C 장치 열기

    if (file_I2C < 0) 
    {
        printf("Unable to open I2C\n");  // I2C 열기 실패 시 메시지 출력
        return -1;
    } 
    else 
    {
        printf("I2C is Connected\n");  // I2C 열기 성공 시 메시지 출력
    }

    while (1)  // 무한 루프
    {
        loop();  // 주기적으로 센서 값 읽기 및 라인트레이싱 함수 호출
    }

    close_I2C(file_I2C);  // I2C 장치 닫기

    return 0;
}
