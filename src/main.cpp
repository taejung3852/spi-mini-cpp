#include "spi.hpp"
#include <iostream>

int main() {
    cv::Mat board = make_board(); // 빈기판 만들고
    std::vector<Defect> truth(PAD_COUNT, Defect::None);  // 48칸 전부 정상으로 시작 -> 정답지? 라고 생각 48칸을 만들어서 전부 정상으로 채워라!라는 뜻

    add_defect(board, truth, 9,  Defect::Insufficient); // 9번 패드 부분을 부족 상태로 만들기
    add_defect(board, truth, 12, Defect::Excess); // 12번 패드를 과다
    add_defect(board, truth, 20, Defect::Missing); // 누락
    add_defect(board, truth, 27, Defect::Shift); // 틀어짐
    add_defect(board, truth, 33, Defect::Bridge);        // 33번과 34번이 이어짐
    add_defect(board, truth, 41, Defect::BorderOk); // 경계정상 -> 아슬아슬 통과

    add_noise(board, NOISE_SIGMA, 42); // 이미지에 노이즈를 조금 섞는 거다

    bool ok = save_heightmap_png("heightmap.png", board)
           && save_truth_csv("truth.csv", truth); // 둘다 성공하면 true
    std::cout << (ok ? "heightmap.png, truth.csv 저장 성공" : "저장 실패") << std::endl;
    return ok ? 0 : 1;
}