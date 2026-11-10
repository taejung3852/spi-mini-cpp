#pragma once
#include <opencv2/core.hpp>
#include <string>
#include <vector>

/*
constexpr는 컴파일할 때 값이 정해지는 상수를 정의할 떄 사용한다고한다.-> java에서 static final 느낌
enum class Defect는 결함 종류에 이름을 붙이는 거다. 예를 들면 Defect::Missing 이렇게 이름 앞에 붙인다
cv::Mat&는 복사하지 않고 원본을 넘기는거다 
cv::Mat은 opencv에서 이미지 데이터를 저장하고 처리하는 자료형이다. matrix의 약자
*/

// --- 기본값 ---- 
constexpr float UM_PER_PX = 10.0f;
constexpr int   BOARD_W = 800;
constexpr int   BOARD_H = 600;
constexpr int   PAD_SIZE = 40;
constexpr int   PAD_PITCH = 100;
constexpr int   PAD_COLS = 8;
constexpr int   PAD_ROWS = 6;
constexpr float NORMAL_H = 120.0f;
constexpr int   SHIFT_PX = 15;
constexpr float NOISE_SIGMA = 2.0f;

// 넣을 수 있는 결함 종류 정의
enum class Defect { None, Insufficient, Excess, Missing, Shift, Bridge, BorderOk};

cv::Mat make_blank(int width, int height);
cv::Rect pad_rect(int i);
cv::Mat make_board(); // 빈이미지 생성
void add_defect(cv::Mat& board, std::vector<Defect>& truth, int pad, Defect type);
void add_noise(cv::Mat& board, float sigma, std::uint64_t seed);
std::string defect_name(Defect d);
bool save_heightmap_png(const std::string& path, const cv::Mat& board);
bool save_truth_csv(const std::string& path, const std::vector<Defect>& truth);


