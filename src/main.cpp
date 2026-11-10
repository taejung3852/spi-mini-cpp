#include "spi.hpp"
#include <opencv2/imgcodecs.hpp>
#include <iostream>

int main() {
    cv::Mat img;
    make_blank(80, 60).convertTo(img, CV_8U); // 실수 이미지를 0~255 정수 이미지 전환 -> png는 실수 이미지를 저장하지 못해서 필요
    bool ok = cv::imwrite("blank.png", img);
    std::cout << (ok ? "blank.png 저장 성공" : "저장 실패") << std::endl;
    return ok ? 0 : 1; // 성공하면 0 실패하면 1
}