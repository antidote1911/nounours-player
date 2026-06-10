#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "nounoursengine.h"
#include "ui/mainwindow.h"
#include "mpvhandler.h"
#include "jellyfinmanager.h"
#include "ui/keydialog.h"

#include <QFileDialog>
#include <QMessageBox>

PreferencesDialog::PreferencesDialog(NounoursEngine *nounours, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    nounours(nounours),
    screenshotDir("")
{
    ui->setupUi(this);

    ui->infoWidget->sortByColumn(0, Qt::AscendingOrder);
    sortLock = new SortLock(ui->infoWidget);

    PopulateLangs();

    QString ontop = nounours->window->getOnTop();
    if(ontop == "never")
        ui->neverRadioButton->setChecked(true);
    else if(ontop == "playing")
        ui->playingRadioButton->setChecked(true);
    else if(ontop == "always")
        ui->alwaysRadioButton->setChecked(true);
    ui->resumeCheckBox->setChecked(nounours->window->getResume());
    ui->groupBox_2->setChecked(nounours->sysTrayIcon->isVisible());
    ui->hidePopupCheckBox->setChecked(nounours->window->getHidePopup());
    ui->gestureCheckBox->setChecked(nounours->window->getGestures());
    ui->leftClickPlayPauseCheckBox->setChecked(nounours->window->getLeftClickPlayPause());
    ui->langComboBox->setCurrentText(nounours->window->getLang());
    int autofit = nounours->window->getAutoFit();
    ui->autoFitCheckBox->setChecked((bool)autofit);
    ui->comboBox->setCurrentText(QString::number(autofit)+"%");
    ui->gestureCheckBox->setChecked(nounours->window->getGestures());
    int maxRecent= nounours->window->getMaxRecent();
    ui->recentCheckBox->setChecked(maxRecent > 0);
    ui->recentSpinBox->setValue(maxRecent);
    ui->resumeCheckBox->setChecked(nounours->window->getResume());
    ui->formatComboBox->setCurrentText(nounours->mpv->getScreenshotFormat());
    screenshotDir = QDir::toNativeSeparators(nounours->mpv->getScreenshotDir());
    ui->templateLineEdit->setText(nounours->mpv->getScreenshotTemplate());
    ui->msgLvlComboBox->setCurrentText(nounours->mpv->getMsgLevel());

    // Video output: populate driver list, then select the current vo
    // (if not in the list, add it as a custom entry so nothing is lost)
    PopulateVoList();
    QString currentVo = nounours->mpv->getVo();
    if(ui->voComboBox->findData(currentVo) == -1)
        ui->voComboBox->addItem(currentVo, currentVo);
    ui->voComboBox->setCurrentIndex(ui->voComboBox->findData(currentVo));
    UpdateVoDescription(currentVo);

    // Hardware decoding: populate driver list, then select the current hwdec
    PopulateHwdecList();
    QString currentHwdec = nounours->mpv->getHwdec();
    if(ui->hwdecComboBox->findData(currentHwdec) == -1)
        ui->hwdecComboBox->addItem(currentHwdec, currentHwdec);
    ui->hwdecComboBox->setCurrentIndex(ui->hwdecComboBox->findData(currentHwdec));
    UpdateHwdecDescription(currentHwdec);

    // Frame dropping
    QString framedrop = nounours->mpv->getFramedrop();
    ui->framedropCheckBox->setChecked(framedrop != "no");
    ui->hardFramedropCheckBox->setChecked(framedrop == "decoder+vo");
    ui->hardFramedropCheckBox->setEnabled(ui->framedropCheckBox->isChecked());

    // Decoding threads and H.264 deblocking filter
    ui->threadsSpinBox->setValue(nounours->mpv->getVdLavcThreads());
    ui->deblockComboBox->setCurrentIndex(nounours->mpv->getSkipLoopFilter() == "all" ? 1 : 0);

    // Jellyfin server connection
    ui->jellyfinUrlEdit->setText(nounours->jellyfin->getServerUrl());
    ui->jellyfinUserEdit->setText(nounours->jellyfin->getUsername());
    ui->jellyfinPasswordEdit->setText(nounours->jellyfin->getPassword());

    // add shortcuts
    saved = nounours->input;
    PopulateShortcuts();

    connect(ui->autoFitCheckBox, &QCheckBox::clicked,
            [=](bool b)
            {
                ui->comboBox->setEnabled(b);
            });

    connect(ui->voComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index)
            {
                UpdateVoDescription(ui->voComboBox->itemData(index).toString());
            });

    connect(ui->hwdecComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index)
            {
                UpdateHwdecDescription(ui->hwdecComboBox->itemData(index).toString());
            });

    connect(ui->framedropCheckBox, &QCheckBox::toggled,
            [=](bool b)
            {
                ui->hardFramedropCheckBox->setEnabled(b);
                if(!b)
                    ui->hardFramedropCheckBox->setChecked(false);
            });

    connect(ui->changeButton, &QPushButton::clicked,
            [=]
            {
                QString dir = QFileDialog::getExistingDirectory(this, tr("Choose screenshot directory"), screenshotDir);
                if(dir != QString())
                    screenshotDir = dir;
            });

    connect(ui->addKeyButton, &QPushButton::clicked,
            [=]
            {
                SelectKey(true);
            });

    connect(ui->editKeyButton, &QPushButton::clicked,
            [=]
            {
                int i = ui->infoWidget->currentRow();
                if(i == -1)
                    return;

                SelectKey(false,
                    {ui->infoWidget->item(i, 0)->text(),
                    {ui->infoWidget->item(i, 1)->text(),
                     ui->infoWidget->item(i, 2)->text()}});
            });

    connect(ui->resetKeyButton, &QPushButton::clicked,
            [=]
            {
                if(QMessageBox::question(this, tr("Reset All Key Bindings?"), tr("Are you sure you want to reset all shortcut keys to its original bindings?")) == QMessageBox::Yes)
                {
                    nounours->input = nounours->default_input;
                    while(numberOfShortcuts > 0)
                        RemoveRow(numberOfShortcuts-1);
                    PopulateShortcuts();
                }
            });

    connect(ui->removeKeyButton, &QPushButton::clicked,
            [=]
            {
                int row = ui->infoWidget->currentRow();
                if(row == -1)
                    return;

                nounours->input[ui->infoWidget->item(row, 0)->text()] = {QString(), QString()};
                RemoveRow(row);
            });

    connect(ui->infoWidget, &QTableWidget::currentCellChanged,
            [=](int r,int,int,int)
            {
                ui->editKeyButton->setEnabled(r != -1);
                ui->removeKeyButton->setEnabled(r != -1);
            });

    connect(ui->infoWidget, &QTableWidget::doubleClicked,
            [=](const QModelIndex &index)
            {
                int i = index.row();
                SelectKey(false,
                    {ui->infoWidget->item(i, 0)->text(),
                    {ui->infoWidget->item(i, 1)->text(),
                     ui->infoWidget->item(i, 2)->text()}});
            });

    connect(ui->jellyfinTestButton, &QPushButton::clicked,
            [=]
            {
                ui->jellyfinStatusLabel->setText(tr("Connecting..."));
                nounours->jellyfin->Connect(ui->jellyfinUrlEdit->text(), ui->jellyfinUserEdit->text(), ui->jellyfinPasswordEdit->text());
            });

    connect(nounours->jellyfin, &JellyfinManager::connectedSignal, this,
            [=]
            {
                ui->jellyfinStatusLabel->setText(tr("Connection successful."));
            });

    connect(nounours->jellyfin, &JellyfinManager::connectionFailedSignal, this,
            [=](QString error)
            {
                ui->jellyfinStatusLabel->setText(tr("Connection failed: %0").arg(error));
            });

    connect(ui->recentCheckBox, SIGNAL(toggled(bool)),
            ui->recentSpinBox, SLOT(setEnabled(bool)));

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(accept()));

    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
}

