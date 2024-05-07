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

#include "pitchwidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "gui/widgets/common/flowlayout.h"
#include "tagwidget.h"

#define LH_STYLE    (QString(\
                        "QLabel {"\
	                        "border-style: solid;"\
	                        "border-top-width: 1px;"\
                            "border-bottom-width: 1px;"\
                            "border-right-width: 1px;"\
                            "border-top-color: rgba(0, 0, 0, 0);"\
	                        "border-bottom-color: %1;"\
                            "border-right-color: %1;"\
                        "}"\
                    ))
#define HL_STYLE    (QString(\
                        "QLabel {"\
	                        "border-style: solid;"\
	                        "border-top-width: 1px;"\
                            "border-bottom-width: 1px;"\
                            "border-right-width: 1px;"\
                            "border-bottom-color: rgba(0, 0, 0, 0);"\
	                        "border-top-color: %1;"\
                            "border-right-color: %1;"\
                        "}"\
                    ))
#define H_STYLE     (QString(\
                        "QLabel {"\
	                        "border-style: solid;"\
	                        "border-top-width: 1px;"\
                            "border-bottom-width: 1px;"\
                            "border-bottom-color: rgba(0, 0, 0, 0);"\
	                        "border-top-color: %1;"\
                        "}"\
                    ))
#define L_STYLE     (QString(\
                        "QLabel {"\
	                        "border-style: solid;"\
	                        "border-top-width: 1px;"\
                            "border-bottom-width: 1px;"\
                            "border-top-color: rgba(0, 0, 0, 0);"\
	                        "border-bottom-color: %1;"\
                        "}"\
                    ))

PitchWidget::PitchWidget(
    const Pitch &pitch, 
    Constants::GlossaryStyle glossaryStyle,
    QWidget *parent
) : QWidget(parent)
{
    if (pitch.mora.isEmpty())
    {
        qDebug() << "Reading of empty size from dictionary" << pitch.dictionary;
        return;
    }

    const bool usingVBox = pitch.position.size() > 1 && 
                           glossaryStyle != Constants::GlossaryStyle::Pipe;

    QLayout *layoutParent;
    if (usingVBox)
    {
        layoutParent = new QVBoxLayout(this);
    }
    else
    {
        layoutParent = new FlowLayout(this, 0, 0);
    }
    layoutParent->setContentsMargins(0, 0, 0, 0);

    layoutParent->addWidget(new TagWidget(pitch));

    if(!usingVBox)
    {
        QSpacerItem * spacer = new QSpacerItem(
            6, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        layoutParent->addItem(spacer);
    }

    QString color = palette().color(foregroundRole()).name();
    for (qsizetype i = 0; i < pitch.position.size(); i++)
    {
        const uint8_t pos = pitch.position[i];
        QLayout *layoutLine = nullptr;
        if(usingVBox)
        {
            layoutLine = new QHBoxLayout;
            layoutLine->setContentsMargins(0, 0, 0, 0);
            layoutLine->setSpacing(0);
            static_cast<QVBoxLayout*>(layoutParent)->addLayout(layoutLine);
        }
        else
        {
            layoutLine = layoutParent;
        }

        if(!usingVBox && i > 0)
        {
            QLabel* delimiter = new QLabel(" | ");
            layoutParent->addWidget(delimiter);
        }

        switch (pos)
        {
        case 0:
        {
            QString text  = pitch.mora.first();
            QString style = LH_STYLE.arg(color);
            layoutLine->addWidget(createLabel(text, style));

            if (pitch.mora.size() > 1)
            {
                text = pitch.mora.join("");
                text.remove(0, pitch.mora.first().size());
                style = H_STYLE.arg(color);
                layoutLine->addWidget(createLabel(text, style));
            }
            break;
        }
        case 1:
        {
            QString text  = pitch.mora.first();
            QString style = HL_STYLE.arg(color);
            layoutLine->addWidget(createLabel(text, style));

            if (pitch.mora.size() > 1)
            {
                text = pitch.mora.join("");
                text.remove(0, pitch.mora.first().size());
                style = L_STYLE.arg(color);
                layoutLine->addWidget(createLabel(text, style));
            }
            break;
        }
        default:
        {
            QString text  = pitch.mora.first();
            QString style = LH_STYLE.arg(color);
            layoutLine->addWidget(createLabel(text, style));

            text.clear();
            for (size_t j = 1; j < pos; ++j)
            {
                text += pitch.mora[j];
            }
            if (!text.isEmpty())
            {
                style = HL_STYLE.arg(color);
                layoutLine->addWidget(createLabel(text, style));
            }

            text.clear();
            for (int j = (int)pos; j < pitch.mora.size(); ++j)
            {
                text += pitch.mora[j];
            }
            if (!text.isEmpty())
            {
                style = L_STYLE.arg(color);
                layoutLine->addWidget(createLabel(text, style));
            }
        }
        }

        QLabel *labelNumber = new QLabel;
        labelNumber->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        labelNumber->setText(" [" +QString::number(pos) + "]");
        layoutLine->addWidget(labelNumber);

        if(usingVBox)
        {
            static_cast<QHBoxLayout*>(layoutLine)->addStretch();
        }
    }
}

QLabel *PitchWidget::createLabel(const QString &text,
                                 const QString &style) const
{
    QLabel *label = new QLabel;
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->setStyleSheet(style);
    label->setText(text);
    return label;
}
