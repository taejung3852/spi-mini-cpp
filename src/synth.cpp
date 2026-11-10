#include "spi.hpp"

cv::Mat make_blank(int width, int height){ // 0으로 채운 이미지
    return cv::Mat::zeros(height, width, CV_32F); // 여기서 CV_32F는 픽셀 하나가 실수라는 거다. 나중에 높이를 담을 형식
}