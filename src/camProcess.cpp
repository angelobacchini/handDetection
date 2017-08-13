
#include "global.h"
#include "kernels.h"
#include "camProcess.h"

#include <math.h>

cvCamCapture::cvCamCapture(QObject* parent) :
  QObject(parent)
{
  m_videoCapture = new cv::VideoCapture(0);
  m_videoCapture->open(0);
  m_videoCapture->set(CV_CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
  m_videoCapture->set(CV_CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);

  if(!m_videoCapture->isOpened())
    qDebug() << "CANNOT OPEN CAMERA";
}

cvCamCapture::~cvCamCapture()
{
  delete m_videoCapture;
}

void cvCamCapture::getFrame()
{
  cv::Mat frame;
  m_videoCapture->read(frame);
  emit frameReady(frame);
}

cvProcessFrame::cvProcessFrame(QObject* parent) :
  QObject(parent)
{
  // MATs pre-allocation (save time later)
  m_bgr.create(CAM_HEIGHT, CAM_WIDTH, CV_8UC3);
  m_hsv.create(CAM_HEIGHT, CAM_WIDTH, CV_8UC3);
  m_oneChannel.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_blended.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_channelSplit[0].create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_channelSplit[1].create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_channelSplit[2].create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_deltaH_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_deltaS_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_delta_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_temp0_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_temp1_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_temp0_32F.create(CAM_HEIGHT, CAM_WIDTH, CV_32F);
  m_temp0_64F.create(CAM_HEIGHT, CAM_WIDTH, CV_64F);
  m_temp1_64F.create(CAM_HEIGHT, CAM_WIDTH, CV_64F);
  m_temp2_64F.create(CAM_HEIGHT, CAM_WIDTH, CV_64F);
  m_floodFill_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_distance_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_distanceMasked_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);
  m_splitScreen_8U.create(CAM_HEIGHT, CAM_WIDTH, CV_8U);

  // vector pre allocation
  m_thetaVector.fill(0, 360);
  m_thetaConv.fill(0, 360);
  m_convKernel << GAUSSIAN_9;
  m_peaks.fill(0, 5);

  // init kalman filter
  initTracker(10.f);

  for(int i=0; i<parametersTable.size(); i++)
    m_parameters.insert(parametersTable.at(i).m_id, parametersTable.at(i).m_default);

  updateParameters();
}

cvProcessFrame::~cvProcessFrame()
{
  delete m_tracker;
}

void cvProcessFrame::updateParameters()
{
  m_mirror = m_parameters.value("mirror");
  m_output = m_parameters.value("output");
  m_channel = m_parameters.value("channel");
  m_blend = m_parameters.value("blend")/100.0;
  m_sigma = 1.f/(2.f*std::pow((double)m_parameters.value("sigma"), 2));
  m_huePicker = m_parameters.value("huePicker");
  m_satPicker = m_parameters.value("satPicker");
  m_median = m_parameters.value("median");
  m_threshold = m_parameters.value("threshold");
}

void cvProcessFrame::getParameters(const QString& _parameterId, const int& _value)
{
  m_parameters.insert(_parameterId, _value);
  updateParameters();
}