PreferencesDialog::~PreferencesDialog()
{
    if(result() == QDialog::Accepted)
    {
        nounours->window->setResume(ui->resumeCheckBox->isChecked());
        if(ui->neverRadioButton->isChecked())
            nounours->window->setOnTop("never");
        else if(ui->playingRadioButton->isChecked())
            nounours->window->setOnTop("playing");
        else if(ui->alwaysRadioButton->isChecked())
            nounours->window->setOnTop("always");
        nounours->sysTrayIcon->setVisible(ui->groupBox_2->isChecked());
        nounours->window->setHidePopup(ui->hidePopupCheckBox->isChecked());
        nounours->window->setGestures(ui->gestureCheckBox->isChecked());
        nounours->window->setLeftClickPlayPause(ui->leftClickPlayPauseCheckBox->isChecked());
        nounours->window->setLang(ui->langComboBox->currentText());
        if(ui->autoFitCheckBox->isChecked())
            nounours->window->setAutoFit(ui->comboBox->currentText().left(ui->comboBox->currentText().length()-1).toInt());
        else
            nounours->window->setAutoFit(0);
        nounours->window->setMaxRecent(ui->recentCheckBox->isChecked() ? ui->recentSpinBox->value() : 0);
        nounours->window->setGestures(ui->gestureCheckBox->isChecked());
        nounours->window->setResume(ui->resumeCheckBox->isChecked());
        nounours->mpv->ScreenshotFormat(ui->formatComboBox->currentText());
        nounours->mpv->ScreenshotDirectory(screenshotDir);
        nounours->mpv->ScreenshotTemplate(ui->templateLineEdit->text());
        nounours->mpv->MsgLevel(ui->msgLvlComboBox->currentText());
        nounours->mpv->Vo(ui->voComboBox->currentData().toString());
        nounours->mpv->Hwdec(ui->hwdecComboBox->currentData().toString());
        nounours->mpv->Framedrop(!ui->framedropCheckBox->isChecked() ? "no" :
                                  ui->hardFramedropCheckBox->isChecked() ? "decoder+vo" : "vo");
        nounours->mpv->VdLavcThreads(ui->threadsSpinBox->value());
        nounours->mpv->SkipLoopFilter(ui->deblockComboBox->currentIndex() == 1 ? "all" : "default");
        nounours->jellyfin->ServerUrl(ui->jellyfinUrlEdit->text());
        nounours->jellyfin->Username(ui->jellyfinUserEdit->text());
        nounours->jellyfin->Password(ui->jellyfinPasswordEdit->text());
        nounours->window->MapShortcuts();
    }
    else
        nounours->input = saved;
    delete sortLock;
    delete ui;
}

