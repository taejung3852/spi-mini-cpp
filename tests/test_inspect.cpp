#include "spi.hpp"
#include <gtest/gtest.h>

TEST(Setup, BlankIsAllZero) {
    cv::Mat m = make_blank(80, 60);
    EXPECT_EQ(m.cols, 80);
    EXPECT_EQ(m.rows, 60);
    EXPECT_EQ(cv::countNonZero(m), 0);
}

TEST(Board, PadRectCorners) {
    EXPECT_EQ(pad_rect(0),  cv::Rect(30, 30, 40, 40));    // 왼쪽 위
    EXPECT_EQ(pad_rect(47), cv::Rect(730, 530, 40, 40));  // 오른쪽 아래
}

TEST(Board, NormalBoardHasAllPads) {
    cv::Mat b = make_board();
    EXPECT_EQ(cv::countNonZero(b), PAD_COUNT * PAD_SIZE * PAD_SIZE);              // 76,800
    EXPECT_DOUBLE_EQ(cv::sum(b)[0], PAD_COUNT * PAD_SIZE * PAD_SIZE * NORMAL_H);  // 9,216,000
}

TEST(Board, DefectsChangeHeightAndTruth) {
    cv::Mat b = make_board();
    std::vector<Defect> truth(PAD_COUNT, Defect::None);
    add_defect(b, truth, 20, Defect::Missing);
    add_defect(b, truth, 33, Defect::Bridge);
    EXPECT_EQ(cv::countNonZero(b(pad_rect(20))), 0);
    EXPECT_EQ(truth[33], Defect::Bridge);
    EXPECT_EQ(truth[34], Defect::Bridge);
}