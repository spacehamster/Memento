////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2021 Ripose
//
// This file is part of Memento.
//
// Memento is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2 of the License.
//
// Memento is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Memento.  If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "subtitleparser.h"

#include <limits>
#include <QDebug>
#include <QFile>
#include <QSet>

/**
 * Information about an SRT subtitle.
 */
struct SRTInfo : public SubtitleInfo
{
    /* The position of this subtitle. */
    QString position;

    SRTInfo &operator=(const SRTInfo &rhs)
    {
        SubtitleInfo::operator=(rhs);
        position = rhs.position;

        return *this;
    }
} typedef SRTInfo;

SubtitleParser::SubtitleParser()
    : m_timecodeSplitter(QRegularExpression("[:\\.,]")),
      m_assFilter(QRegularExpression("{\\\\.*?}")),
      m_assNewLineReplacer(QRegularExpression("\\\\n|\\\\N")),
      m_srtFilter(
          QRegularExpression(
              "<\\/?[biu]>|" // Removes <b></b> <i></i> <u></u>
              "{\\/?[biu]}|" // Removes {b}{/b} {i}{/i} {u}{/u}
              "{\\\\a\\d}"   // Removes {\a#}
          )
      ),
      m_angleBraceCleaner(QRegularExpression("<[^>]*>"))
{
    m_vttSections << "NOTE" << "STYLE" << "REGION";
}

QList<SubtitleInfo> SubtitleParser::parseSubtitles(
    const QString &path,
    const bool compress) const
{
    QList<SubtitleInfo> subtitles;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return subtitles;
    }

    QString lowerPath = path.toLower();
    if (lowerPath.endsWith(".ass"))
    {
        if (!parseASS(file, subtitles))
        {
            return QList<SubtitleInfo>();
        }
    }
    else if (lowerPath.endsWith(".srt"))
    {
        if (!parseSRT(file, subtitles))
        {
            return QList<SubtitleInfo>();
        }
    }
    else if (lowerPath.endsWith(".vtt"))
    {
        if (!parseVTT(file, subtitles))
        {
            return QList<SubtitleInfo>();
        }
    }

    if (compress)
    {
        return compressSubtitles(subtitles);
    }
    return subtitles;
}

#define ASS_HEADER      "[Script Info]"
#define STYLE_HEADER    "[V4+ Styles]"
#define EVENT_HEADER    "[Events]"

#define FORMAT_PREFIX   "Format: "
#define STYLE_PREFIX    "Style: "
#define DIALOGUE_PREFIX "Dialogue: "

#define START_FORMAT    "Start"
#define END_FORMAT      "End"
#define TEXT_FORMAT     "Text"