void PreferencesDialog::showPreferences(NounoursEngine *nounours, QWidget *parent)
{
    PreferencesDialog dialog(nounours, parent);
    dialog.exec();
}

void PreferencesDialog::PopulateVoList()
{
    ui->voComboBox->addItem(tr("Auto (recommended)"), "");
    ui->voComboBox->addItem("gpu-next", "gpu-next");
    ui->voComboBox->addItem("gpu", "gpu");
    ui->voComboBox->addItem("x11", "x11");
    ui->voComboBox->addItem("xv", "xv");
    ui->voComboBox->addItem("wlshm", "wlshm");
    ui->voComboBox->addItem("dmabuf-wayland", "dmabuf-wayland");
    ui->voComboBox->addItem("vdpau", "vdpau");
    ui->voComboBox->addItem("vaapi", "vaapi");
}

void PreferencesDialog::UpdateVoDescription(const QString &vo)
{
    static const QHash<QString, QString> descriptions = {
        {"", tr("Let mpv pick the best available driver automatically.")},
        {"gpu-next", tr("New GPU video output driver based on libplacebo. Recommended: best image quality and performance on modern systems.")},
        {"gpu", tr("Default GPU video output driver, supports OpenGL, Vulkan and Direct3D. Stable and widely compatible.")},
        {"x11", tr("Basic X11 output without hardware acceleration. Slow, only useful as a fallback.")},
        {"xv", tr("X11 output using the Xv extension. Basic hardware acceleration for older X11 systems.")},
        {"wlshm", tr("Wayland output using shared memory, without GPU acceleration. Compatible but less efficient.")},
        {"dmabuf-wayland", tr("Hardware-accelerated zero-copy output for Wayland compositors that support DMA-BUF.")},
        {"vdpau", tr("Hardware-accelerated output using VDPAU (NVIDIA GPUs on Linux).")},
        {"vaapi", tr("Hardware-accelerated output using VA-API (Intel/AMD GPUs on Linux).")},
    };

    ui->label_vo_desc->setText(descriptions.value(vo, tr("Custom or unrecognized video output driver.")));
}

