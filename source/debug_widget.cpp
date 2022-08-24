#include "debug_widget.h"


void OverlayImage(cv::Mat *src, cv::Mat *overlay, const cv::Point &location) {
    for (int y = max(location.y, 0); y < src->rows; ++y) {
        int fY = y - location.y;

        if (fY >= overlay->rows)
            break;

        for (int x = max(location.x, 0); x < src->cols; ++x) {
            int fX = x - location.x;

            if (fX >= overlay->cols)
                break;

            double opacity = ((double) overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255;

            for (int c = 0; opacity > 0 && c < src->channels(); ++c) {
                unsigned char overlayPx = overlay->data[fY * overlay->step + fX * overlay->channels() + c];
                unsigned char srcPx = src->data[y * src->step + x * src->channels() + c];
                src->data[y * src->step + src->channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
            }
        }
    }
}


cv::Mat mergeTwoImages(cv::Mat *image1, cv::Mat *image2) {
    int rows = max(image1->rows, image2->rows);
    int cols = image1->cols + image2->cols;
    cv::Mat res = cv::Mat::zeros(cv::Size(cols,rows),CV_8UC1);
    image1->copyTo(res(cv::Rect(0, 0, image1->cols, image1->rows)));
    image2->copyTo(res(cv::Rect(image1->cols, 0, image2->cols, image2->rows)));
    return res;
}


void ShowDebugImage(cv::Mat& frame, const cv::Rect& cropDefault, const cv::Rect& cropOut,
                    const cv::Scalar& colorRangeLow, const cv::Scalar& colorRangeHigh) {
    if (frame.empty() || frame.cols < 1080)
    {
        cv::waitKey(30);
        return;
    }
    cv::Mat cropDefaultImage = frame(cropDefault);
    cv::Mat cropOutImage = frame(cropOut);
    cv::inRange(cropDefaultImage, colorRangeLow, colorRangeHigh, cropDefaultImage);
    cv::inRange(cropOutImage, colorRangeLow, colorRangeHigh, cropOutImage);
    cv::Mat croppedDialogue = mergeTwoImages(&cropDefaultImage, &cropOutImage);
    cv::cvtColor(croppedDialogue, croppedDialogue, cv::COLOR_BGR2RGB);
    cv::Rect overlayRect = cv::Rect(0, 0, (int) (croppedDialogue.cols), (int) (croppedDialogue.rows));
    cv::rectangle(frame, overlayRect, cv::Scalar(0, 0, 0), -1);
    OverlayImage(&frame, &croppedDialogue, cv::Point());
    cv::rectangle(frame, cropDefault, cv::Scalar(0, 255, 0));
    cv::rectangle(frame, cropDefault, cv::Scalar(0, 255, 0));
    cv::rectangle(frame, cropOut, cv::Scalar(0, 0, 255));
    cv::imwrite("debug.png", frame);
    cv::resize(frame, frame, cv::Size(), 0.5, 0.5);
    cv::imshow("debug", frame);
    cv::waitKey(30);
}