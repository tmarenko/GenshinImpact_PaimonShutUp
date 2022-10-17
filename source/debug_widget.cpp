#include "debug_widget.h"

int MAX_WIDTH = 0;
int MAX_HEIGHT = 0;


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


cv::Mat mergeTwoImages(cv::Mat *image1, cv::Mat *image2, int mat_type = CV_8UC1) {
    int rows = max(image1->rows, image2->rows);
    int cols = image1->cols + image2->cols;
    cv::Mat res = cv::Mat::zeros(cv::Size(cols, rows), mat_type);
    image1->copyTo(res(cv::Rect(0, 0, image1->cols, image1->rows)));
    image2->copyTo(res(cv::Rect(image1->cols, 0, image2->cols, image2->rows)));
    return res;
}


void copyToFrame(cv::Mat &frame, cv::Mat &toCopy, const int row, const int col=0) {
    cv::Rect overlayRect = cv::Rect((int) (toCopy.cols) * col, (int) (toCopy.rows) * row,
                                    (int) (toCopy.cols), (int) (toCopy.rows));
    //cv::rectangle(frame, overlayRect, cv::Scalar(255, 255, 255, 0), -1);
    cv::Mat destRoi = frame(overlayRect);
    toCopy.copyTo(destRoi);
}


void ShowDebugImage(cv::Mat& frame, const cv::Rect& cropDefault, const cv::Rect& cropOut,
                    const cv::Scalar& colorRangeLow, const cv::Scalar& colorRangeHigh) {
    if (frame.empty() || frame.cols < 1080)
    {
        cv::waitKey(30);
        return;
    }
    if (frame.cols > MAX_WIDTH)
        MAX_WIDTH = frame.cols;
    if (frame.rows > MAX_HEIGHT)
        MAX_HEIGHT = frame.rows;

    cv::Mat cropDefaultImage = frame(cropDefault);
    cv::Mat cropOutImage = frame(cropOut);
    cv::Mat cropDefaultImageInRange;
    cv::Mat cropOutImageInRange;
    cv::Mat cropDefaultImageLowRange;
    cv::Mat cropOutImageLowRange;
    cv::Mat cropDefaultImageHighRange;
    cv::Mat cropOutImageHighRange;
    cv::inRange(cropDefaultImage, colorRangeLow, colorRangeHigh, cropDefaultImageInRange);
    cv::inRange(cropOutImage, colorRangeLow, colorRangeHigh, cropOutImageInRange);
    cv::inRange(cropDefaultImage, cv::Scalar(0, 0, 0), colorRangeHigh, cropDefaultImageLowRange);
    cv::inRange(cropOutImage, cv::Scalar(0, 0, 0), colorRangeHigh, cropOutImageLowRange);
    cv::inRange(cropDefaultImage, colorRangeLow, cv::Scalar(255, 255, 255), cropDefaultImageHighRange);
    cv::inRange(cropOutImage, colorRangeLow, cv::Scalar(255, 255, 255), cropOutImageHighRange);

    cv::Mat croppedDialogueInRange = mergeTwoImages(&cropDefaultImageInRange, &cropOutImageInRange);
    cv::Mat croppedDialogueLowRange = mergeTwoImages(&cropDefaultImageLowRange, &cropOutImageLowRange);
    cv::Mat croppedDialogueHighRange = mergeTwoImages(&cropDefaultImageHighRange, &cropOutImageHighRange);
    cv::Mat croppedDialogueInRangeRGB = croppedDialogueInRange.clone();
    cv::Mat croppedDialogueLowRangeRGB = croppedDialogueInRange.clone();
    cv::Mat croppedDialogueHighRangeRGB = croppedDialogueInRange.clone();
    cv::cvtColor(croppedDialogueInRange, croppedDialogueInRange, cv::COLOR_BGR2RGB);
    cv::cvtColor(croppedDialogueLowRange, croppedDialogueLowRange, cv::COLOR_BGR2RGB);
    cv::cvtColor(croppedDialogueHighRange, croppedDialogueHighRange, cv::COLOR_BGR2RGB);
    cv::Mat croppedDialogue = mergeTwoImages(&cropDefaultImage, &cropOutImage, CV_8UC3);
    copyToFrame(frame, croppedDialogueInRange, 0);
    copyToFrame(frame, croppedDialogueLowRange, 1);
    copyToFrame(frame, croppedDialogueHighRange, 2);
    copyToFrame(frame, croppedDialogueInRangeRGB, 0, 1);
    copyToFrame(frame, croppedDialogueLowRangeRGB, 1, 1);
    copyToFrame(frame, croppedDialogueHighRangeRGB, 2, 1);
    copyToFrame(frame, croppedDialogue, 3);

    // Draw rectangles (red and green) on the places where name is captured
    cv::rectangle(frame, cropDefault, cv::Scalar(0, 255, 0));
    cv::rectangle(frame, cropOut, cv::Scalar(0, 0, 255));
    // Write debug image to file
    std::string windowName = "debug " + std::to_string(MAX_WIDTH) + "x" + std::to_string(MAX_HEIGHT);
    cv::imwrite(windowName + ".png", frame);
    // Show debug image in separate window
    cv::resize(frame, frame, cv::Size(), 0.5, 0.5);
    cv::imshow(windowName, frame);

    cv::waitKey(30);
}