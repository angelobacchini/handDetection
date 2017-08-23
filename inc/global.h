#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtWidgets>

#define CAM_WIDTH 640
#define CAM_HEIGHT 480

#define THETA_VECTOR 0
#define VICTORY_CORR 1
#define RNR_CORR 2

#define SLIDER_STYLE "background-color: rgb(10, 10, 10);"
#define SLIDER_LABEL_STYLE  "color: rgb(90, 160, 100); font: 10pt ""DejaVu Sans"";"
#define FPS_LABEL_STYLE "color: rgb(80, 120, 180); font: 24pt ""DejaVu Sans""; border-style: none; background: transparent;"
#define FINGERS_INFO_STYLE "QTableView {color: rgb(90, 100, 130); font: 8pt ""DejaVu Sans Mono""; border-style: none; background: transparent;} \
                            QHeaderView::section {color: rgb(80, 120, 180); font: 8pt ""DejaVu Sans Mono""; border-style: none; background: transparent;} \
                            QTableCornerButton::section {border-style: none; background: transparent;}"
#define GESTURE_INFO_STYLE "color: rgb(90, 100, 130); font: 8pt ""DejaVu Sans Mono"";"

struct processParameter
{
  QString m_id;
  int m_min;
  int m_max;
  int m_default;

  processParameter()
  {}

  processParameter(QString _id, int _min, int _max, int _default) :
    m_id(_id), m_min(_min), m_max(_max), m_default(_default)
  {}
};

extern QVector<processParameter> parametersTable;

#endif