void cvProcessFrame::processFrame(const cv::Mat& _frame)
{
  _frame.copyTo(m_hsv); //actually this is BGR!
  if(m_mirror == 1) cv::flip(m_hsv, m_bgr, 1); // bgr version stored on m_bgr (mirrored if m_mirror is true)
  cv::cvtColor(m_bgr, m_hsv, CV_BGR2HSV); // hsv channels on m_hsv MAT

  cv::split(m_hsv, m_channelSplit);
  m_oneChannel = m_channelSplit[m_channel]; // 3 MATs array

  // find the distance from the skin picker tone in HS space for each pixel
  m_channelSplit[0].convertTo(m_temp1_64F, CV_64F); //hue channel
  cv::threshold(m_temp1_64F, m_temp2_64F, 90.f, 180.f, cv::THRESH_BINARY); // circular shift on range (values 90 to 180 become -90 to 0)
  m_temp0_64F = m_temp1_64F - m_temp2_64F;
  m_temp1_64F = 0.5*(m_temp0_64F - (double)m_huePicker);

  m_channelSplit[1].convertTo(m_temp0_64F, CV_64F); //saturation channel
  m_temp2_64F = 0.1*(m_temp0_64F - (double)m_satPicker);

  // apply a gaussian curve for each pixel
  cv::pow(m_temp1_64F, 2, m_temp0_64F);
  m_temp0_64F.convertTo(m_deltaH_8U, CV_8U); //delta^2 in Cb image
  cv::pow(m_temp2_64F, 2, m_temp1_64F);
  m_temp1_64F.convertTo(m_deltaS_8U, CV_8U); //delta^2 in Cr image

  m_temp2_64F = m_temp0_64F + m_temp1_64F;
  m_temp0_64F = -(m_temp2_64F * m_sigma); //gaussian curve 1/2
  cv::exp(m_temp0_64F, m_temp1_64F); //gaussian curve 2/2
  m_temp2_64F = m_temp1_64F * 255.f; //normalize to 0-255 before converting to 8bit
  m_temp2_64F.convertTo(m_temp0_8U, CV_8U);

  // median filtering
  cv::medianBlur(m_temp0_8U, m_delta_8U, m_median);

  // extract hand shape
  m_delta_8U.copyTo(m_temp0_8U);
  cv::threshold(m_temp0_8U, m_floodFill_8U, m_threshold, 255, cv::THRESH_BINARY);

  // compute distance transform
  cv::distanceTransform(m_floodFill_8U, m_temp0_32F, cv::DIST_L2, cv::DIST_MASK_PRECISE);
  m_temp0_32F.convertTo(m_temp0_64F, CV_64F);
  m_temp0_64F.convertTo(m_distance_8U, CV_8U);

  // find max of distance transform (center of the hand)
  double max;
  cv::Point maxDistance = cv::Point();
  cv::minMaxLoc(m_temp0_64F, 0, &max, 0, &maxDistance);

  // apply kalman filtering on the center and radius of the hand
  trackCenter(maxDistance, max);

  // apply a circular mask around the hand center
  m_temp0_8U = cv::Scalar(0);
  cv::circle(m_temp0_8U, maxDistance, max*3.f, cv::Scalar(255), -1);
  cv::circle(m_temp0_8U, maxDistance, max*1.75f, cv::Scalar(0), -1);
  m_temp0_64F.copyTo(m_temp1_64F, m_temp0_8U);
  m_temp1_64F.convertTo(m_distanceMasked_8U, CV_8U);

  // convert to polar coordinates (origin = center of the hand) and find distance transform intensities for each theta on the polar coordinates space
  const double* index;
  m_thetaVector.fill(0);
  int theta;
  for(int i = 0; i < m_temp1_64F.rows; i++)
  {
    index = m_temp1_64F.ptr<double>(i);
    for(int j = 0; j < m_temp1_64F.cols; j++)
    {
      if (index[j] > 5)
      {
        theta = std::floor(std::atan2((double)j - maxDistance.x, maxDistance.y - (double)i)*180/3.14f) + 180;
        if (theta < 300 && theta > 60)
          m_thetaVector[theta] = m_thetaVector.at(theta) + index[j];
      }
    }
  }

  // smooth the theta densities vector (convolution with a gaussian kernel)
  int len = m_convKernel.size();
  for(int i = len-1; i < 360; i++)
  {
    m_thetaConv[i] = 0;
    for(int j = 0; j < len; j++)
      m_thetaConv[i] += m_thetaVector.at(i - j) * m_convKernel.at(j);
  }

  // find peaks of the theta vector (fingers)
  int k = 0;
  m_peaks.fill(0);
  for(int i = 2; i < 358; i++)
  {
    if(m_thetaConv.at(i) > m_thetaConv.at(i-1) &&
       m_thetaConv.at(i) > m_thetaConv.at(i-2) &&
       m_thetaConv.at(i) > m_thetaConv.at(i+1) &&
       m_thetaConv.at(i) > m_thetaConv.at(i+2) &&
       m_thetaConv.at(i) > 0.15)
      m_peaks[k++] = i;
    if(k >= 5)
      break;
  }

  // create splitscreen output
  cv::Size sz;
  sz.height = CAM_HEIGHT/2;
  sz.width = CAM_WIDTH/2;
  cv::Mat topLeft(m_splitScreen_8U, cv::Rect(0, 0, CAM_WIDTH/2, CAM_HEIGHT/2)); // copy constructor
  cv::Mat topRight(m_splitScreen_8U, cv::Rect(CAM_WIDTH/2, 0, CAM_WIDTH/2, CAM_HEIGHT/2));
  cv::Mat bottomLeft(m_splitScreen_8U, cv::Rect(0, CAM_HEIGHT/2, CAM_WIDTH/2, CAM_HEIGHT/2));
  cv::Mat bottomRigth(m_splitScreen_8U, cv::Rect(CAM_WIDTH/2, CAM_HEIGHT/2, CAM_WIDTH/2, CAM_HEIGHT/2));
  cv::resize(m_oneChannel, topLeft, sz);
  cv::resize(m_delta_8U, topRight, sz);
  cv::resize(m_floodFill_8U, bottomLeft, sz);
  cv::resize(m_distance_8U * 3, bottomRigth, sz);

  // output selector
  if(m_output == 0)
    m_temp1_8U = m_deltaH_8U;
  else if(m_output == 1)
    m_temp1_8U = m_deltaS_8U;
  else if(m_output == 2)
    m_temp1_8U = m_delta_8U;
  else if(m_output == 3)
    m_temp1_8U = m_floodFill_8U;
  else if(m_output == 4)
    m_temp1_8U = m_distance_8U * 2;
  else if(m_output == 5)
    m_temp1_8U = m_distanceMasked_8U * 2;
  else if(m_output == 6)
    m_temp1_8U = m_splitScreen_8U;
  else
    m_temp1_8U = m_oneChannel;

  // blend processed frame with original
  if(m_output != 6)
    cv::addWeighted(m_oneChannel, 1-m_blend, m_temp1_8U, m_blend, 0.0, m_blended);
  else
    m_blended = m_temp1_8U;

  // load MAT on a QByteArray to be sent to the UI thread
  QByteArray array = QByteArray((char*)m_blended.data, CAM_WIDTH*CAM_HEIGHT);
  emit imageReady(array);

  if(m_output != 6) // if splitscreen is not selected
  {
    emit sendCenterPosition(maxDistance.x, maxDistance.y, max);
    emit sendFingers(m_peaks);
  }
  else //when in splitscreen hand tracker graphics are not shown
  {
    emit sendCenterPosition(0, 0, 0);
  }
  emit sendVector(THETA_VECTOR, m_thetaConv);
}

