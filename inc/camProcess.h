
#ifndef CAMPROCESS_H
#define CAMPROCESS_H

#include "global.h"
#include "opencv2/opencv.hpp"

#include <QtWidgets>

Q_DECLARE_METATYPE(cv::Mat)

class cvCamCapture : public QObject
{
  Q_OBJECT

public:
  explicit cvCamCapture(QObject *parent = 0);
  ~cvCamCapture();
  cvCamCapture(cvCamCapture const&) = delete;
  cvCamCapture& operator=(cvCamCapture const&) = delete;

public slots:
  void getFrame();

signals:
  void frameReady(const cv::Mat&);

private:
  cv::VideoCapture* m_videoCapture;
};

class cvProcessFrame: public QObject
{
  Q_OBJECT

public:
  explicit cvProcessFrame(QObject *parent = 0);
  ~cvProcessFrame();
  cvProcessFrame(cvProcessFrame const&) = delete;
  cvProcessFrame& operator=(cvProcessFrame const&) = delete;

public slots:
  void processFrame(const cv::Mat& _frame);
  void getParameters(const QString& _parameterId, const int& _value);
  void getSamplePosition(const int& _x, const int& _y);

private:
  static void matDeleter(void* mat){delete static_cast<cv::Mat*>(mat); }
  void updateParameters();
  void initTracker(double _noiseCov);
  void trackCenter(cv::Point& _point, double& _radius);

signals:
  void imageReady(QByteArray _image);
  void sendCenterPosition(const int& _x, const int& _y, const double& _r);
  void sendFingers(const QVector<int>& _fingers);
  void sendVector(int _label, const QVector<double>& _vector);

private:
  QImage m_imageOut;
  QMap<QString, int> m_parameters;
  cv::Mat m_bgr, m_hsv, m_oneChannel, m_blended;
  cv::Mat m_deltaH_8U, m_deltaS_8U, m_delta_8U, m_temp0_8U, m_temp1_8U, m_temp0_32F, m_temp0_64F, m_temp1_64F, m_temp2_64F;
  cv::Mat m_floodFill_8U;
  cv::Mat m_distance_8U;
  cv::Mat m_distanceMasked_8U;
  cv::Mat m_channelSplit[3];
  cv::Mat m_splitScreen_8U;
  QVector<double> m_thetaVector;
  QVector<double> m_thetaConv;
  QVector<double> m_convKernel;
  QVector<int> m_peaks;
  cv::KalmanFilter* m_tracker;
  cv::Mat m_trackerMeasurement;
  cv::Mat m_trackerPrediction;
  double m_trackerTicks;
  int m_mirror;
  int m_output;
  int m_channel;
  double m_blend;
  double m_sigma;
  int m_huePicker;
  int m_satPicker;
  int m_median;
  int m_threshold;
};

#endif /* CAMPROCESS_H */
