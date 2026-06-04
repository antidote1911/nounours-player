#include "mediainfohelper.h"

#ifndef UNICODE
#  define UNICODE
#endif
#include <MediaInfo/MediaInfo.h>
#include <QStringList>

MediaInfoHelper::MediaInfoHelper(const QString &filePath)
{
    mi = new MediaInfoLib::MediaInfo();
    valid = !filePath.isEmpty() && mi->Open(filePath.toStdWString()) == 1;
}

MediaInfoHelper::~MediaInfoHelper()
{
    if(mi) { mi->Close(); delete mi; }
}

AudioTrackInfo MediaInfoHelper::audioTrack(int index) const
{
    AudioTrackInfo info;
    if(!valid) return info;

    int n = QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Audio, index, L"Channel(s)")).toInt();
    if(n == 1)      info.channels = "Mono";
    else if(n == 2) info.channels = "Stereo";
    else if(n == 6) info.channels = "5.1";
    else if(n == 8) info.channels = "7.1";
    else if(n > 0)  info.channels = QString::number(n) + "ch";

    QString fmt = QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Audio, index, L"Format"));
    QString add = QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Audio, index, L"Format_AdditionalFeatures"));
    QString com = QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Audio, index, L"Format_Commercial_IfAny"));

    info.codecName = fmt.startsWith("MLP", Qt::CaseInsensitive) ? "TrueHD" : fmt;

    if(add.contains("JOC", Qt::CaseInsensitive) || com.contains("Atmos", Qt::CaseInsensitive))
        info.hint = "Dolby Atmos";
    else if(add.contains("XLL X", Qt::CaseInsensitive) || com.contains("DTS:X", Qt::CaseInsensitive) || com.contains("DTS-X", Qt::CaseInsensitive))
        info.hint = "DTS:X";
    else if(fmt.contains("DTS", Qt::CaseInsensitive) && add.contains("XLL", Qt::CaseInsensitive))
        info.hint = "DTS-HD";

    return info;
}

AudioTrackInfo MediaInfoHelper::fromMpvFallback(const QString &demuxChannels, const QString &codec)
{
    AudioTrackInfo info;
    info.codecName = codec;

    QString ch = demuxChannels;
    int paren = ch.indexOf('(');
    if(paren != -1) ch = ch.left(paren).trimmed();
    if(ch.startsWith("unknown", Qt::CaseInsensitive) && ch.size() > 7) {
        int n = ch.mid(7).toInt();
        if(n == 1)      ch = "Mono";
        else if(n == 2) ch = "Stereo";
        else if(n == 6) ch = "5.1";
        else if(n == 8) ch = "7.1";
        else if(n > 2)  ch = QString::number(n) + "ch";
    }
    info.channels = ch;
    return info;
}

QString MediaInfoHelper::fullReport() const
{
    if(!valid) return QString();
    mi->Option(L"Complete", L"1");
    return QString::fromStdWString(mi->Inform());
}

VideoTrackInfo MediaInfoHelper::videoTrack(int index) const
{
    VideoTrackInfo info;
    if(!valid) return info;

    QString fmt = QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Video, index, L"Format"));
    if(fmt.compare("AVC",  Qt::CaseInsensitive) == 0) fmt = "AVC/H.264";
    else if(fmt.compare("HEVC", Qt::CaseInsensitive) == 0) fmt = "HEVC/H.265";
    info.codec = fmt;

    QString hdrAll;
    for(const wchar_t *k : {L"HDR_Format", L"HDR_Format_Compatibility",
                             L"HDR_Format_Profile", L"HDR_Format_Settings"})
        hdrAll += QString::fromStdWString(mi->Get(MediaInfoLib::Stream_Video, index, k)) + ' ';

    bool hasDV     = hdrAll.contains("Dolby Vision", Qt::CaseInsensitive);
    bool hasHDR10p = hdrAll.contains("2094-40") || hdrAll.contains("HDR10+", Qt::CaseInsensitive);
    bool hasHDR10  = false;
    { QString h = hdrAll; h.replace("HDR10+", "_", Qt::CaseInsensitive);
      hasHDR10 = h.contains("HDR10", Qt::CaseInsensitive) || hdrAll.contains("ST 2086"); }
    bool hasHLG    = hdrAll.contains("HLG", Qt::CaseInsensitive);

    QStringList parts;
    if(hasDV)     parts << "Dolby Vision";
    if(hasHDR10p) parts << "HDR10+";
    if(hasHDR10)  parts << "HDR10";
    if(hasHLG)    parts << "HLG";
    info.hdrFormat = parts.join(" | ");
    return info;
}