bool SubtitleParser::parseASS(QFile &file, QList<SubtitleInfo> &out) const
{
    /* Make sure the file isn't empty */
    if (file.atEnd())
    {
        qDebug() << "ASS Parser: Empty file";
        return false;
    }

    /* Check for the header */
    QString currentLine = file.readLine();
    if (currentLine.trimmed() != ASS_HEADER)
    {
        qDebug() << "ASS Parser: Missing ASS header";
        qDebug() << currentLine;
        return false;
    }

    /* Skip to the [Events] section */
    while (!file.atEnd())
    {
        currentLine = file.readLine();
        if (currentLine.trimmed() == EVENT_HEADER)
        {
            break;
        }
    }

    /* There is no [Events] section. This is odd but valid. */
    if (file.atEnd())
    {
        return true;
    }

    /* Get format section */
    currentLine = file.readLine();
    if (!currentLine.startsWith(FORMAT_PREFIX))
    {
        qDebug() << "ASS Parser: Missing Format line in the [Events] section";
        qDebug() << currentLine;
        return false;
    }
    int startIndex = -1;
    int endIndex = -1;
    int textIndex = -1;
    QStringList format =
        currentLine
            .right(currentLine.size() - sizeof(FORMAT_PREFIX) + 1)
            .split(',');
    for (int i = 0; i < format.size(); ++i)
    {
        if (format[i].trimmed() == START_FORMAT)
        {
            if (startIndex != -1)
            {
                qDebug() << "ASS Parser: Start format redefinition";
                return false;
            }
            startIndex = i;
        }
        else if (format[i].trimmed() == END_FORMAT)
        {
            if (endIndex != -1)
            {
                qDebug() << "ASS Parser: End format redefinition";
                return false;
            }
            endIndex = i;
        }
        else if (format[i].trimmed() == TEXT_FORMAT)
        {
            if (textIndex != -1)
            {
                qDebug() << "ASS Parser: Text format redefinition";
                return false;
            }
            textIndex = i;
        }
    }

    /* Get dialogue */
    while (!file.atEnd())
    {
        currentLine = file.readLine();

        /* End of the [Events] section */
        if (currentLine.trimmed().isEmpty())
        {
            break;
        }
        /* Skip non-dialogue lines */
        else if (!currentLine.startsWith(DIALOGUE_PREFIX))
        {
            continue;
        }

        /* Split dialogue line */
        QStringList dialogue =
            currentLine
                .right(currentLine.size() - sizeof(DIALOGUE_PREFIX) + 1)
                .split(',');
        if (dialogue.size() < format.size())
        {
            qDebug() << "ASS Parser: Dialogue-Format mismatch";
            return false;
        }

        /* Construct the SubtitleInfo */
        SubtitleInfo info;

        /* Get timings */
        bool ok = false;
        info.start = timecodeToDouble(dialogue[startIndex], &ok);
        if (!ok || info.start < 0)
        {
            qDebug() << "ASS Parser: Invalid start time";
            qDebug() << dialogue[startIndex];
            return false;
        }
        info.end = timecodeToDouble(dialogue[startIndex], &ok);
        if (!ok || info.end < info.start)
        {
            qDebug() << "ASS Parser: Invalid end time";
            qDebug() << dialogue[endIndex];
            return false;
        }

        /* Get Text */
        info.text = dialogue[textIndex];
        for (int i = textIndex + 1; i < dialogue.size(); ++i)
        {
            info.text += ',';
            info.text += dialogue[textIndex];
        }
        info.text.remove(m_assFilter);
        info.text.replace(m_assNewLineReplacer, "\n");
        info.text.chop(1);

        /* Throw out empty subtitles */
        if (info.text.trimmed().isEmpty())
        {
            continue;
        }

        /* Add the subtitle */
        out << info;
    }

    std::sort(out.begin(), out.end(),
        [] (const SubtitleInfo &lhs, const SubtitleInfo &rhs)
        {
            return lhs.start < rhs.start;
        }
    );
    return true;
}

#undef ASS_HEADER
#undef STYLE_HEADER
#undef EVENT_HEADER

#undef FORMAT_PREFIX
#undef STYLE_PREFIX
#undef DIALOGUE_PREFIX

#undef START_FORMAT
#undef END_FORMAT
#undef TEXT_FORMAT

#define TIMING_START_INDEX 0
#define TIMING_ARROW_INDEX 1
#define TIMING_END_INDEX 2

#define TIMING_ARROW "-->"

bool SubtitleParser::parseSRT(QFile &file, QList<SubtitleInfo> &out) const
{
    QList<SRTInfo> subs;
    while (!file.atEnd())
    {
        SRTInfo info;

        /* Get the position */
        bool ok = false;
        QString currentLine = file.readLine();
        info.position = currentLine.trimmed().toUInt(&ok);
        if (!ok || info.position < 0)
        {
            qDebug() << "SRT Parser: Invalid position";
            qDebug() << currentLine;
            return false;
        }

        /* Get the timings */
        if (file.atEnd())
        {
            qDebug() << "SRT Parser: Unexpected file end after position";
            return false;
        }
        currentLine = file.readLine();
        QStringList timing = currentLine.trimmed().split(' ');
        if (timing.size() != 3)
        {
            qDebug() << "SRT Parser: Invalid timing";
            qDebug() << currentLine;
            return false;
        }
        if (timing[TIMING_ARROW_INDEX] != TIMING_ARROW)
        {
            qDebug() << "SRT Parser: Missing timing arrow";
            qDebug() << currentLine;
            return false;
        }
        info.start = timecodeToDouble(timing[TIMING_START_INDEX], &ok);
        if (!ok || info.start < 0.0)
        {
            qDebug() << "SRT Parser: Invalid start time";
            qDebug() << timing[TIMING_START_INDEX];
            return false;
        }
        info.end = timecodeToDouble(timing[TIMING_END_INDEX], &ok);
        if (!ok || info.end < info.start)
        {
            qDebug() << "SRT Parser: Invalid end time";
            qDebug() << timing[TIMING_END_INDEX];
            return false;
        }

        /* Get the lines */
        if (file.atEnd())
        {
            qDebug() << "SRT Parser: Unexpected file end after timings";
            return false;
        }
        while (!file.atEnd() && (currentLine = file.readLine()) != '\n')
        {
            info.text += currentLine;
        }
        info.text.chop(1);

        /* Filter out SRT formatting */
        info.text.remove(m_srtFilter);

        /* Don't add if the subtitle is only whitespace */
        if (info.text.trimmed().isEmpty())
        {
            continue;
        }

        /* Append to the output list */
        subs << info;
    }

    std::sort(subs.begin(), subs.end(),
        [] (const SRTInfo &lhs, const SRTInfo &rhs)
        {
            return lhs.start < rhs.start ||
                   (lhs.start == rhs.start &&
                   lhs.position < rhs.position);
        }
    );
    for (const SRTInfo &info : subs)
    {
        out << info;
    }
    return true;
}

