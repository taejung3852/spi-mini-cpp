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

// 기판 전체 솔더를 덩어리로 나눠, 한 덩어리가 패드 2개 이상에 걸치면 브리지
std::vector<bool> find_bridges(const cv::Mat& board) {
    cv::Mat mask = board > SOLDER_MIN_H;      // 솔더 칸 255, 나머지 0
    cv::Mat labels;
    int n = cv::connectedComponents(mask, labels, 8, CV_32S);  // 붙어 있는 칸끼리 같은 번호

    std::vector<int> padsPerLabel(n, 0);      // 덩어리 번호마다 걸친 패드 수
    std::vector<int> padLabel(PAD_COUNT, 0);  // 패드마다 덩어리 번호
    for (int i = 0; i < PAD_COUNT; ++i) {
        cv::Rect p = pad_rect(i);
        int lbl = labels.at<int>(p.y + p.height / 2, p.x + p.width / 2);  // 패드 중심 칸의 번호
        padLabel[i] = lbl;
        if (lbl > 0) ++padsPerLabel[lbl];     // 0은 배경
    }

    std::vector<bool> bridge(PAD_COUNT, false);
    for (int i = 0; i < PAD_COUNT; ++i) {
        int lbl = padLabel[i];
        bridge[i] = lbl > 0 && padsPerLabel[lbl] >= 2;
    }
    return bridge;
}

// 허용 범위를 벗어난 항목을 모두 사유로 남긴다
static void check_range(const char* name, double pct, std::string& reason) {
    if (pct < TOL_LO || pct > TOL_HI) {
        std::ostringstream s;
        s << name << ' ' << std::fixed << std::setprecision(0) << pct << '%';
        if (!reason.empty()) reason += "; ";
        reason += s.str();
    }
}

void judge(PadResult& r) {
    r.reason.clear();
    check_range("area", r.area_pct, r.reason);
    check_range("height", r.height_pct, r.reason);
    check_range("volume", r.volume_pct, r.reason);
    if (r.offset_um > OFFSET_MAX_UM) {
        std::ostringstream s;
        s << "offset " << std::fixed << std::setprecision(0) << r.offset_um << "um";
        if (!r.reason.empty()) r.reason += "; ";
        r.reason += s.str();
    }
    if (r.bridge) {
        if (!r.reason.empty()) r.reason += "; ";
        r.reason += "bridge";
    }
    r.ok = r.reason.empty();
}

std::vector<PadResult> inspect_board(const cv::Mat& board) {
    std::vector<bool> bridge = find_bridges(board);
    std::vector<PadResult> results;
    for (int i = 0; i < PAD_COUNT; ++i) {
        PadResult r = measure_pad(board, i);
        r.bridge = bridge[i];
        judge(r);
        results.push_back(r);
    }
    return results;
}

bool save_result_csv(const std::string& path, const std::vector<PadResult>& results) {
    std::ofstream out(path);
    if (!out) return false;
    out << "pad,area_pct,height_pct,volume_pct,offset_um,bridge,result,reason\n";
    out << std::fixed << std::setprecision(1);
    for (const PadResult& r : results) {
        out << r.pad << ',' << r.area_pct << ',' << r.height_pct << ',' << r.volume_pct << ','
            << r.offset_um << ',' << (r.bridge ? 1 : 0) << ',' << (r.ok ? "OK" : "NG") << ','
            << '"' << r.reason << '"' << '\n';
    }
    return true;
}

bool save_overlay_png(const std::string& path, const cv::Mat& board, const std::vector<PadResult>& results) {
    cv::Mat gray, color;
    cv::normalize(board, gray, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::cvtColor(gray, color, cv::COLOR_GRAY2BGR);       // 흑백 바탕에 색 테두리
    for (const PadResult& r : results) {
        cv::Scalar c = r.ok ? cv::Scalar(0, 200, 0) : cv::Scalar(0, 0, 255);  // BGR: 초록 / 빨강
        cv::rectangle(color, window_rect(r.pad), c, 2);
        cv::putText(color, std::to_string(r.pad), window_rect(r.pad).tl() + cv::Point(2, 14),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, c, 1);
    }
    return cv::imwrite(path, color);
}
