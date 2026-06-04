#ifndef NOUNOURSENGINE_H
#define NOUNOURSENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTranslator>

class MainWindow;
class MpvHandler;
class Settings;
class GestureHandler;
class OverlayHandler;
class UpdateManager;
class DimDialog;

class NounoursEngine : public QObject
{
    Q_OBJECT
public:
    explicit NounoursEngine(QObject *parent = 0);
    ~NounoursEngine();

    MainWindow     *window;
    MpvHandler     *mpv;
    Settings       *settings;
    GestureHandler *gesture;
    OverlayHandler *overlay;
    UpdateManager  *update;
    DimDialog      *dimDialog;

    QSystemTrayIcon *sysTrayIcon;
    QMenu           *trayIconMenu;

    QTranslator     *translator,
                    *qtTranslator;

    // input hash-table provides O(1) input-command lookups
    QHash<QString, QPair<QString, QString>> input; // [shortcut] = QPair<command, comment>

    // the following are the default input bindings
    // they are loaded into the input before parsing the settings file
    // when saving the settings file we don't save things that appear here
    const QHash<QString, QPair<QString, QString>> default_input = {
        {"Ctrl++",          {"mpv add sub-scale +0.1",              tr("Increase sub size")}},
        {"Ctrl+-",          {"mpv add sub-scale -0.1",              tr("Decrease sub size")}},
        {"Ctrl+W",          {"mpv cycle sub-visibility",            tr("Toggle subtitle visibility")}},
        {"Ctrl+R",          {"mpv set time-pos 0",                  tr("Restart playback")}},
        {"PgDown",          {"mpv add chapter +1",                  tr("Go to next chapter")}},
        {"PgUp",            {"mpv add chapter -1",                  tr("Go to previous chapter")}},
        {"Right",           {"mpv seek +5",                         tr("Seek forwards by %0 sec").arg("5")}},
        {"Left",            {"mpv seek -5",                         tr("Seek backwards by %0 sec").arg("5")}},
        {"Up",              {"mpv seek +30",                        tr("Seek forwards by %0 sec").arg("30")}},
        {"Down",            {"mpv seek -30",                        tr("Seek backwards by %0 sec").arg("30")}},
        {"Shift+Left",      {"mpv frame-back-step",                 tr("Frame step backwards")}},
        {"Shift+Right",     {"mpv frame-step",                      tr("Frame step")}},
        {"Ctrl+M",          {"mute",                                tr("Toggle mute audio")}},
        {"Ctrl+T",          {"screenshot subtitles",                tr("Take screenshot with subtitles")}},
        {"Ctrl+Shift+T",    {"screenshot",                          tr("Take screenshot without subtitles")}},
        {"Ctrl+Down",       {"volume -5",                           tr("Decrease volume")}},
        {"Ctrl+Up",         {"volume +5",                           tr("Increase volume")}},
        {"Ctrl+Shift+Up",   {"speed +0.1",                          tr("Increase playback speed by %0").arg("10")}},
        {"Ctrl+Shift+Down", {"speed -0.1",                          tr("Decrease playback speed by %0").arg("10")}},
        {"Ctrl+Shift+R",    {"speed 1.0",                           tr("Reset speed")}},
        {"Alt+Return",      {"fullscreen",                          tr("Toggle fullscreen")}},
        {"Ctrl+H",          {"hide_all_controls",                   tr("Toggle hide all controls mode")}},
        {"Ctrl+D",          {"dim",                                 tr("Dim lights")}},
        {"Ctrl+E",          {"show_in_folder",                      tr("Show the file in its folder")}},
        {"Tab",             {"media_info",                          tr("View media information")}},
        {"Ctrl+J",          {"jump",                                tr("Show jump to time dialog")}},
        {"Ctrl+N",          {"new",                                 tr("Open a new window")}},
        {"Ctrl+O",          {"open",                                tr("Show open file dialog")}},
        {"Ctrl+Q",          {"quit",                                tr("Quit")}},
        {"Ctrl+Right",      {"playlist play +1",                    tr("Play next file")}},
        {"Ctrl+Left",       {"playlist play -1",                    tr("Play previous file")}},
        {"Ctrl+S",          {"stop",                                tr("Stop playback")}},
        {"Ctrl+U",          {"open_location",                       tr("Show location dialog")}},
        {"Ctrl+V",          {"open_clipboard",                      tr("Open clipboard location")}},
        {"Ctrl+F",          {"playlist toggle",                     tr("Toggle playlist visibility")}},
        {"Ctrl+Z",          {"open_recent 0",                       tr("Open the last played file")}},
        {"Ctrl+`",          {"output",                              tr("Access command-line")}},
        {"F1",              {"online_help",                         tr("Launch online help")}},
        {"Space",           {"play_pause",                          tr("Play/Pause")}},
        {"Alt+1",           {"fitwindow",                           tr("Fit the window to the video")}},
        {"Alt+2",           {"fitwindow 50",                        tr("Fit window to %0%").arg("50")}},
        {"Alt+3",           {"fitwindow 75",                        tr("Fit window to %0%").arg("75")}},
        {"Alt+4",           {"fitwindow 100",                       tr("Fit window to %0%").arg("100")}},
        {"Alt+5",           {"fitwindow 150",                       tr("Fit window to %0%").arg("150")}},
        {"Alt+6",           {"fitwindow 200",                       tr("Fit window to %0%").arg("200")}},
        {"Esc",             {"boss",                                tr("Boss key")}},
        {"Return",          {"playlist play",                       tr("Play selected file on playlist")}},
        {"Del",             {"playlist remove",                     tr("Remove selected file from playlist")}}
    };

public slots:
    void LoadSettings();
    void SaveSettings();