void PreferencesDialog::PopulateHwdecList()
{
    ui->hwdecComboBox->addItem(tr("Disabled"), "no");
    ui->hwdecComboBox->addItem(tr("Auto (recommended)"), "auto");
    ui->hwdecComboBox->addItem(tr("Auto (copy)"), "auto-copy");
    ui->hwdecComboBox->addItem("VA-API", "vaapi");
    ui->hwdecComboBox->addItem(tr("VA-API (copy)"), "vaapi-copy");
    ui->hwdecComboBox->addItem("VDPAU", "vdpau");
    ui->hwdecComboBox->addItem("NVDEC", "nvdec");
    ui->hwdecComboBox->addItem(tr("NVDEC (copy)"), "nvdec-copy");
    ui->hwdecComboBox->addItem("D3D11VA", "d3d11va");
    ui->hwdecComboBox->addItem(tr("D3D11VA (copy)"), "d3d11va-copy");
    ui->hwdecComboBox->addItem("DXVA2", "dxva2");
    ui->hwdecComboBox->addItem("Vulkan", "vulkan");
    ui->hwdecComboBox->addItem(tr("Vulkan (copy)"), "vulkan-copy");
}

void PreferencesDialog::UpdateHwdecDescription(const QString &hwdec)
{
    static const QHash<QString, QString> descriptions = {
        {"no", tr("No hardware decoding: all video is decoded on the CPU.")},
        {"auto", tr("Automatically pick the best hardware decoder for your system. Recommended.")},
        {"auto-copy", tr("Like Auto, but copies decoded frames back to system memory so video filters keep working.")},
        {"vaapi", tr("Hardware decoding via VA-API (Intel/AMD GPUs on Linux).")},
        {"vaapi-copy", tr("VA-API with frames copied to system memory, for compatibility with video filters.")},
        {"vdpau", tr("Hardware decoding via VDPAU (older NVIDIA GPUs on Linux).")},
        {"nvdec", tr("Hardware decoding via NVDEC (NVIDIA GPUs).")},
        {"nvdec-copy", tr("NVDEC with frames copied to system memory, for compatibility with video filters.")},
        {"d3d11va", tr("Hardware decoding via Direct3D 11 (Windows).")},
        {"d3d11va-copy", tr("D3D11VA with frames copied to system memory, for compatibility with video filters.")},
        {"dxva2", tr("Hardware decoding via DirectX Video Acceleration 2 (Windows, legacy).")},
        {"vulkan", tr("Hardware decoding via Vulkan video (cross-platform).")},
        {"vulkan-copy", tr("Vulkan video decoding with frames copied to system memory, for compatibility with video filters.")},
    };

    ui->label_hwdec_desc->setText(descriptions.value(hwdec, tr("Custom or unrecognized hardware decoding mode.")));
}

