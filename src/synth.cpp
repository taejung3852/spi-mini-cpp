#include "spi.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>

cv::Mat make_blank(int width, int height) {
    return cv::Mat::zeros(height, width, CV_32F);
}

/*
패드 번호를 받아서 사격형 위치를 돌려주는거다.

함수 생김새:  cv::Rect   pad_rect   (int i)
              ①돌려주는 자료형  ②함수 이름  ③받는 값(패드 번호)
- cv::Rect = OpenCV의 사각형 자료형 (x, y, 가로, 세로). cv:: 는 "OpenCV 소속"이라는 뜻 (Java 패키지 느낌)
- Java로 치면 static Rectangle padRect(int i) { ... } 와 같은 모양
- 선언(;로 끝남)은 spi.hpp에, 실제 본문({ })은 여기 synth.cpp에 있다
- 사용 예: cv::Rect r = pad_rect(9);  // 9번 패드 → (130, 130, 40, 40)
*/
cv::Rect pad_rect(int i) {
    int col = i % PAD_COLS;                       // 몇 번째 열
    int row = i / PAD_COLS;                       // 몇 번째 줄
    int cx = PAD_PITCH / 2 + col * PAD_PITCH;     // 중심 x = 50, 150, ...
    int cy = PAD_PITCH / 2 + row * PAD_PITCH;     // 중심 y
    
    return cv::Rect(cx - PAD_SIZE / 2, cy - PAD_SIZE / 2, PAD_SIZE, PAD_SIZE);
}

/*
빈 기판을 만들고 for 문으로 48번(pad_count)으로 패드 자리에 120을 입력
*/
cv::Mat make_board() {
    cv::Mat board = make_blank(BOARD_W, BOARD_H);
    for (int i = 0; i < PAD_COUNT; ++i) {
        board(pad_rect(i)).setTo(NORMAL_H);       // 패드 영역만 120 입력
    }

    return board;
}


/*

*/
void add_defect(cv::Mat& board, std::vector<Defect>& truth, int pad, Defect type) {
    cv::Rect r = pad_rect(pad);
    switch (type) {
    case Defect::Insufficient:
        board(r).setTo(NORMAL_H * 0.5f);          // 60
        break;
    case Defect::Excess:
        board(r).setTo(NORMAL_H * 1.5f);          // 180
        break;
    case Defect::Missing:
        board(r).setTo(0.0f);
        break;
    case Defect::BorderOk:
        board(r).setTo(NORMAL_H * 0.85f);         // 102, 정답은 OK
        break;
    case Defect::Shift:
        board(r).setTo(0.0f);                             // 원래 자리 지우고
        board(r + cv::Point(SHIFT_PX, 0)).setTo(NORMAL_H); // 오른쪽으로 15칸
        break;
    case Defect::Bridge: {
        CV_Assert(pad % PAD_COLS != PAD_COLS - 1);  // 맨 오른쪽 열이면 옆 패드가 없다
        cv::Rect next = pad_rect(pad + 1);
        int gapX = r.x + r.width;                   // 왼쪽 패드의 오른쪽 끝
        int gapW = next.x - gapX;                   // 빈자리 60칸
        int stripH = 10;                            // 띠 두께
        int stripY = r.y + (r.height - stripH) / 2; // 패드 세로 가운데
        board(cv::Rect(gapX, stripY, gapW, stripH)).setTo(NORMAL_H);
        truth[pad + 1] = Defect::Bridge;            // 오른쪽 패드도 브리지
        break;
    }
    case Defect::None:
        break;
    }
    truth[pad] = type;
}

void add_noise(cv::Mat& board, float sigma, std::uint64_t seed) {
    cv::theRNG().state = seed;                  // 같은 시드 = 같은 노이즈
    cv::Mat noise(board.size(), CV_32F);
    cv::randn(noise, 0.0, sigma);               // 평균 0, 표준편차 sigma
    board += noise;
}

std::string defect_name(Defect d) {
    switch (d) {
    case Defect::None:         return "none";
    case Defect::Insufficient: return "insufficient";
    case Defect::Excess:       return "excess";
    case Defect::Missing:      return "missing";
    case Defect::Shift:        return "shift";
    case Defect::Bridge:       return "bridge";
    case Defect::BorderOk:     return "border_ok";
    }
    return "unknown";
}

bool save_heightmap_png(const std::string& path, const cv::Mat& board) {
    cv::Mat gray, color;
    cv::normalize(board, gray, 0, 255, cv::NORM_MINMAX, CV_8U); // 가장 낮은 값 0, 가장 높은 값 255
    cv::applyColorMap(gray, color, cv::COLORMAP_JET);          // 낮으면 파랑, 높으면 빨강
    return cv::imwrite(path, color);
}

bool save_truth_csv(const std::string& path, const std::vector<Defect>& truth) {
    std::ofstream out(path);
    if (!out) return false;
    out << "pad,defect\n";
    for (std::size_t i = 0; i < truth.size(); ++i) {
        out << i << ',' << defect_name(truth[i]) << '\n';
    }
    return true;
}