#ifndef APPLICATION_DECIMAL_INFER_H_
#define APPLICATION_DECIMAL_INFER_H_

#include <opencv2/opencv.hpp>

int recognizeNumber(const cv::Mat& aROI);
void trainGemModel();
void dnnTest();

#endif
