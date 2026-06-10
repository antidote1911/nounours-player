#include "videoequalizer.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>

#include "../mpvhandler.h"

VideoEqualizerDialog::VideoEqualizerDialog(MpvHandler *mpv, QWidget *parent)
    : QDialog(parent), mpv(mpv)
{
    setWindowTitle(tr("Video Equalizer"));
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    enabledCheckBox = new QCheckBox(tr("Enable color adjustments"), this);
    enabledCheckBox->setChecked(mpv->getEqEnabled());

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    addRow(form, tr("Brightness"), brightnessSlider, brightnessSpinBox, mpv->getBrightness(), &MpvHandler::Brightness);
    addRow(form, tr("Contrast"),   contrastSlider,   contrastSpinBox,   mpv->getContrast(),   &MpvHandler::Contrast);
    addRow(form, tr("Saturation"), saturationSlider, saturationSpinBox, mpv->getSaturation(), &MpvHandler::Saturation);
    addRow(form, tr("Gamma"),      gammaSlider,      gammaSpinBox,      mpv->getGamma(),      &MpvHandler::Gamma);
    addRow(form, tr("Hue"),        hueSlider,        hueSpinBox,        mpv->getHue(),        &MpvHandler::Hue);

    resetBtn = new QPushButton(tr("Reset all"), this);
    connect(resetBtn, &QPushButton::clicked, this, &VideoEqualizerDialog::resetAll);
    eqWidgets.append(resetBtn);

    for(auto *w : eqWidgets)
        w->setEnabled(enabledCheckBox->isChecked());

    connect(enabledCheckBox, &QCheckBox::toggled,
            this, [=](bool checked)
            {
                mpv->EqEnabled(checked);
                for(auto *w : eqWidgets)
                    w->setEnabled(checked);
            });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto *root = new QVBoxLayout(this);
    root->addWidget(enabledCheckBox);
    root->addLayout(form);
    root->addWidget(resetBtn);
    root->addWidget(buttons);
}

void VideoEqualizerDialog::addRow(QFormLayout *form, const QString &label,
                                   QSlider *&slider, QSpinBox *&spin,
                                   int currentValue,
                                   void (MpvHandler::*setter)(int))
{
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(-100, 100);
    slider->setValue(currentValue);
    slider->setMinimumWidth(240);

    spin = new QSpinBox(this);
    spin->setRange(-100, 100);
    spin->setValue(currentValue);
    spin->setFixedWidth(60);

    connect(slider, &QSlider::valueChanged, spin,   &QSpinBox::setValue);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged, this, [=](int v) { (mpv->*setter)(v); });

    auto *rowResetBtn = new QPushButton("↺", this);
    rowResetBtn->setFixedWidth(26);
    rowResetBtn->setToolTip(tr("Reset"));
    connect(rowResetBtn, &QPushButton::clicked, this, [=] { slider->setValue(0); });

    eqWidgets.append(slider);
    eqWidgets.append(spin);
    eqWidgets.append(rowResetBtn);

    auto *row = new QHBoxLayout;
    row->addWidget(slider);
    row->addWidget(spin);
    row->addWidget(rowResetBtn);
    form->addRow(label, row);
}

void VideoEqualizerDialog::resetAll()
{
    for(auto *s : {brightnessSlider, contrastSlider, saturationSlider, gammaSlider, hueSlider})
        s->setValue(0);
}
