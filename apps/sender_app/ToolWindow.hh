#pragma once

#include "ImageProcess.hh"
#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>
#include <QtGui/qevent.h>
#include <QtWidgets/qboxlayout.h>

namespace Application {

class ToolWindow : public QWidget {
  Q_OBJECT
public:
  explicit ToolWindow(QWidget *parent = nullptr);

signals:
  void setParams(const Processing::ImageAdjustParams &params);
  void sendImage();
  void saveImage();

private:
  QSlider *exposure_slider_;
  QSlider *contrast_slider_;
  QSlider *highlight_slider_;
  QSlider *shadow_slider_;
  QSlider *saturation_slider_;
  QSlider *temperature_slider_;
  QSlider *tint_slider_;

  Processing::ImageAdjustParams params_ = {}; // 画像調整パラメータ
};

} // namespace Application
