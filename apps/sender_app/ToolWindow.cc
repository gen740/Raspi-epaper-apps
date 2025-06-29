#include "ToolWindow.hh"
#include <QComboBox>

namespace Application {

using Processing::ImageAdjustParams;

ToolWindow::ToolWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Preview");
  setFixedSize(500, 600);

  auto *lay = new QVBoxLayout(this);
  auto label_minimum_width = 80;
  QLabel *label;
  QHBoxLayout *hl;

  // Algorithm selection
  auto *combo = new QComboBox(this);
  combo->addItem("Atkinson",
                 QVariant::fromValue(Processing::DitheringAlgorithm::ATKINSON));
  combo->addItem(
      "Floyd-Steinberg",
      QVariant::fromValue(Processing::DitheringAlgorithm::FLOYD_STEINBERG));
  combo->addItem(
      "Jarvis-Judice-Ninke",
      QVariant::fromValue(Processing::DitheringAlgorithm::JARVIS_JUDICE_NINKE));
  combo->addItem("Stucki",
                 QVariant::fromValue(Processing::DitheringAlgorithm::STUCKI));
  combo->addItem("Burkes",
                 QVariant::fromValue(Processing::DitheringAlgorithm::BURKES));
  combo->addItem("Sierra",
                 QVariant::fromValue(Processing::DitheringAlgorithm::SIERRA));
  combo->addItem("Sierra2",
                 QVariant::fromValue(Processing::DitheringAlgorithm::SIERRA2));
  combo->addItem(
      "Sierra Lite",
      QVariant::fromValue(Processing::DitheringAlgorithm::SIERRA_LITE));
  combo->addItem("DBS",
                 QVariant::fromValue(Processing::DitheringAlgorithm::DBS));
  combo->addItem("Ordered",
                 QVariant::fromValue(Processing::DitheringAlgorithm::ORDERED));
  combo->addItem("Random",
                 QVariant::fromValue(Processing::DitheringAlgorithm::RANDOM));
  combo->addItem("Threshold", QVariant::fromValue(
                                  Processing::DitheringAlgorithm::THRESHOLD));

  connect(combo, &QComboBox::currentIndexChanged, this, [this, combo]() {
    auto alg = combo->currentData().value<Processing::DitheringAlgorithm>();
    if (alg != dithering_algorithm_) {
      dithering_algorithm_ = alg;
      params_.dithering_algorithm = dithering_algorithm_;
      emit setParams(params_);
    }
  });

  lay->addWidget(combo);

  // Exposure slider
  exposure_slider_ = new QSlider(Qt::Horizontal, this);
  exposure_slider_->setRange(-100, 100);
  exposure_slider_->setTickPosition(QSlider::TicksBelow);
  exposure_slider_->setTickInterval(40);
  exposure_slider_->setSingleStep(1);
  label = new QLabel("exposure");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(exposure_slider_);
  lay->addLayout(hl);

