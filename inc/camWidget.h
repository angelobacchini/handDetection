
#ifndef CAMWIDGET_H
#define CAMWIDGET_H

#include "global.h"

#include <deque>
#include <QtWidgets>
#include <QTimer>
#include <QtGui>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

class imageBox;

class camWidget : public QWidget
{
  Q_OBJECT

friend class imageBox;

public:
  explicit camWidget(QWidget* parent = 0);
  ~camWidget();
  camWidget(camWidget const&) = delete;
  camWidget& operator=(camWidget const&) = delete;

public slots:
  void getImage(QByteArray _image);
  void getSamplePosition(const int& _x, const int& _y);
  void getCenterPosition(const int& _x, const int& _y, const double& _r);
  void getFingers(const QVector<int>& _fingers);
  void getVector(int _label, const QVector<double>& _vector);

private slots:
  void countFps();
  void getParameters();
  void showSliderValue(const int& _value);
  void gestureDetect(const QVector<int>& _fingers);

signals:
  void ready();
  void sendParameters(const QString& _sliderId, const int& _value);
  void sendSamplePosition(const int& _x, const int& _y);
  void sendCenterPosition(const int& _x, const int& _y, const double& _r);
  void sendFingers(const QVector<int>& _fingers);
  void gestureDetected(const int& _angle, const QString& _gesture);

private:
  QTimer* m_timer;
  imageBox* m_imageBox;
  QVBoxLayout* m_controlBox;
  QVBoxLayout* m_infoBox;
  QGridLayout* m_layout;
  QLabel* m_fpsLabel;

  QChart* m_thetaChart;
  QLineSeries* m_thetaSeries;

  QTableWidget* m_fingersInfo;
  QVector<int>* m_fingers;
  QLabel* m_gestureInfo;

  QImage m_image;
  QByteArray m_imageArray;

  int m_fps;
  bool m_sliderPressed;
};

class imageBox : public QWidget
{
  Q_OBJECT

public:
  imageBox(QWidget* parent = 0);
  ~imageBox();
  imageBox(imageBox const&) = delete;
  imageBox& operator=(imageBox const&) = delete;
  QSize sizeHint() const;

protected:
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* event );

signals:
  void sendSamplePosition(const int& _x, const int& _y);

public slots:
  void getCenterPosition(const int& _x, const int& _y, const double& _r);
  void getFingers(const QVector<int>& _fingers);
  void gestureDetected(const int& _angle, const QString& _gesture);
  void gestureClear();

private:
  QTimer* m_timer;
  QPointF m_center;
  double m_radius;
  int m_arcProgress;
  QVector<double> m_fingers;
  QString m_gesture;
  int m_gestureAngle;
};

#endif /* CAMWIDGET_H */
