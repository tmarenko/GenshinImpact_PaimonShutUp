#ifndef GENSHINIMPACT_PAIMONSHUTUP_DEBUG_WIDGET_H
#define GENSHINIMPACT_PAIMONSHUTUP_DEBUG_WIDGET_H

#include "opencv2/core.hpp"
#include <opencv2/highgui.hpp>
#include "opencv2/photo.hpp"
#include "windows.h"

void ShowDebugImage(cv::Mat& frame, const cv::Rect& cropDefault, const cv::Rect& cropOut,
                    const cv::Scalar& colorRangeLow, const cv::Scalar& colorRangeHigh);

#endif //GENSHINIMPACT_PAIMONSHUTUP_DEBUG_WIDGET_H