    void Command(QString command);

protected slots:
    // Utility functions
    void Print(QString what, QString who = "nounours");
    void PrintLn(QString what, QString who = "nounours");
    void InvalidCommand(QString);
    void InvalidParameter(QString);
    void RequiresParameters(QString);

    // Settings Loading
    void Load2_0_3();

signals:


private:
    // This is a nounours-command hashtable initialized below
    //  by using a hash-table -> function pointer we acheive O(1) function lookups
    // Format: void NounoursCommand(QStringList args)
    // See nounourscommands.cpp for function definitions

    // todo: write advanced information about commands
    typedef void(NounoursEngine::*NounoursCommandFPtr)(QStringList&);
    const QHash<QString, QPair<NounoursCommandFPtr, QStringList>> NounoursCommandMap = {
        {"mpv", // command
         {&NounoursEngine::NounoursMpv,
          {
           // params     description
           QString(), tr("executes mpv command"),
           QString() // advanced
          }
         }
        },
        {"sh",
         {&NounoursEngine::NounoursSh,
          {
           QString(), tr("executes system shell command"),
           QString()
          }
         }
        },
        {"new",
         {&NounoursEngine::NounoursNew, // function pointer to command functionality
          {
           // params     description
           QString(), tr("creates a new instance of nounours-player"),
           QString()
          }
         }
        },
        {"open_location",
         {&NounoursEngine::NounoursOpenLocation,
          {
           QString(),
           tr("shows the open location dialog"),
           QString()
          }
         }
        },
        {"open_clipboard",
         {&NounoursEngine::NounoursOpenClipboard,
          {
           QString(),
           tr("opens the clipboard"),
           QString()
          }
         }
        },
        {"show_in_folder",
         {&NounoursEngine::NounoursShowInFolder,
          {
           QString(),
           tr("shows the current file in folder"),
           QString()
          }
         }
        },
        {"add_subtitles",
         {&NounoursEngine::NounoursAddSubtitles,
          {
           QString(),
           tr("add subtitles dialog"),
           QString()
          }
         }
        },
        {"add_audio",
         {&NounoursEngine::NounoursAddAudio,
          {
           QString(),
           tr("add audio track dialog"),
           QString()
          }
         }
        },
        {"screenshot",
         {&NounoursEngine::NounoursScreenshot,
          {
           tr("[subs]"),
           tr("take a screenshot (with subtitles if specified)"),
           QString()
          }
         }
        },
        {"media_info",
         {&NounoursEngine::NounoursMediaInfo,
          {
           QString(),
           tr("toggles media info display"),
           QString()
          }
         }
        },
        {"mediainfo_full",
         {&NounoursEngine::NounoursMediaInfoFull,
          {
           QString(),
           tr("show full MediaInfo details"),
           QString()
          }
         }
        },
        {"stop",
         {&NounoursEngine::NounoursStop,
          {
           QString(),
           tr("stops the current playback"),
           QString()
          }
         }
        },
        {"playlist",
         {&NounoursEngine::NounoursPlaylist,
          {
           "[...]",
           tr("playlist options"),
           QString()
          }
         }
        },
        {"jump",
         {&NounoursEngine::NounoursJump,
          {
           QString(),
           tr("opens jump dialog"),
           QString()
          }
         }
        },
        {"dim",
         {&NounoursEngine::NounoursDim,
          {
           QString(),
           tr("toggles dim desktop"),
           QString()
          }
         }
        },
        {"output",
         {&NounoursEngine::NounoursOutput,
          {
           QString(),
           tr("toggles output textbox"),
           QString()
          }
         }
        },
        {"preferences",
         {&NounoursEngine::NounoursPreferences,
          {
           QString(),
           tr("opens preferences dialog"),
           QString()
          }
         }
        },
        {"online_help",
         {&NounoursEngine::NounoursOnlineHelp,
          {
           QString(),
           tr("launches online help"),
           QString()
          }
         }
        },
        {"update",
         {&NounoursEngine::NounoursUpdate,
          {
           QString(),
           tr("opens the update dialog or updates youtube-dl"),
           QString()
          }
         }
        },
        {"open",
         {&NounoursEngine::NounoursOpen,
          {
           tr("[file]"),
           tr("opens the open file dialog or the file specified"),
           QString()
          }
         }
        },
        {"play_pause",
         {&NounoursEngine::NounoursPlayPause,
          {
           QString(),
           tr("toggle play/pause state"),
           QString()
          }
         }
        },
        {"fitwindow",
         {&NounoursEngine::NounoursFitWindow,
          {
           tr("[percent]"),
           tr("fit the window"),
           QString()
          }
         }
        },
        {"deinterlace",
         {&NounoursEngine::NounoursDeinterlace,
          {
           QString(),
           tr("toggle deinterlace"),
           QString()
          }
         }
        },
        {"interpolate",
         {&NounoursEngine::NounoursInterpolate,
          {
           QString(),
           tr("toggle motion interpolation"),
           QString()
          }
         }
        },
        {"mute",
         {&NounoursEngine::NounoursMute,
          {
           QString(),
           tr("mutes the audio"),
           QString()
          },
         }
        },
        {"volume",
         {&NounoursEngine::NounoursVolume,
          {
           tr("[level]"),
           tr("adjusts the volume"),
           QString()
          }
         }
        },
        {"speed",
         {&NounoursEngine::NounoursSpeed,
          {
           tr("[ratio]"),
           tr("adjusts the speed"),
           QString()
          }
         }
        },
        {"fullscreen",
         {&NounoursEngine::NounoursFullScreen,
          {
           QString(),
           tr("toggles fullscreen state"),
           QString()
          }
         }
        },
        {"hide_all_controls",
         {&NounoursEngine::NounoursHideAllControls,
          {
           QString(),
           tr("toggles hide all controls state"),
           QString()
          }
         }
        },
        {"boss",
         {&NounoursEngine::NounoursBoss,
          {
           QString(),
           tr("pause and hide the window"),
           QString()
          }
         }
        },
        {"clear",
         {&NounoursEngine::NounoursClear,
          {
           QString(),
           tr("clears the output textbox"),
           QString()
          }
         }
        },
        {"help",
         {&NounoursEngine::NounoursHelp,
          {
           tr("[command]"),
           tr("internal help menu"),
           QString()
          }
         }
        },
        {"about",
         {&NounoursEngine::NounoursAbout,
          {
           tr("[qt]"),
           tr("open about dialog"),
           QString()
          }
         }
        },
        {"msg_level",
         {&NounoursEngine::NounoursMsgLevel,
          {
           tr("[level]"),
           tr("set mpv msg-level"),
           QString()
          }
         }
        },
        {"quit",
         {&NounoursEngine::NounoursQuit,
          {
           QString(),
           tr("quit nounours-player"),
           QString()
          }
         }
        }
    };
    // Nounours Command Functions
    void NounoursMpv(QStringList&);
    void NounoursSh(QStringList&);
    void NounoursNew(QStringList&);
    void NounoursOpenLocation(QStringList&);
    void NounoursOpenClipboard(QStringList&);
    void NounoursShowInFolder(QStringList&);
    void NounoursAddSubtitles(QStringList&);
    void NounoursAddAudio(QStringList&);
    void NounoursScreenshot(QStringList&);
    void NounoursMediaInfo(QStringList&);
    void NounoursMediaInfoFull(QStringList&);
    void NounoursStop(QStringList&);
    void NounoursPlaylist(QStringList&);
    void NounoursJump(QStringList&);
    void NounoursDim(QStringList&);
    void NounoursOutput(QStringList&);
    void NounoursPreferences(QStringList&);
    void NounoursOnlineHelp(QStringList&);
    void NounoursUpdate(QStringList&);
    void NounoursOpen(QStringList&);
    void NounoursPlayPause(QStringList&);
    void NounoursFitWindow(QStringList&);
    void NounoursAspect(QStringList&);
    void NounoursDeinterlace(QStringList&);
    void NounoursInterpolate(QStringList&);
    void NounoursMute(QStringList&);
    void NounoursVolume(QStringList&);
    void NounoursSpeed(QStringList&);
    void NounoursFullScreen(QStringList&);
    void NounoursHideAllControls(QStringList&);
    void NounoursBoss(QStringList&);
    void NounoursClear(QStringList&);
    void NounoursHelp(QStringList&);
    void NounoursAbout(QStringList&);
    void NounoursMsgLevel(QStringList&);
    void NounoursQuit(QStringList&);
public:
    void Open();
    void OpenLocation();
    void Screenshot(bool subs);
    void MediaInfo(bool show);
    void MediaInfoFull();
    void PlayPause();
    void Jump();
    void FitWindow(int percent = 0, bool msg = true);
    void Dim(bool dim);
    void About(QString what = QString());
    void Quit();
};

#endif // NOUNOURSENGINE_H