  connect(exposure_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.exposure = static_cast<float>(value) / 100.0f; // 露出補正 (EV)
    emit setParams(params_);
  });

  // Contrast slider
  contrast_slider_ = new QSlider(Qt::Horizontal, this);
  contrast_slider_->setRange(0, 200);
  contrast_slider_->setValue(100);
  contrast_slider_->setTickPosition(QSlider::TicksBelow);
  contrast_slider_->setTickInterval(50);
  contrast_slider_->setSingleStep(1);
  label = new QLabel("contrast");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(contrast_slider_);
  lay->addLayout(hl);

  connect(contrast_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.contrast = static_cast<float>(value) / 100.0f; // コントラスト
    emit setParams(params_);
  });

  // Highlight slider
  highlight_slider_ = new QSlider(Qt::Horizontal, this);
  highlight_slider_->setRange(0, 100);
  highlight_slider_->setValue(0);
  highlight_slider_->setTickPosition(QSlider::TicksBelow);
  highlight_slider_->setTickInterval(10);
  highlight_slider_->setSingleStep(1);
  label = new QLabel("highlight");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(highlight_slider_);
  lay->addLayout(hl);

  connect(highlight_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.highlight = -static_cast<float>(value) / 100.0f; // ハイライト抑制
    emit setParams(params_);
  });

  // Shadow slider
  shadow_slider_ = new QSlider(Qt::Horizontal, this);
  shadow_slider_->setRange(0, 100);
  shadow_slider_->setValue(0);
  shadow_slider_->setTickPosition(QSlider::TicksBelow);
  shadow_slider_->setTickInterval(10);
  shadow_slider_->setSingleStep(1);
  label = new QLabel("shadow");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(shadow_slider_);
  lay->addLayout(hl);

  connect(shadow_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.shadow = static_cast<float>(value) / 100.0f; // シャドウ持ち上げ
    emit setParams(params_);
  });

  // Saturation slider
  saturation_slider_ = new QSlider(Qt::Horizontal, this);
  saturation_slider_->setRange(0, 200);
  saturation_slider_->setValue(100);
  saturation_slider_->setTickPosition(QSlider::TicksBelow);
  saturation_slider_->setTickInterval(50);
  saturation_slider_->setSingleStep(1);
  label = new QLabel("saturation");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(saturation_slider_);
  lay->addLayout(hl);

  connect(saturation_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.saturation = static_cast<float>(value) / 100.0f; // 彩度
    emit setParams(params_);
  });

  // Temperature slider
  temperature_slider_ = new QSlider(Qt::Horizontal, this);
  temperature_slider_->setRange(-100, 100);
  temperature_slider_->setValue(0);
  temperature_slider_->setTickPosition(QSlider::TicksBelow);
  temperature_slider_->setTickInterval(20);
  temperature_slider_->setSingleStep(1);
  label = new QLabel("temperature");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(temperature_slider_);
  lay->addLayout(hl);

  connect(temperature_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.temperature = static_cast<float>(value) / 100.0f; // 色温度
    emit setParams(params_);
  });

  // Tint slider
  tint_slider_ = new QSlider(Qt::Horizontal, this);
  tint_slider_->setRange(-100, 100);
  tint_slider_->setValue(0);
  tint_slider_->setTickPosition(QSlider::TicksBelow);
  tint_slider_->setTickInterval(20);
  tint_slider_->setSingleStep(1);
  label = new QLabel("tint");
  label->setMinimumWidth(label_minimum_width);

  hl = new QHBoxLayout();
  hl->addWidget(label);
  hl->addWidget(tint_slider_);
  lay->addLayout(hl);

  connect(tint_slider_, &QSlider::valueChanged, this, [this](int value) {
    params_.tint = static_cast<float>(value) / 100.0f; // 色合い
    emit setParams(params_);
  });

  // Send Botton
  auto *hl2 = new QHBoxLayout();
  auto *save_button = new QPushButton("Save", this);
  auto *reset_button = new QPushButton("Reset", this);
  auto *send_button = new QPushButton("Send", this);

  hl2->addWidget(save_button);
  hl2->addWidget(send_button);
  hl2->addWidget(reset_button);

  lay->addLayout(hl2);

  connect(save_button, &QPushButton::clicked, this,
          [this]() { emit saveImage(); });
  connect(send_button, &QPushButton::clicked, this,
          [this]() { emit sendImage(); });
  connect(reset_button, &QPushButton::clicked, this, [this]() {
    params_ = {};
    exposure_slider_->setValue(static_cast<int>(params_.exposure * 100));
    contrast_slider_->setValue(static_cast<int>(params_.contrast * 100));
    highlight_slider_->setValue(static_cast<int>(params_.highlight * 100));
    shadow_slider_->setValue(static_cast<int>(params_.shadow * 100));
    saturation_slider_->setValue(static_cast<int>(params_.saturation * 100));
    temperature_slider_->setValue(static_cast<int>(params_.temperature * 100));
    tint_slider_->setValue(static_cast<int>(params_.tint * 100));
    emit setParams(params_);
  });
}

} // namespace Application

#include "ToolWindow.moc"
