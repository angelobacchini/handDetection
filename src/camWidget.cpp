
#include "global.h"
#include "kernels.h"
#include "camWidget.h"

#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

camWidget::camWidget(QWidget* parent) :
  QWidget(parent), m_fps(0), m_sliderPressed(false)
{
  m_controlBox = new QVBoxLayout();
  m_infoBox = new QVBoxLayout();

  // create sliders from parameters table
  QSlider* slider;
  QLabel* label;
  for(int i=0; i<parametersTable.size(); i++)
  {
    slider = new QSlider();
    slider->setAccessibleName(parametersTable.at(i).m_id);
    slider->setRange(parametersTable.at(i).m_min, parametersTable.at(i).m_max);
    slider->setValue(parametersTable.at(i).m_default);
    slider->setOrientation(Qt::Horizontal);
    slider->setStyleSheet(SLIDER_STYLE);
    slider->setFixedWidth(150);
    label = new QLabel();
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(SLIDER_LABEL_STYLE);
    label->setText(parametersTable.at(i).m_id);
    m_controlBox->addWidget(label);
    m_controlBox->addWidget(slider);
    m_controlBox->addSpacing(3);
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(showSliderValue(int)));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(getParameters()));
  }

  // create FPS (frame per second) label
  m_fpsLabel = new QLabel();
  m_fpsLabel->setStyleSheet(FPS_LABEL_STYLE);
  m_infoBox->addWidget(m_fpsLabel);

  // create hand profile chart
  QValueAxis* axisX;
  QValueAxis* axisY;
  QChartView* chartView;
  m_thetaSeries = new QLineSeries();
  m_thetaChart = new QChart();
  axisX = new QValueAxis();
  axisX->setRange(60, 299);
  axisX->setLabelFormat("%d");
  axisY = new QValueAxis;
  axisY->setRange(0, 2000);
  axisY->setLabelFormat("%1.0f");
  m_thetaChart->addAxis(axisY, Qt::AlignLeft);
  m_thetaChart->addAxis(axisX, Qt::AlignBottom);
  m_thetaChart->addSeries(m_thetaSeries);
  m_thetaSeries->attachAxis(axisX);
  m_thetaSeries->attachAxis(axisY);
  m_thetaChart->setTheme(QChart::ChartThemeDark);
  m_thetaChart->setBackgroundBrush(QBrush(QColor(10, 10, 10)));
  m_thetaChart->setPlotAreaBackgroundVisible(false);
  m_thetaChart->legend()->hide();
  m_thetaChart->setMargins(QMargins(0, 0, 0, 0));
  m_thetaSeries->setColor(QColor(60, 200, 120));
  chartView = new QChartView(m_thetaChart);
  chartView->setRenderHint(QPainter::Antialiasing);
  chartView->setRubberBand(QChartView::HorizontalRubberBand);
  chartView->setMinimumWidth(250);
  chartView->setMinimumHeight(175);
  m_infoBox->addWidget(chartView);

  // create fingers info label
  m_fingers = new QVector<int>(5);
  m_fingersInfo = new QTableWidget(5, 2);
  QStringList labels;
  labels << "finger0" << "finger1" << "finger2" << "finger3" << "finger4";
  m_fingersInfo->setVerticalHeaderLabels(QStringList(labels));
  labels.clear();
  labels << "value" << "theta";
  m_fingersInfo->setHorizontalHeaderLabels(QStringList(labels));
  QTableWidgetItem* item;
  for(int i = 0; i < 5; i++)
  {
    item = new QTableWidgetItem("-");
    m_fingersInfo->setItem(i, 0, item);
    item = new QTableWidgetItem("-");
    m_fingersInfo->setItem(i, 1, item);
  }
  m_fingersInfo->setStyleSheet(FINGERS_INFO_STYLE);
  m_infoBox->addWidget(m_fingersInfo);

  // create image box
  m_imageBox = new imageBox();

  // populate main layout
  m_layout = new QGridLayout();
  m_layout->addLayout(m_infoBox, 0, 0, 1, 1);
  m_layout->addWidget(m_imageBox, 0, 1, 1, 1);
  m_layout->addLayout(m_controlBox, 0, 2, 1, 1);
  setLayout(m_layout);

  // create timer for fps calculation
  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);

  // signal slots connection
  connect(m_imageBox, SIGNAL(sendSamplePosition(int,int)), this, SLOT(getSamplePosition(int,int)));
  connect(this, SIGNAL(sendCenterPosition(int,int,double)), m_imageBox, SLOT(getCenterPosition(int,int,double)));
  connect(this, SIGNAL(sendFingers(QVector<int>)), m_imageBox, SLOT(getFingers(QVector<int>)));
  connect(this, SIGNAL(gestureDetected(int,QString)), m_imageBox, SLOT(gestureDetected(int,QString)));
  connect(m_timer, SIGNAL(timeout()), this, SLOT(countFps()));

  m_timer->start(1000);
}