#undef TIMING_START_INDEX
#undef TIMING_ARROW_INDEX
#undef TIMING_END_INDEX

#undef TIMING_ARROW

#define TIMING_START_INDEX 0
#define TIMING_ARROW_INDEX 1
#define TIMING_END_INDEX 2

#define VTT_HEADER      "WEBVTT"
#define TIMING_ARROW    "-->"

bool SubtitleParser::parseVTT(QFile &file, QList<SubtitleInfo> &out) const
{
    /* Exit if the file is empty */
    if (file.atEnd())
    {
        qDebug() << "VTT Parser: Unexpected empty file";
        return false;
    }
    /* Exit if the file is missing the header */
    else if (!file.readLine().startsWith(VTT_HEADER))
    {
        qDebug() << "VTT Parser: Missing VTT header";
        return false;
    }

    /* Skip past header info */
    while (!file.atEnd())
    {
        if (file.readLine().trimmed().isEmpty())
        {
            break;
        }
    }

    /* Read subtitles */
    while (!file.atEnd())
    {
        QString currentLine = file.readLine().trimmed();
        /* Skip empty lines */
        if (currentLine.isEmpty())
        {
            continue;
        }
        /* Skip non-subtitle sections */
        else if (m_vttSections.contains(currentLine.split(' ')[0]))
        {
            while (!file.atEnd())
            {
                currentLine = file.readLine();
                if (currentLine.trimmed().isEmpty())
                {
                    break;
                }
            }
            continue;
        }

        SubtitleInfo info;

        /* Get timings */
        QStringList timings = currentLine.split(' ');
        if (timings.size() < 3 || timings[TIMING_ARROW_INDEX] != TIMING_ARROW)
        {
            if (file.atEnd())
            {
                qDebug() << "VTT Parser: Unexpected file end after cue";
                return false;
            }
            currentLine = file.readLine();
            timings = currentLine.split(' ');
            if (timings.size() < 3 || timings[TIMING_ARROW_INDEX] != TIMING_ARROW)
            {
                qDebug() << "VTT Parser: Invalid timing line";
                qDebug() << currentLine;
                return false;
            }
        }
        bool ok = false;
        info.start = timecodeToDouble(timings[TIMING_START_INDEX], &ok);
        if (!ok || info.start < 0.0)
        {
            qDebug() << "VTT Parser: Invalid start time";
            qDebug() << timings[TIMING_START_INDEX];
            return false;
        }
        info.end = timecodeToDouble(timings[TIMING_END_INDEX], &ok);
        if (!ok || info.end < info.start)
        {
            qDebug() << "VTT Parser: Invalid end time";
            qDebug() << timings[TIMING_END_INDEX];
            return false;
        }

        /* Get the lines */
        if (file.atEnd())
        {
            qDebug() << "VTT Parser: Unexpected file end after timings";
            return false;
        }
        while (!file.atEnd() && (currentLine = file.readLine()) != '\n')
        {
            info.text += currentLine;
        }
        info.text.chop(1);

        /* Filter out VTT angle brace formatting */
        info.text.remove(m_angleBraceCleaner);

        /* Don't add if the subtitle is only whitespace */
        if (info.text.trimmed().isEmpty())
        {
            continue;
        }

        /* Append to the output list */
        out << info;
    }

    std::sort(out.begin(), out.end(),
        [] (const SubtitleInfo &lhs, const SubtitleInfo &rhs)
        {
            return lhs.start < rhs.start;
        }
    );
    return true;
}

#undef TIMING_START_INDEX
#undef TIMING_ARROW_INDEX
#undef TIMING_END_INDEX

#undef VTT_HEADER
#undef TIMING_ARROW

#define SECONDS_IN_HOUR         3600
#define SECONDS_IN_MINUTE       60
#define SECONDS_IN_MILLISECOND  0.001
#define SECONDS_IN_HUNDREDTH    0.01

