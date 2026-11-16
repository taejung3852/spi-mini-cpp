// 4단계: 평가
// 1) 검출표 - 결함을 잘 잡았나, 놓쳤나, 정상을 괜히 NG로 버렸나(가성불량)
// 2) 노이즈 비교 - 노이즈(σ)를 0, 2, 10µm로 올리면 어떻게 되나
// 3) 처리 시간 - 기판 1장 검사에 몇 ms 걸리나
#include "spi.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

const int BOARDS = 20;   // 노이즈 시드를 바꿔서 기판 20장씩 본다

// main.cpp랑 같은 결함을 같은 자리에 넣은 기판
// (결함 위치는 고정이고 노이즈만 시드마다 달라진다)
cv::Mat make_test_board(std::vector<Defect>& truth, float sigma, int seed) {
    cv::Mat board = make_board();
    truth.assign(PAD_COUNT, Defect::None);
    add_defect(board, truth, 9,  Defect::Insufficient);
    add_defect(board, truth, 12, Defect::Excess);
    add_defect(board, truth, 20, Defect::Missing);
    add_defect(board, truth, 27, Defect::Shift);
    add_defect(board, truth, 33, Defect::Bridge);
    add_defect(board, truth, 41, Defect::BorderOk);
    if (sigma > 0) add_noise(board, sigma, seed);
    return board;
}

// 결함 종류마다 몇 개 넣었고 몇 개 잡았는지 세는 칸
// 순서는 enum Defect 순서랑 같다 (None, Insufficient, Excess, Missing, Shift, Bridge, BorderOk)
struct Count {
    int total = 0;
    int ng = 0;   // NG로 판정한 개수
};

void run_noise_level(float sigma) {
    Count count[7];
    double volErrSum = 0;   // 정상 패드 부피 오차(%p) 합
    int normalPads = 0;

    for (int seed = 1; seed <= BOARDS; ++seed) {
        std::vector<Defect> truth;
        cv::Mat board = make_test_board(truth, sigma, seed);
        std::vector<PadResult> results = inspect_board(board);

        for (const PadResult& r : results) {
            int t = (int)truth[r.pad];
            count[t].total++;
            if (!r.ok) count[t].ng++;
            if (truth[r.pad] == Defect::None) {
                volErrSum += std::abs(r.volume_pct - 100.0);   // 정상은 100%가 정답
                normalPads++;
            }
        }
    }

    std::cout << "\n### σ = " << sigma << "µm (기판 " << BOARDS << "장)\n\n";
    std::cout << "| 종류 | 넣은 개수 | NG로 잡음 | 놓침 / 가성불량 |\n";
    std::cout << "|---|---|---|---|\n";
    for (int t = 1; t <= 5; ++t) {   // 진짜 결함: 잡아야 함
        std::cout << "| " << defect_name((Defect)t) << " | " << count[t].total << " | "
                  << count[t].ng << " | 놓침 " << count[t].total - count[t].ng << " |\n";
    }
    // 정상이랑 경계정상은 OK가 나와야 함. 여기서 NG면 가성불량
    std::cout << "| none (정상) | " << count[0].total << " | - | 가성불량 " << count[0].ng << " |\n";
    std::cout << "| border_ok (85%) | " << count[6].total << " | - | 가성불량 " << count[6].ng << " |\n";
    std::cout << "\n정상 패드 부피 오차 평균: " << volErrSum / normalPads << "%p\n";
}

void measure_time() {
    std::vector<Defect> truth;
    cv::Mat board = make_test_board(truth, NOISE_SIGMA, 42);

    std::vector<double> ms;
    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::steady_clock::now();
        std::vector<PadResult> results = inspect_board(board);
        auto end = std::chrono::steady_clock::now();
        ms.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }
    std::sort(ms.begin(), ms.end());
    double median = (ms[49] + ms[50]) / 2.0;   // 100개라 가운데 두 개 평균
    std::cout << "\n### 처리 시간\n\n기판 1장(패드 48개) 검사 100번 중앙값: " << median << " ms\n";
}

int main() {
    std::cout << "## 평가 결과\n";
    run_noise_level(0.0f);
    run_noise_level(2.0f);
    run_noise_level(10.0f);
    measure_time();
    return 0;
}
