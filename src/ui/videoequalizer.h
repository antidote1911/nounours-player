#ifndef VIDEOEQUALIZER_H
#define VIDEOEQUALIZER_H

#include <QDialog>

class QSlider;
class QSpinBox;
class MpvHandler;

class VideoEqualizerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VideoEqualizerDialog(MpvHandler *mpv, QWidget *parent = nullptr);

private:
    MpvHandler *mpv;

    QSlider  *brightnessSlider,  *contrastSlider,  *saturationSlider,  *gammaSlider,  *hueSlider;
    QSpinBox *brightnessSpinBox, *contrastSpinBox, *saturationSpinBox, *gammaSpinBox, *hueSpinBox;

    void addRow(class QFormLayout *form, const QString &label,
                QSlider *&slider, QSpinBox *&spin, int currentValue,
                void (MpvHandler::*setter)(int));
    void resetAll();
};

#endif // VIDEOEQUALIZER_H