void PreferencesDialog::PopulateLangs()
{
    // open the language directory
    QDir root(NOUNOURS_PLAYER_LANG_PATH);
    // get files in the directory with .qm extension
    QFileInfoList flist;
    flist = root.entryInfoList({"*.qm"}, QDir::Files);
    // add the languages to the combo box
    ui->langComboBox->addItem("auto");
    ui->langComboBox->addItem("en");
    for(auto &i : flist)
    {
        QString lang = i.fileName().mid(i.fileName().indexOf("_") + 1); // nounours-player_....
        lang.chop(3); // -  .qm
        ui->langComboBox->addItem(lang);
    }
}

void PreferencesDialog::PopulateShortcuts()
{
    sortLock->lock();
    numberOfShortcuts = 0;
    for(auto iter = nounours->input.begin(); iter != nounours->input.end(); ++iter)
    {
        QPair<QString, QString> p = iter.value();
        if(p.first == QString() || p.second == QString())
            continue;
        AddRow(iter.key(), p.first, p.second);
    }
    sortLock->unlock();
}

void PreferencesDialog::AddRow(QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->insertRow(numberOfShortcuts);
    ui->infoWidget->setItem(numberOfShortcuts, 0, new QTableWidgetItem(first));
    ui->infoWidget->setItem(numberOfShortcuts, 1, new QTableWidgetItem(second));
    ui->infoWidget->setItem(numberOfShortcuts, 2, new QTableWidgetItem(third));
    ++numberOfShortcuts;
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::ModifyRow(int row, QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->item(row, 0)->setText(first);
    ui->infoWidget->item(row, 1)->setText(second);
    ui->infoWidget->item(row, 2)->setText(third);
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::RemoveRow(int row)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->removeCellWidget(row, 0);
    ui->infoWidget->removeCellWidget(row, 1);
    ui->infoWidget->removeCellWidget(row, 2);
    ui->infoWidget->removeRow(row);
    --numberOfShortcuts;
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::SelectKey(bool add, QPair<QString, QPair<QString, QString>> init)
{
    sortLock->lock();
    KeyDialog dialog(this);
    int status = 0;
    while(status != 2)
    {
        QPair<QString, QPair<QString, QString>> result = dialog.SelectKey(add, init);
        if(result == QPair<QString, QPair<QString, QString>>()) // cancel
            break;
        for(int i = 0; i < numberOfShortcuts; ++i)
        {
            if(!add && i == ui->infoWidget->currentRow()) // don't compare selected row if we're changing
                continue;
            if(ui->infoWidget->item(i, 0)->text() == result.first)
            {
                if(QMessageBox::question(this,
                       tr("Existing keybinding"),
                       tr("%0 is already being used. Would you like to change its function?").arg(
                           result.first)) == QMessageBox::Yes)
                {
                    nounours->input[ui->infoWidget->item(i, 0)->text()] = {QString(), QString()};
                    RemoveRow(i);
                    status = 0;
                }
                else
                {
                    init = result;
                    status = 1;
                }
                break;
            }
        }
        if(status == 0)
        {
            if(add) // add
                AddRow(result.first, result.second.first, result.second.second);
            else // change
            {
                if(result.first != init.first)
                    nounours->input[init.first] = {QString(), QString()};
                ModifyRow(ui->infoWidget->currentRow(), result.first, result.second.first, result.second.second);
            }
            nounours->input[result.first] = result.second;
            status = 2;
        }
        else
            status = 0;
    }
    sortLock->unlock();
}

PreferencesDialog::SortLock::SortLock(QTableWidget *parent):parent(parent) {}

void PreferencesDialog::SortLock::lock()
{
    parent->setSortingEnabled(false);
    QMutex::lock();
}

void PreferencesDialog::SortLock::unlock()
{
    QMutex::unlock();
    parent->setSortingEnabled(true);
}
