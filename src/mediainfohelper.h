#ifndef MEDIAINFOHELPER_H
#define MEDIAINFOHELPER_H

#include <QString>

namespace MediaInfoLib { class MediaInfo; }

struct AudioTrackInfo {
    QString codecName;  // "E-AC-3", "DTS", "TrueHD", "AAC", …
    QString channels;   // "Mono", "Stereo", "5.1", "7.1", "Nch"
    QString hint;       // "Dolby Atmos", "DTS:X", "DTS-HD", or ""
};

struct VideoTrackInfo {
    QString codec;      // "HEVC", "AVC", "AV1", "VP9", …
    QString hdrFormat;  // "Dolby Vision", "HDR10+", "HDR10", "HLG", combinations, or ""
};

class MediaInfoHelper
{
public:
    explicit MediaInfoHelper(const QString &filePath);
    ~MediaInfoHelper();

    bool isValid() const { return valid; }

    AudioTrackInfo audioTrack(int index) const;
    VideoTrackInfo videoTrack(int index = 0) const;
    QString fullReport() const;

    struct AudioLabelParts {
        QString main;  // detected info: "1: VFF E-AC-3 5.1"
        QString sub;   // track name remainder (shown grayed), may be empty
    };

    static AudioTrackInfo fromMpvFallback(const QString &demuxChannels, const QString &codec);
    static AudioLabelParts formatAudioLabelParts(int id, const QString &lang, const QString &title, const AudioTrackInfo &info);

private:
    MediaInfoLib::MediaInfo *mi = nullptr;
    bool valid = false;
};

#endif // MEDIAINFOHELPER_H
