import sys
import io
from PyQt6.QtWidgets import (
    QApplication, QLabel, QSlider, QVBoxLayout, QHBoxLayout,
    QWidget, QFileDialog, QMainWindow, QSizePolicy
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize
from PyQt6.QtGui import QPixmap, QImage, QResizeEvent
from PIL import Image, ImageEnhance
import numpy as np

class ImageProcessor(QThread):
    processed = pyqtSignal(Image.Image)

    def __init__(self, original: Image.Image):
        super().__init__()
        self.original = original
        self.params = {
            "brightness": 1.0,
            "contrast": 1.0,
            "saturation": 1.0,
            "highlights": 1.0,
            "shadows": 1.0
        }
        self._abort = False

    def update_params(self, new_params: dict):
        self.params.update(new_params)
        if not self.isRunning():
            self.start()

    def run(self):
        if self._abort:
            return
        img = self.original.copy()

        img = ImageEnhance.Brightness(img).enhance(self.params["brightness"])
        img = ImageEnhance.Contrast(img).enhance(self.params["contrast"])
        img = ImageEnhance.Color(img).enhance(self.params["saturation"])

        np_img = np.array(img).astype(np.float32) / 255.0
        luminance = np.mean(np_img, axis=2, keepdims=True)

        shadow_mask = (luminance < 0.5).astype(np.float32)
        np_img += shadow_mask * (1.0 - np_img) * (self.params["shadows"] - 1.0)

        highlight_mask = (luminance >= 0.5).astype(np.float32)
        np_img *= (1.0 + (self.params["highlights"] - 1.0) * (1.0 - luminance) * highlight_mask)

        np_img = np.clip(np_img, 0, 1)
        img = Image.fromarray((np_img * 255).astype(np.uint8))

        if not self._abort:
            self.processed.emit(img)

    def abort(self):
        self._abort = True

class ResizableLabel(QLabel):
    def __init__(self):
        super().__init__()
        self.setScaledContents(False)
        self.image = None

    def set_image(self, img: Image.Image):
        self.image = img
        self.update_pixmap()

    def resizeEvent(self, event: QResizeEvent):
        self.update_pixmap()
        super().resizeEvent(event)

    def update_pixmap(self):
        if self.image is None:
            return

        label_ratio = self.width() / self.height()
        img_ratio = self.image.width / self.image.height

        if img_ratio > label_ratio:
            new_width = self.width()
            new_height = int(self.width() / img_ratio)
        else:
            new_height = self.height()
            new_width = int(self.height() * img_ratio)

        resized = self.image.copy().resize((new_width, new_height))
        buf = io.BytesIO()
        resized.save(buf, format="PNG")
        qt_img = QImage.fromData(buf.getvalue())
        self.setPixmap(QPixmap.fromImage(qt_img))

class ImageEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Photo Editor (Resizable Preview)")
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        self.image_layout = QHBoxLayout()
        self.control_layout = QVBoxLayout()
        layout = QVBoxLayout()
        layout.addLayout(self.image_layout)
        layout.addLayout(self.control_layout)
        self.central_widget.setLayout(layout)

        file_path, _ = QFileDialog.getOpenFileName(self, "Open Image", "", "Images (*.png *.jpg *.bmp)")
        if not file_path:
            sys.exit()
        self.original = Image.open(file_path).convert("RGB")
        self.processed = self.original.copy()

        self.label_original = ResizableLabel()
        self.label_processed = ResizableLabel()
        self.label_original.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.label_processed.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.image_layout.addWidget(self.label_original)
        self.image_layout.addWidget(self.label_processed)

        self.update_images()

        self.controls = {}
        self.processor = ImageProcessor(self.original)
        self.processor.processed.connect(self.set_processed_image)

        for name in ["Brightness", "Contrast", "Saturation", "Highlights", "Shadows"]:
            label = QLabel(name)
            slider = QSlider(Qt.Orientation.Horizontal)
            slider.setMinimum(0)
            slider.setMaximum(200)
            slider.setValue(100)
            slider.valueChanged.connect(self.slider_changed)
            self.control_layout.addWidget(label)
            self.control_layout.addWidget(slider)
            self.controls[name.lower()] = slider

    def slider_changed(self):
        params = {k: v.value() / 100 for k, v in self.controls.items()}
        self.processor.update_params(params)

    def set_processed_image(self, img: Image.Image):
        self.processed = img
        self.update_images()

    def update_images(self):
        self.label_original.set_image(self.original)
        self.label_processed.set_image(self.processed)

def main():
    app = QApplication(sys.argv)
    editor = ImageEditor()
    editor.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
