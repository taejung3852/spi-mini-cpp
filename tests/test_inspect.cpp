#include "spi.hpp"
#include <gtest/gtest.h>

TEST(Setup, BlankIsAllZero) { // 테스트 하나 묶음이다
    cv::Mat m = make_blank(80, 60);
    EXPECT_EQ(m.cols, 80); // 두 값이 같아야 통과다 (실제, 기대)
    EXPECT_EQ(m.rows, 60);
    EXPECT_EQ(cv::countNonZero(m), 0); // d여기서 countNonZero는 0이 아닌 픽셀수
}