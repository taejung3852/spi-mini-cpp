#include "spi.hpp"
#include <gtest/gtest.h>

TEST(Setup, BlankIsAllZero) { // 빈 이미지가 잘만들어지나? 테스트하는거다. 
    cv::Mat m = make_blank(80, 60);
    EXPECT_EQ(m.cols, 80); // cols가 80이면 통과
    EXPECT_EQ(m.rows, 60);
    EXPECT_EQ(cv::countNonZero(m), 0); // 전부 0인지?
}

TEST(Board, PadRectCorners) { // 경계값 분석
    EXPECT_EQ(pad_rect(0),  cv::Rect(30, 30, 40, 40));    // 왼쪽 위
    EXPECT_EQ(pad_rect(47), cv::Rect(730, 530, 40, 40));  // 오른쪽 아래
}

TEST(Board, NormalBoardHasAllPads) { // 48개가 다 그려졌는지 확인하는거다.
    cv::Mat b = make_board();
    EXPECT_EQ(cv::countNonZero(b), PAD_COUNT * PAD_SIZE * PAD_SIZE);              // 76,800
    EXPECT_DOUBLE_EQ(cv::sum(b)[0], PAD_COUNT * PAD_SIZE * PAD_SIZE * NORMAL_H);  // 9,216,000
}

TEST(Board, DefectsChangeHeightAndTruth) { // 결함이 기판과 정답지 양쪽에 들어가나?
    cv::Mat b = make_board();
    std::vector<Defect> truth(PAD_COUNT, Defect::None);
    add_defect(b, truth, 20, Defect::Missing);
    add_defect(b, truth, 33, Defect::Bridge);
    EXPECT_EQ(cv::countNonZero(b(pad_rect(20))), 0);
    EXPECT_EQ(truth[33], Defect::Bridge);
    EXPECT_EQ(truth[34], Defect::Bridge);
}

TEST(Inspect, CleanBoardAllOk) {
    cv::Mat b = make_board();                          // 결함·노이즈 없음
    for (const PadResult& r : inspect_board(b)) {
        EXPECT_TRUE(r.ok) << r.pad << "번: " << r.reason;
        EXPECT_DOUBLE_EQ(r.volume_pct, 100.0);
    }
}

TEST(Inspect, EachDefectGetsRightReason) {
    cv::Mat b = make_board();
    std::vector<Defect> truth(PAD_COUNT, Defect::None);
    add_defect(b, truth, 9,  Defect::Insufficient);
    add_defect(b, truth, 20, Defect::Missing);
    add_defect(b, truth, 27, Defect::Shift);
    add_defect(b, truth, 33, Defect::Bridge);
    add_defect(b, truth, 41, Defect::BorderOk);
    std::vector<PadResult> res = inspect_board(b);

    EXPECT_NEAR(res[9].volume_pct, 50.0, 0.01);        // 부족: 부피 50%
    EXPECT_FALSE(res[9].ok);
    EXPECT_EQ(res[20].area_pct, 0.0);                  // 누락: 솔더 칸 0
    EXPECT_FALSE(res[20].ok);
    EXPECT_NEAR(res[27].offset_um, 150.0, 0.01);       // 틀어짐: 15칸 = 150µm
    EXPECT_FALSE(res[27].ok);
    EXPECT_TRUE(res[33].bridge);                       // 브리지: 두 패드 모두
    EXPECT_TRUE(res[34].bridge);
    EXPECT_TRUE(res[41].ok);                           // 경계 정상 85%: 통과해야 함
}

// ---- 3단계: 테스트 더 추가 ----

// 손으로 계산할 수 있는 입력: 빈 판에 0번 패드만 높이 60µm 상자를 놓는다
// 60은 정상(120)의 절반이니까 면적 100%, 높이 50%, 부피 50%가 나와야 한다
TEST(Measure, SingleBoxByHand) {
    cv::Mat b = make_blank(BOARD_W, BOARD_H);
    b(pad_rect(0)).setTo(60.0f);   // 패드 자리만 60으로 채우기

    PadResult r = measure_pad(b, 0);
    EXPECT_DOUBLE_EQ(r.area_pct, 100.0);
    EXPECT_DOUBLE_EQ(r.height_pct, 50.0);
    EXPECT_DOUBLE_EQ(r.volume_pct, 50.0);
    EXPECT_DOUBLE_EQ(r.offset_um, 0.0);   // 딱 패드 위에 있으니까 안 밀림
}

// 경계값 분석 (ISTQB): 70%, 130% 바로 안쪽과 바깥쪽을 확인
// 판정만 보고 싶어서 측정은 안 하고 PadResult에 숫자를 직접 넣었다
static PadResult make_result(double volume_pct, double offset_um) {
    PadResult r;
    r.area_pct = 100.0;
    r.height_pct = 100.0;
    r.volume_pct = volume_pct;
    r.offset_um = offset_um;
    judge(r);
    return r;
}

TEST(Judge, VolumeBoundary) {
    EXPECT_FALSE(make_result(69.9, 0).ok);   // 70 아래 -> NG
    EXPECT_TRUE(make_result(70.0, 0).ok);    // 70 딱 -> OK (경계는 포함)
    EXPECT_TRUE(make_result(130.0, 0).ok);   // 130 딱 -> OK
    EXPECT_FALSE(make_result(130.1, 0).ok);  // 130 위 -> NG
}

TEST(Judge, OffsetBoundary) {
    EXPECT_TRUE(make_result(100.0, 60.0).ok);    // 60µm 딱 -> OK
    EXPECT_FALSE(make_result(100.0, 60.1).ok);   // 넘으면 NG
}

// 예외 상황: 솔더가 하나도 없는 판 -> 죽지 않고 48개 전부 NG(면적 0)
TEST(Inspect, BlankBoardDoesNotCrash) {
    cv::Mat b = make_blank(BOARD_W, BOARD_H);
    std::vector<PadResult> res = inspect_board(b);
    ASSERT_EQ(res.size(), PAD_COUNT);
    for (const PadResult& r : res) {
        EXPECT_FALSE(r.ok);
        EXPECT_EQ(r.area_pct, 0.0);
    }
}

// 검사창이 판 밖으로 나가면 at<float>()이 이상한 곳을 읽는다
// 제일 구석 패드(0번, 47번)의 창이 판 안에 있는지 확인
TEST(Window, CornerWindowsInsideBoard) {
    cv::Rect boardRect(0, 0, BOARD_W, BOARD_H);
    EXPECT_EQ(window_rect(0) & boardRect, window_rect(0));    // 겹친 부분 = 창 전체면 안쪽
    EXPECT_EQ(window_rect(47) & boardRect, window_rect(47));
}