camWidget::~camWidget()
{}

void camWidget::getParameters()
{
  QSlider* slider = qobject_cast<QSlider*>(sender());
  emit sendParameters(slider->accessibleName(), slider->value());
  m_sliderPressed = false;
}

void camWidget::countFps()
{
  if (!m_sliderPressed)
    m_fpsLabel->setText(QString::number(m_fps) + " fps");
  m_fps = 0;
  m_timer->start(1000);
}

void camWidget::showSliderValue(const int& _value)
{
  m_sliderPressed = true;
  QSlider* slider = qobject_cast<QSlider*>(sender());
  m_fpsLabel->setText(slider->accessibleName() + " = " + QString::number(_value));
}

void camWidget::getImage(QByteArray _image)
{
  // new image received from the processing thread
  m_imageArray = _image;
  m_image = QImage((uchar*)m_imageArray.data(), CAM_WIDTH, CAM_HEIGHT, 640, QImage::Format_Grayscale8);
  m_imageBox->update();
  m_fps++; // update fps count
  emit ready();
}

void camWidget::getSamplePosition(const int& _x, const int& _y)
{
  // user click signal received from the imageBox. Forward signal and click position to processing thread
  emit sendSamplePosition(_x, _y);
}

void camWidget::getCenterPosition(const int& _x, const int& _y, const double& _r)
{
  // center of the hand coordinates received from the processing thread. Forward signal to the imageBox
  emit sendCenterPosition(_x, _y, _r);
}

void camWidget::getFingers(const QVector<int> &_fingers)
{
  // fingers position received from the processing thread. Update fingers info table
  for(int i = 0; i < _fingers.size(); i++)
  {
    m_fingers->replace(i, _fingers.at(i));
    qreal value = m_thetaSeries->at(m_fingers->at(i)).y();
    QTableWidgetItem* item;
    if(value > 0)
    {
      item = m_fingersInfo->item(i, 0);
      item->setText(QString::number(value, 'f', 2));
      item = m_fingersInfo->item(i, 1);
      item->setText(QString::number(m_fingers->at(i)));
    }
    else
    {
      item = m_fingersInfo->item(i, 0);
      item->setText("-");
      item = m_fingersInfo->item(i, 1);
      item->setText("-");
    }
  }
  emit sendFingers(_fingers); // forward fingers positionto the imageBox
}

void camWidget::getVector(int _label, const QVector<double> &_vector)
{
  // theta vector received from the procssing thread. Store it and update the hand profile chart
  QVector<QPointF> series(360);

  for(int i = 0; i < 360; i++)
    series.replace(i, QPointF(i, _vector.at(i)));

  if(_label == THETA_VECTOR) // to be able to receive different vectors series using the same slot
    m_thetaSeries->replace(series);
}

void camWidget::gestureDetect(QString _gesture, const QLineSeries& _series)
{
  int index = 0;
  emit gestureDetected(index, _gesture);
}

imageBox::imageBox(QWidget *parent) :
  QWidget(parent)
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_fingers.fill(0, 5);
  m_arcProgress = 0;
  m_gesture = QString("");
  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(gestureClear()));
  m_timer->start(1000); // 1 second
}

imageBox::~imageBox()
{}

