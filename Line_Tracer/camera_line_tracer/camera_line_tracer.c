#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <termio.h>
#include <softPwm.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

using namespace cv;
using namespace std;

#define IMG_Width  640
#define IMG_Height 480

#define ASSIST_BASE_LINE  320
#define ASSIST_BASE_WIDTH  40

#define ENA 6  // Physerial 
#define IN1 4  // Physerial 
#define IN2 5  // Physerial 

#define ENB 0  // Physerial 
#define IN3 2  // Physerial 
#define IN4 3  // Physerial 

#define TRIG 21
#define ECHO 22

int guide_width1  = 50;
int guide_height1 = 20;
int guide_center  = IMG_Width / 2;
int line_center   = -1;

int P_term = 0;
int D_term = 0; 

#define MAX_PWM_DUTY   200
#define BASE_SPEED      70
#define STRAIGHT_SPEED  70
#define RIGHT_SPEED    45
#define LEFT_SPEED     55
#define P_s_gain     0.12 //0.126
#define D_s_gain     0.12 //0.098
#define P_rc_gain    0.096
#define D_rc_gain    0.094
#define P_lc_gain    0.098
#define D_lc_gain    0.075

#define ADDRESS 0x16
static const char *deviceName = "/dev/i2c-1";

int file_I2C;

int getch(void)
{
    int ch;
    struct termios buf;
    struct termios save;

    tcgetattr(0, &save);
    buf = save;
    buf.c_lflag &= ~(ICANON | ECHO);
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &buf);
    ch = getchar();
    tcsetattr(0, TCSAFLUSH, &save);
    return ch;
}

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

