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

#include "tagwidget.h"

#define STYLE_FORMAT    (QString(\
                            "font-family: \"Noto Sans\", \"Noto Sans CJK JP\", sans-serif;"\
                            "font-weight: bold;"\
                            "padding-left: 5px;"\
                            "padding-right: 5px;"\
                            "color: white;"\
                            "background: %1;"\
                            "border: 2px solid %1;"\
                            "border-radius: 5px;"\
                        ))

/*
#define FREQ_FORMAT     (QString(\
                            "<style>"\
                                ".freq {"\
                                    "font-family: \"Noto Sans\", \"Noto Sans CJK JP\", sans-serif;"\
                                    "padding-left: 5px;"\
                                    "padding-right: 5px;"\
                                    "border-bottom: 2px solid #5cb85c;"\
                                    "border-top: 2px solid #5cb85c;"\
                                "}"\
                                ".freq-tag {"\
                                    "font-weight: bold;"\
                                    "color: white;"\
                                    "background: #5cb85c;"\
                                    "border-left: 2px solid #5cb85c;"\
                                    "border-radius: 5px 0px 0px 5px;"\
                                "}"\
                                ".freq-body {"\
                                    "font-weight: normal;"\
                                    "color: black;"\
                                    "background: white;"\
                                    "border-right: 2px solid #5cb85c;"\
                                    "border-radius: 0px 5px 5px 0px;"\
                                "}"\
                            "</style>"\
                            "<span class=\"freq-tag\">%1</span><span class=\"freq-body\">%2</span>"\
                        )) */

#define FREQ_FORMAT     (QString(\
                            "<style>"\
                                ".freq-body {"\
                                    "color: black;"\
                                "}"\
                            "</style>"\
                            "%1 <span class=\"freq-body\"> %2 </span>"\
                        ))

TagWidget::TagWidget(const Tag &tag, QWidget *parent) : TagWidget(parent)
{
    TagColor color = def;
    if (tag.category == "name")
    {
        color = name;
    }
    else if (tag.category == "expression")
    {
        color = expression;
    }
    else if (tag.category == "popular")
    {
        color = popular;
    }
    else if (tag.category == "frequent")
    {
        color = frequent;
    }
    else if (tag.category == "archaism")
    {
        color = archaism;
    }
    else if (tag.category == "dictionary")
    {
        color = dictionary;
    }
    else if (tag.category == "frequency")
    {
        color = frequency;
    }
    else if (tag.category == "partOfSpeech")
    {
        color = pos;
    }
    else if (tag.category == "search")
    {
        color = search;
    }
    else if (tag.category == "pitch-accent-dictionary")
    {
        color = pitch;
    }
    
    setStyleSheet(STYLE_FORMAT.arg(colors[color]));
    setToolTip(tag.notes);
    setText(tag.name);
}

TagWidget::TagWidget(const TermFrequency &freq, QWidget *parent) : TagWidget(parent)
{
    setStyleSheet(STYLE_FORMAT.arg(colors[frequency]));
    setText(FREQ_FORMAT.arg(freq.dictionary).arg(freq.freq));
    setToolTip(freq.dictionary);
}

TagWidget::TagWidget(const QString &dicName, QWidget *parent) : TagWidget(parent)
{
    setStyleSheet(STYLE_FORMAT.arg(colors[dictionary]));
    setToolTip(dicName);
    setText(dicName);
}

TagWidget::TagWidget(QWidget *parent) : QLabel(parent)
{
    setWordWrap(true);
    setTextFormat(Qt::RichText);
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}