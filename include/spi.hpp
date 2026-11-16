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
constexpr int   PAD_COUNT   = PAD_COLS * PAD_ROWS;  // 48
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


// ---- 2단계: 측정·판정 기준 (예시값, 계획서 5장) ----
constexpr int    WIN_MARGIN    = 20;     // 검사창 = 패드 + 사방 20칸
constexpr float  SOLDER_MIN_H  = 30.0f;  // 30µm 넘는 칸만 솔더로 센다
constexpr double TOL_LO        = 70.0;   // 허용 범위 아래 (%)
constexpr double TOL_HI        = 130.0;  // 허용 범위 위 (%)
constexpr double OFFSET_MAX_UM = 60.0;   // 허용 어긋남 (µm)

// 패드 하나를 잰 결과
struct PadResult {
    int pad = 0;
    double area_pct = 0;    // 면적: 정상 대비 %
    double height_pct = 0;  // 높이: 솔더 칸 평균 높이, 정상 대비 %
    double volume_pct = 0;  // 부피: 높이 합, 정상 대비 %
    double offset_um = 0;   // 무게중심 − 패드 중심 거리 (µm)
    bool bridge = false;    // 옆 패드와 이어졌나
    bool ok = true;         // 최종 판정
    std::string reason;     // NG 사유 (예: "volume 50%")
};

cv::Rect window_rect(int i);
PadResult measure_pad(const cv::Mat& board, int pad);
std::vector<bool> find_bridges(const cv::Mat& board);
void judge(PadResult& r);
std::vector<PadResult> inspect_board(const cv::Mat& board);
bool save_result_csv(const std::string& path, const std::vector<PadResult>& results);
bool save_overlay_png(const std::string& path, const cv::Mat& board, const std::vector<PadResult>& results);