QSize imageBox::sizeHint() const
{
  return QSize(CAM_WIDTH, CAM_HEIGHT);
}

void imageBox::paintEvent(QPaintEvent* event)
{
  camWidget* parent = qobject_cast<camWidget*>(parentWidget());

  double width =  m_radius/10;
  int fontSize = floor(18.f*m_radius/20.f);

  QPainter p(this);
  QPen pen;
  p.save();
  p.setRenderHint(QPainter::Antialiasing);

  p.drawImage(0, 0, parent->m_image);
  p.translate(m_center);

  if(m_gesture == QString("") && m_radius > 20) // if no gesture is detected, draw a circle around the center of the hand
  {
    pen.setWidth(3);
    pen.setColor(QColor(255, 255, 255, 150));
    p.setPen(pen);
    p.drawArc(-m_radius - width/2.f, -m_radius - width/2.f, 2.f*m_radius + width, 2.f*m_radius + width, 90.f*16.f, -(double)m_arcProgress*16.f);

    pen.setWidth(2);
    pen.setColor(QColor(160, 30, 70, 150));
    p.setPen(pen);
    p.drawEllipse(QPointF(0.f, 0.f), 20, 20);
    p.setBrush(QColor(160, 30, 70, 50));
    p.drawEllipse(QPointF(0.f, 0.f), 20 + width, 20 + width);

    pen.setWidth(2);
    pen.setColor(QColor(25, 120, 100, 150));
    p.setPen(pen);
    p.drawEllipse(QPointF(0.f, 0.f), m_radius, m_radius);
    p.setBrush(QColor(25, 120, 100, 50));
    p.drawEllipse(QPointF(0.f, 0.f), m_radius + width, m_radius + width);

    QLineF line;
    line.setP1(QPointF(0.f, 0.f));
    line.setP2(QPointF(CAM_WIDTH, CAM_HEIGHT));
    for(int i = 0; i < 5; i++) // draw a ray starting from the center of the hand for each finger detected
    {
      if(m_fingers.at(i) > 0)
      {
        line.setAngle(m_fingers.at(i));
        pen.setWidth(10);
        pen.setColor(QColor(30, 80, 100, 25));
        p.setPen(pen);
        p.drawLine(line);
        pen.setWidth(2);
        pen.setColor(QColor(30, 80, 100, 125));
        p.setPen(pen);
        p.drawLine(line);
      }
    }
  }
  else // if a gesture is detected, display the gesture label
  {
    p.rotate(m_gestureAngle - 180);
    p.setFont(QFont("DejaVu Sans", fontSize));
    pen.setWidth(3);
    pen.setColor(QColor(230, 140, 50, 200));
    p.setPen(pen);
    p.drawText(QRectF(-3*m_radius, - m_radius, 6.f*m_radius, 2.f*m_radius), Qt::AlignCenter, m_gesture);
  }
  p.restore();
}

void imageBox::mousePressEvent(QMouseEvent* event)
{
  // signal the mouse click event to camWidget
  emit sendSamplePosition(event->pos().x(), event->pos().y());
}

void imageBox::getCenterPosition(const int& _x, const int& _y, const double& _r)
{
  // center of the hand coordinates received from camWidget. Update properties
  m_arcProgress += 10;
  m_arcProgress = m_arcProgress % 360;
  m_center.setX(_x);
  m_center.setY(_y);
  m_radius = _r;
}

void imageBox::getFingers(const QVector<int>& _fingers)
{
  // fingers info received from camWidget. Update properties
  for(int i = 0; i < 5; i++)
  {
    if(_fingers.at(i) > 0)
      m_fingers[i] = 270.f - (double)_fingers.at(i);
    else
      m_fingers[i] = 0;
  }
}

void imageBox::gestureDetected(const int& _angle, const QString& _gesture)
{
  // gesture detected signal received from camWidget
  m_gesture = _gesture;
  m_gestureAngle = _angle;
  m_timer->start(200); // delay timer expiration each time a gesture detected signal is received
}

void imageBox::gestureClear()
{
  m_gesture = QString(""); // upon timer expiration clear gesture
}
