#include "spi.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

// 검사창: 패드 사각형을 사방으로 20칸씩 넓힌 것
cv::Rect window_rect(int i) {
    cv::Rect r = pad_rect(i);
    return cv::Rect(r.x - WIN_MARGIN, r.y - WIN_MARGIN,
                    r.width + 2 * WIN_MARGIN, r.height + 2 * WIN_MARGIN);
}

// 검사창 안을 한 칸씩 돌며 솔더(30µm 초과) 칸을 세고 더한다
PadResult measure_pad(const cv::Mat& board, int pad) {
    PadResult res;
    res.pad = pad;
    cv::Rect w = window_rect(pad);

    int count = 0;          // 솔더 칸 수
    double sumH = 0;        // 높이 합
    double sumX = 0, sumY = 0;  // 무게중심 계산용 좌표 합

    for (int y = w.y; y < w.y + w.height; ++y) {
        for (int x = w.x; x < w.x + w.width; ++x) {
            float h = board.at<float>(y, x);   // (줄, 열) 순서!
            if (h > SOLDER_MIN_H) {
                ++count;
                sumH += h;
                sumX += x;
                sumY += y;
            }
        }
    }

    const double padArea = PAD_SIZE * PAD_SIZE;          // 정상 면적 1,600칸
    const double normalVolume = padArea * NORMAL_H;      // 정상 부피 (칸 × µm)

    res.area_pct = count / padArea * 100.0;
    res.volume_pct = sumH / normalVolume * 100.0;
    if (count > 0) {
        res.height_pct = (sumH / count) / NORMAL_H * 100.0;

        // 무게중심 = 솔더 칸들의 x 평균, y 평균 (+0.5는 칸의 가운데)
        double cx = sumX / count + 0.5;
        double cy = sumY / count + 0.5;
        cv::Rect p = pad_rect(pad);
        double px = p.x + p.width / 2.0;     // 패드 중심
        double py = p.y + p.height / 2.0;
        double distPx = std::hypot(cx - px, cy - py);   // 피타고라스 거리
        res.offset_um = distPx * UM_PER_PX;
    }
    return res;
}