double SubtitleParser::timecodeToDouble(const QString &timecode, bool *ok) const
{
    double timeDouble = 0.0;
    int tmp;

    QStringList pieces = timecode.trimmed().split(m_timecodeSplitter);
    std::reverse(pieces.begin(), pieces.end());
    if (pieces.length() != 3 && pieces.length() != 4)
    {
        goto error;
    }

    /* Get sub-second values */
    bool localOk;
    if (pieces[0].size() == 2)
    {
        tmp = pieces[0].toInt(&localOk);
        if (!localOk || tmp < 0 || tmp > 99)
        {
            goto error;
        }
        timeDouble += tmp * SECONDS_IN_HUNDREDTH;
    }
    else if (pieces[0].size() == 3)
    {
        tmp = pieces[0].toInt(&localOk);
        if (!localOk || tmp < 0 || tmp > 999)
        {
            goto error;
        }
        timeDouble += tmp * SECONDS_IN_MILLISECOND;
    }
    else
    {
        goto error;
    }

    /* Get Seconds */
    tmp = pieces[1].toInt(&localOk);
    if (!localOk || tmp < 0 || tmp > 59)
    {
        goto error;
    }
    timeDouble += tmp;

    /* Get Minutes */
    tmp = pieces[2].toInt(&localOk);
    if (!localOk || tmp < 0 || tmp > 59)
    {
        goto error;
    }
    timeDouble += tmp * SECONDS_IN_MINUTE;

    /* Get Hours */
    if (pieces.length() == 4)
    {
        tmp = pieces[3].toInt(&localOk);
        if (!localOk || tmp < 0)
        {
            goto error;
        }
        timeDouble += tmp * SECONDS_IN_HOUR;
    }

    if (ok)
    {
        *ok = true;
    }
    return timeDouble;

error:
    if (ok)
    {
        *ok = false;
    }
    return 0.0;
}

#undef SECONDS_IN_HOUR
#undef SECONDS_IN_MINUTE
#undef SECONDS_IN_MILLISECOND
#undef SECONDS_IN_HUNDREDTH

QList<SubtitleInfo> SubtitleParser::compressSubtitles(
    const QList<SubtitleInfo> &subtitles) const
{
    QList<SubtitleInfo> compressed;

    QList<const SubtitleInfo *> currentSubInfos;
    double earliestEnd;
    double latestEnd;
    for (const SubtitleInfo &info : subtitles)
    {
        /* Add all the subtitles that ended before this one started */
        while (!currentSubInfos.isEmpty() && earliestEnd <= info.start)
        {
            /* Add all the current subtitles */
            QString text;
            for (const SubtitleInfo *curr : currentSubInfos)
            {
                text += curr->text + '\n';
            }
            text.chop(1);
            compressed << SubtitleInfo{
                .text = text,
                .start = currentSubInfos.first()->start,
                .end = latestEnd
            };

            /* Remove elements that already ended and find the new earliest
             * end.
             */
            double tmp = std::numeric_limits<double>::infinity();
            for (int i = 0; i < currentSubInfos.size(); ++i) // This should be int because -1 is valid
            {
                if (currentSubInfos[i]->end == earliestEnd)
                {
                    currentSubInfos.removeAt(i--);
                }
                else
                {
                    if (currentSubInfos[i]->end < tmp)
                    {
                        tmp = currentSubInfos[i]->end;
                    }
                    if (currentSubInfos[i]->end > latestEnd)
                    {
                        latestEnd = currentSubInfos[i]->end;
                    }
                }
            }
            if (tmp != std::numeric_limits<double>::infinity())
            {
                earliestEnd = tmp;
            }
        }

        if (currentSubInfos.isEmpty())
        {
            earliestEnd = info.end;
            latestEnd = info.end;
        }
        currentSubInfos << &info;
    }

    while (!currentSubInfos.isEmpty())
    {
        /* Add all the current subtitles */
        QString text;
        for (const SubtitleInfo *curr : currentSubInfos)
        {
            text += curr->text + '\n';
        }
        text.chop(1);
        compressed << SubtitleInfo{
            .text = text,
            .start = currentSubInfos.first()->start,
            .end = latestEnd
        };

        /* Remove elements that already ended and find the new earliest end */
        double tmp = std::numeric_limits<double>::infinity();
        for (int i = 0; i < currentSubInfos.size(); ++i) // This should be int because -1 is valid
        {
            if (currentSubInfos[i]->end == earliestEnd)
            {
                currentSubInfos.removeAt(i--);
            }
            else if (currentSubInfos[i]->end < tmp)
            {
                tmp = currentSubInfos[i]->end;
            }
        }
        earliestEnd = tmp;
    }

    return compressed;
}