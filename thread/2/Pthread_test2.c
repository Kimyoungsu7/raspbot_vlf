#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <wiringPi.h>
#include <signal.h>
#include <softPwm.h>


#define OBSTACLE_DISTANCE 10
//GPIO Motor Control
#define ENA 26 // Physical pin 37
#define IN1 4  // Physical pin 16
#define IN2 5  // Physical pin 18

#define ENB 0  // Physical pin 11
#define IN3 2  // Physical pin 13
#define IN4 3  // Physical pin 15

#define MAX_PWM_DUTY 100

///////////////////Ultrasonic Sensor///////////////

#define TRIG 21  
#define ECHO 22

void sig_Handler(int sig);

float u_sensor_data = 0;
int flag_obstacle_detection = 0;
int pwm_r, pwm_l;

void motor_control_r(int pwm)
{
    if (pwm > 0)
    {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        softPwmWrite(ENA, pwm);
    }
    else if (pwm == 0)
    {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        softPwmWrite(ENA, 0);
    }
    else
    {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        softPwmWrite(ENA, -pwm);
    }
}

void motor_control_l(int pwm)
{
    if (pwm > 0)
    {
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        softPwmWrite(ENB, pwm);
    }
    else if (pwm == 0)
    {
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        softPwmWrite(ENB, 0);
    }
    else
    {
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        softPwmWrite(ENB, -pwm);
    }
}


int GPIO_control_setup(void)
{
    if (wiringPiSetup() == -1)
    {
        printf("wiringPi Setup error!\n");
        return -1;
    }

    pinMode(ENA, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    pinMode(ENB, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    softPwmCreate(ENA, 1, MAX_PWM_DUTY);
    softPwmCreate(ENB, 1, MAX_PWM_DUTY);

    softPwmWrite(ENA, 0);
    softPwmWrite(ENB, 0);
    
    pwm_l = pwm_r = 0;
    return 0;
}

float ultrasonic_sensor()
{
    long start_time, end_time;
    long temp_time1, temp_time2;
    int duration = -1;
    float distance = 0;
    
    digitalWrite(TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    
    delayMicroseconds(200);//wait for burst signal. 40kHz x 8 = 8x25us = 200
    //printf("200msec \n");
    temp_time1 = micros();
    printf("%d\n", temp_time1);
    while(digitalRead(ECHO) == LOW) //wait unitl ECHO pin is High
    {
        temp_time2 = micros();
        duration = temp_time2 - temp_time1;
        if(duration > 1000) return -1;
    }
  
    start_time = micros();
    //printf("echo signal hight \n");
    while(digitalRead(ECHO) == HIGH) //wait unitl ECHO pin is Low
    {
        temp_time2 = micros();
        duration = temp_time2 - start_time;
        if(duration > 4000) return -1;
    }
    end_time = micros();
    //printf("echo signal low \n");
   
        duration = end_time - start_time;
        distance = duration/58;
        
        return distance;
}

void* ultrasonic_sensor_thread(void* num)
{
    while(1)
    {
        u_sensor_data = ultrasonic_sensor();
        if(u_sensor_data <= OBSTACLE_DISTANCE)
        {
            printf("Obstacle detecte !! \n\n");
            flag_obstacle_detection = 1;
        }
    }
        //pthread_exit(NULL);
}

void* motor_control_thread(void* num)
{
    while(1)
    {
        motor_control_r(pwm_r);
        motor_control_l(pwm_l);
    }
}




int main()
{
    pthread_t pthread_A, pthread_B;
    
    int cnt = 0;
    
    if(GPIO_control_setup() == -1)
    {
        return -1;
    }
    signal(SIGINT, sig_Handler);
    
    
    printf("Create Thread A \n");
    pthread_create(&pthread_A, NULL, ultrasonic_sensor_thread, NULL);
    printf("Create Thread B \n");
    pthread_create(&pthread_B, NULL, motor_control_thread, NULL);
    
    //pthread_join(ultrasonic_sensor_thread, NULL);
    //pthread_join(pthread_B, NULL);
    
    
    while(1)
    {
        printf("Ultrasonic Sensor : %6.3lf[cm] \n", u_sensor_data);
        printf("Thread test: %3d ", cnt);
        cnt++;
        cnt = cnt % 100;
        pwm_l = pwm_r = cnt;
        delay(100);
    }
    
    return 1;
}

void sig_Handler(int sig)
{
    printf("\n\n\n\n\nProgram and Motor Stop! \n\n\n\n");
    //motor_control_r(0);
    //motor_control_l(0);
    exit(0);
}