float ultrasonic_sensor(void)
{
    long start_time, end_time;
    long temp_time1, temp_time2;
    int duration;
    float distance;

    digitalWrite(TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    delayMicroseconds(200);
    temp_time1 = micros();

    while (digitalRead(ECHO) == LOW)
    {
        temp_time2 = micros();
        duration = temp_time2 - temp_time1;
        if (duration > 1000)
            return -1;
    }

    start_time = micros();

    while (digitalRead(ECHO) == HIGH)
    {
        temp_time2 = micros();
        duration = temp_time2 - temp_time1;
        if (duration > 2000)
            return -1;
    }
    end_time = micros();

    duration = end_time - start_time;
    distance = duration / 58;

    return distance;
}

Mat region_of_interest(Mat img, Point *points)
{
    Mat img_mask = Mat::zeros(img.rows, img.cols, CV_8UC1);
    Scalar mask_color = Scalar(255, 255, 255);
    const Point *pt[1] = {points};
    int npt[] = {4};

    fillPoly(img_mask, pt, npt, 1, Scalar(255, 255, 255), LINE_8);

    Mat masked_img;
    bitwise_and(img, img_mask, masked_img);

    return masked_img;
}

Mat Canny_Edge_Detection(Mat img)
{
    Mat mat_blur_img, mat_canny_img;
    blur(img, mat_blur_img, Size(3, 3));
    Canny(mat_blur_img, mat_canny_img, 50, 165, 3);

    return mat_canny_img;
}

Mat Draw_Guide_Line(Mat img)
{
    // 입력된 이미지를 복사하여 새로운 이미지 객체를 생성합니다.
    Mat result_img;
    img.copyTo(result_img);

    // Increase the width of the pink rectangle
    int pink_rectangle_margin = 100; // New margin to increase the width of the rectangle

    // 기준 선을 그립니다. 세로 높이를 절반으로 줄였습니다.
    rectangle(result_img, Point(pink_rectangle_margin, ASSIST_BASE_LINE - ASSIST_BASE_WIDTH / 2),
              Point(IMG_Width - pink_rectangle_margin, ASSIST_BASE_LINE + ASSIST_BASE_WIDTH / 2),
              Scalar(152, 160, 237), 1, LINE_AA);

    // Draw horizontal guide lines extending from the center
    line(result_img, Point(guide_center - guide_width1, ASSIST_BASE_LINE),
         Point(guide_center, ASSIST_BASE_LINE), Scalar(0, 255, 255), 1, 0);
    line(result_img, Point(guide_center, ASSIST_BASE_LINE),
         Point(guide_center + guide_width1, ASSIST_BASE_LINE), Scalar(0, 255, 255), 1, 0);

    // 왼쪽 가이드 라인의 세로선을 가로선의 끝에 맞춥니다.
    line(result_img, Point(guide_center - guide_width1, ASSIST_BASE_LINE - guide_height1),
         Point(guide_center - guide_width1, ASSIST_BASE_LINE + guide_height1), Scalar(0, 255, 255), 1, 0);
    // 오른쪽 가이드 라인의 세로선을 가로선의 끝에 맞춥니다.
    line(result_img, Point(guide_center + guide_width1, ASSIST_BASE_LINE - guide_height1),
         Point(guide_center + guide_width1, ASSIST_BASE_LINE + guide_height1), Scalar(0, 255, 255), 1, 0);

    // 이미지 중앙에 수직선을 그려 화면의 중심을 나타냅니다.
    line(result_img, Point(IMG_Width / 2, ASSIST_BASE_LINE - guide_height1 * 1),
         Point(IMG_Width / 2, ASSIST_BASE_LINE + guide_height1 * 1), Scalar(255), 2, 0);

    // 검출된 라인의 중심을 나타내는 수직선을 그립니다.
    line(result_img, Point(line_center, ASSIST_BASE_LINE - guide_height1 * 1),
         Point(line_center, ASSIST_BASE_LINE + guide_height1 * 1), Scalar(0, 0, 255), 2, 0);

    // 가이드 라인이 추가된 이미지를 반환합니다.
    return result_img;
}


void line_tracer_motor_control(int line_center)
{
    static int prev_error = 0; // 이전 오차값 저장
    int pwm_r, pwm_l;
    int error = line_center - guide_center; // 현재 오차 계산

    // Check if the error is within the range to consider the line centered
    if (error >= -1 && error <= 1) 
    {
        P_term = (int)(P_s_gain * error);
        D_term = (int)(D_s_gain * (error - prev_error));
        // Set both motors to a speed of 100 for straight movement
        pwm_r = RIGHT_SPEED; 
        pwm_l = LEFT_SPEED;
    }
    else if (error >= 1)
    {
        // 비례 제어와 미분 제어를 통한 속도 조절
        P_term = (int)(P_rc_gain * error);
        D_term = (int)(D_rc_gain * (error - prev_error));

        int control = P_term + D_term;

        // 오른쪽과 왼쪽 모터 속도를 계산하여 조절
        pwm_r = RIGHT_SPEED - control;
        pwm_l = RIGHT_SPEED + control;

        // PWM 값을 범위 내로 제한
        if (pwm_r > MAX_PWM_DUTY) pwm_r = MAX_PWM_DUTY;
        if (pwm_r < 0) pwm_r = 0;
        if (pwm_l > MAX_PWM_DUTY) pwm_l = MAX_PWM_DUTY;
        if (pwm_l < 0) pwm_l = 0;
    }
    else
    {
        // 비례 제어와 미분 제어를 통한 속도 조절
        P_term = (int)(P_lc_gain * error);
        D_term = (int)(D_lc_gain * (error - prev_error));

        int control = P_term + D_term;

        // 오른쪽과 왼쪽 모터 속도를 계산하여 조절
        pwm_r = LEFT_SPEED - control;
        pwm_l = LEFT_SPEED + control;

        // PWM 값을 범위 내로 제한
        if (pwm_r > MAX_PWM_DUTY) pwm_r = MAX_PWM_DUTY;
        if (pwm_r < 0) pwm_r = 0;
        if (pwm_l > MAX_PWM_DUTY) pwm_l = MAX_PWM_DUTY;
        if (pwm_l < 0) pwm_l = 0;
    }
        

    printf("PWM L : %3d | PWM R : %3d | Error: %3d\n", pwm_l, pwm_r, error);

    car_control(1, pwm_l, 1, pwm_r); // I2C를 사용하여 모터 제어

    prev_error = error; // 현재 오차를 이전 오차로 업데이트
}


void sig_Handler(int sig)
{
    printf("\n\n\n\nProgram and Motor Stop\n\n\n");
    car_stop();
    exit(0);
}

int main(void)
{
    int fd;
    int pwm_r = 0;
    int pwm_l = 0;
    unsigned char test;

    int img_width, img_height;
    Point points[4];
    int no_label;
    int c_x, c_y;
    int c_x_sum = 0;0;
    int steer_error = 0;

    img_width = 640;
    img_height = 480;
    c_x = c_y = 0;

    Mat mat_image_org_color_Overlay;
    Mat mat_image_org_color;
    Mat mat_image_org_gray;
    Mat mat_image_org_gray_result;
    Mat mat_image_canny_edge;
    Mat mat_image_roi_canny_edge;
    Mat mat_image_line_image = Mat(IMG_Height, IMG_Width, CV_8UC1, Scalar(0));
    Mat image;
    Mat img_labels, stats, centroids;

    Scalar GREEN(0, 255, 0);
    Scalar RED(0, 0, 255);
    Scalar BLUE(255, 0, 0);
    Scalar YELLOW(0, 255, 255);

    ////////////////////////////////////////////////////////////////////////////

    VideoCapture cap(0);

    cap.set(CV_CAP_PROP_FRAME_WIDTH, img_width);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, img_height);

    if (!cap.isOpened())
    {
        cerr << "에러 - 카메라를 열 수 없습니다. \n";
        mat_image_org_color = imread("/home/pi/Team_aMAP/Line_Tracer/3/images/line_1.jpg");
    }
    else
    {
        cap.read(mat_image_org_color);
    }

    mat_image_org_color.copyTo(mat_image_org_color_Overlay);

    if (mat_image_org_color.empty())
    {
        cerr << "빈 영상입니다. \n";
        return -1;
    }

    img_width = mat_image_org_color.size().width;
    img_height = mat_image_org_color.size().height;

    namedWindow("Display window", WINDOW_NORMAL);
    resizeWindow("Display window", img_width, img_height);
    moveWindow("Display window", 10, 10);

    namedWindow("ROI Edge window", WINDOW_NORMAL);
    resizeWindow("ROI Edge window", img_width, img_height);
    moveWindow("ROI Edge window", 700, 10);

    namedWindow("Line Image window", WINDOW_NORMAL);
    resizeWindow("Line Image window", img_width, img_height);
    moveWindow("Line Image window", 10, 500);

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

    signal(SIGINT, sig_Handler);
    test = 'B';

    points[0] = Point(0, ASSIST_BASE_LINE - ASSIST_BASE_WIDTH);
    points[1] = Point(0, ASSIST_BASE_LINE + ASSIST_BASE_WIDTH);
    points[2] = Point(IMG_Width, ASSIST_BASE_LINE + ASSIST_BASE_WIDTH);
    points[3] = Point(IMG_Width, ASSIST_BASE_LINE - ASSIST_BASE_WIDTH);

    while (1)
    {
        if (!cap.isOpened())
        {
            cerr << "에러 - 카메라를 열 수 없습니다. \n";
            car_stop();
            return -1;
        }
        mat_image_line_image = Scalar(0);
        cap.read(mat_image_org_color);

        cvtColor(mat_image_org_color, mat_image_org_gray, COLOR_RGB2GRAY);
        threshold(mat_image_org_gray, mat_image_canny_edge, 200, 255, THRESH_BINARY);
        mat_image_canny_edge = Canny_Edge_Detection(mat_image_org_gray);
        mat_image_roi_canny_edge = region_of_interest(mat_image_canny_edge, points);

        vector<Vec4i> linesP;
        HoughLinesP(mat_image_roi_canny_edge, linesP, 1, CV_PI / 180, 30, 15, 10);
        printf("Line Number : %3d\n", linesP.size());
        for (int i = 0; i < linesP.size(); i++)
        {
            Vec4i L = linesP[i];
            line(mat_image_line_image, Point(L[0], L[1]), Point(L[2], L[3]), Scalar(255), 4, LINE_AA);
        }

        if (linesP.size() != 0)
        {
            no_label = connectedComponentsWithStats(mat_image_line_image, img_labels, stats, centroids, 8, CV_32S);
            printf("no label : %3d\n", no_label);
            c_x_sum = 0;
            for (int i = 1; i < no_label; i++)
            {
                int area = stats.at<int>(i, CC_STAT_AREA);

                c_x = centroids.at<double>(i, 0);
                c_y = centroids.at<double>(i, 1);

                c_x_sum = c_x_sum + c_x;
            }

            line_center = c_x_sum / (no_label - 1);
            printf("Centroid Center : %3d \n", line_center);

            line_tracer_motor_control(line_center);
        }

        mat_image_org_color_Overlay = Draw_Guide_Line(mat_image_org_color);

        imshow("Display window", mat_image_org_color_Overlay);
        imshow("ROI Edge window", mat_image_roi_canny_edge);
        imshow("Line Image window", mat_image_line_image);

        if (waitKey(10) > 0)
            break;
    }

    car_stop();
    close_I2C(file_I2C);

    return 0;
}