void cvProcessFrame::getSamplePosition(const int &_x, const int &_y)
{
  // show on the console the H S V value of the pixel clicked
  float a = (float)(m_channelSplit[0].at<uchar>(_y, _x));
  float b = (float)(m_channelSplit[1].at<uchar>(_y, _x));
  float c = (float)(m_channelSplit[2].at<uchar>(_y, _x));
  qDebug() << a << "" << b << "" << c;
}

void cvProcessFrame::trackCenter(cv::Point& _point, double& _radius)
{
  m_trackerMeasurement.at<float>(0, 0) = (float)_point.x;
  m_trackerMeasurement.at<float>(1, 0) = (float)_point.y;
  m_trackerMeasurement.at<float>(2, 0) = (float)_radius;

  double precTick = m_trackerTicks;
  m_trackerTicks = (double) cv::getTickCount();
  double dT = (m_trackerTicks - precTick) / cv::getTickFrequency(); //seconds

  m_tracker->transitionMatrix.at<float>(0, 2) = dT;
  m_tracker->transitionMatrix.at<float>(1, 3) = dT;

  m_trackerPrediction = m_tracker->predict();
  cv::Mat estimate = m_tracker->correct(m_trackerMeasurement);
  m_trackerMeasurement.at<float>(0, 0) = estimate.at<float>(0, 0);
  m_trackerMeasurement.at<float>(1, 0) = estimate.at<float>(1, 0);
  m_trackerMeasurement.at<float>(2, 0) = estimate.at<float>(4, 0);

  _point.x = (int)m_trackerPrediction.at<float>(0, 0);
  _point.y = (int)m_trackerPrediction.at<float>(1, 0);
  _radius = (double)m_trackerPrediction.at<float>(4, 0);
}

void cvProcessFrame::initTracker(double _noiseCov)
{
  m_trackerTicks = 0;
  m_trackerMeasurement.create(3, 1, CV_32F);
  m_trackerPrediction.create(5, 1, CV_32F);
  m_tracker = new cv::KalmanFilter(5, 3, 0);

  cv::setIdentity(m_tracker->transitionMatrix);
  m_tracker->statePre.at<float>(0) = (float)CAM_WIDTH/2.f;
  m_tracker->statePre.at<float>(1) = (float)CAM_HEIGHT/2.f;
  m_tracker->statePre.at<float>(2) = 0.f;
  m_tracker->statePre.at<float>(3) = 0.f;
  m_tracker->statePre.at<float>(4) = 100.f;

  cv::setIdentity(m_tracker->measurementMatrix);
  m_tracker->measurementMatrix = cv::Mat::zeros(3, 5, CV_32F);
  m_tracker->measurementMatrix.at<float>(0, 0) = 1.0f;
  m_tracker->measurementMatrix.at<float>(1, 1) = 1.0f;
  m_tracker->measurementMatrix.at<float>(2, 4) = 1.0f;

  m_tracker->processNoiseCov.at<float>(0, 0) = 0.1f;
  m_tracker->processNoiseCov.at<float>(1, 1) = 0.1f;
  m_tracker->processNoiseCov.at<float>(2, 2) = 2.f;
  m_tracker->processNoiseCov.at<float>(3, 3) = 2.f;
  m_tracker->processNoiseCov.at<float>(4, 4) = 0.1f;

  cv::setIdentity(m_tracker->measurementNoiseCov, cv::Scalar::all(_noiseCov));
}
