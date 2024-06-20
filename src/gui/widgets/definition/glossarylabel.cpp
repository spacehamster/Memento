////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022 Ripose
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

#include "glossarylabel.h"

#include <QAbstractTextDocumentLayout>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>
#include <QThreadPool>

#include "dict/dictionary.h"
#include "util/constants.h"
#include "util/globalmediator.h"
#include "util/utils.h"

/* The maximum allowed width of images. This is chosen with consideration of
 * DefinitionWidget's default 500px width. */
#define MAX_WIDTH 350

/* Begin Constructor/Destructor */

GlossaryLabel::GlossaryLabel(
    Qt::KeyboardModifier modifier,
    Constants::GlossaryStyle style,
    QWidget *parent)
    : QTextEdit(parent),
      m_searchModifier(modifier),
      m_style(style)
{
    setTextInteractionFlags(
        Qt::TextSelectableByKeyboard |
        Qt::TextSelectableByMouse |
        Qt::LinksAccessibleByKeyboard |
        Qt::LinksAccessibleByMouse
    );
    setFrameStyle(QFrame::NoFrame);
    setStyleSheet("QTextEdit { background: rgba(0, 0, 0, 0); }");
    setReadOnly(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrapMode(QTextOption::WrapMode::WordWrap);
    setAcceptRichText(true);

    document()->setDefaultStyleSheet(
        "img { vertical-align: bottom; }"
        "table, th, td {"
            "border: 1px solid;"
            "border-collapse: collapse;"
        "}"
        "th, td { padding: 5px; }"
    );

    connect(
        this, &GlossaryLabel::textChanged,
        this, &GlossaryLabel::adjustSize
    );
    connect(
        verticalScrollBar(), &QScrollBar::valueChanged,
        this, [=] (int value) {
            if (value != 0)
            {
                verticalScrollBar()->setValue(0);
            }
        }
    );
}

GlossaryLabel::~GlossaryLabel()
{

}

/* End Constructor/Destructor */
/* Begin Public Methods */

void GlossaryLabel::deselectText()
{
    QTextCursor q = textCursor();
    q.clearSelection();
    setTextCursor(q);
}

/* End Other Object Parsers */
/* Begin Event Handlers */

void GlossaryLabel::adjustSize()
{
    setFixedHeight(document()->size().toSize().height());
}

void GlossaryLabel::handleSearch(
    SharedTermList terms,
    SharedKanji kanji,
    int position,
    int length)
{
    if (position != m_currentIndex)
    {
        return;
    }

    Q_EMIT contentSearched(terms, kanji);

    QTextCursor q = textCursor();
    q.setPosition(position);
    q.setPosition(position + length, QTextCursor::KeepAnchor);
    setTextCursor(q);
}

QSize GlossaryLabel::sizeHint() const
{
    QSize s = QTextEdit::sizeHint();
    s.setHeight(document()->size().toSize().height());
    return s;
}

void GlossaryLabel::showEvent(QShowEvent *event)
{
    QTextEdit::showEvent(event);
    adjustSize();
}

void GlossaryLabel::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);
    adjustSize();
}

/* Prevents large searches from being executed and freezing everything up */
#define MAX_SEARCH_SIZE 40

void GlossaryLabel::mouseMoveEvent(QMouseEvent *event)
{
    QTextEdit::mouseMoveEvent(event);

    if (!(QGuiApplication::keyboardModifiers() & m_searchModifier))
    {
        return;
    }

    int position =
        document()->documentLayout()->hitTest(event->pos(), Qt::ExactHit);
    if (position == -1 || position == m_currentIndex)
    {
        return;
    }
    m_currentIndex = position;

    QString text = toPlainText();
    QRegularExpression delim("[\\n。\\.]");

    int end = text.indexOf(delim, position);
    if (end != -1)
    {
        end -= text[end] == '\n' ? position : position - 1;
    }
    else
    {
        end = text.size() - 1;
    }
    QString query = text.mid(position, end).left(MAX_SEARCH_SIZE);

    int start = text.lastIndexOf(delim, position);
    start = start == -1 ? 0 : start + 1;
    text = text.mid(start, end + position - start);

    if (query.isEmpty() || text.isEmpty())
    {
        return;
    }

    int index = position - start;
    DictionaryWorker *worker = new DictionaryWorker(
        query, text, index, position
    );
    connect(
        worker, &DictionaryWorker::searchDone,
        this, &GlossaryLabel::handleSearch
    );
    QThreadPool::globalInstance()->start(worker);
}

#undef MAX_SEARCH_SIZE

void GlossaryLabel::mousePressEvent(QMouseEvent *event)
{
    QTextEdit::mousePressEvent(event);
    event->ignore();
}

/* End Event Handlers */
/* Begin Worker Implementation */

void DictionaryWorker::run()
{
    Dictionary *dict = GlobalMediator::getGlobalMediator()->getDictionary();
    SharedTermList terms = dict->searchTerms(query, sentence, index, &index);

    if (terms == nullptr)
    {
        /* noop */
    }
    else if (terms->isEmpty())
    {
        terms = nullptr;
    }

    SharedKanji kanji(nullptr);
    if (CharacterUtils::isKanji(query[0]))
    {
        kanji = SharedKanji(dict->searchKanji(query[0]));
        if (kanji)
        {
            kanji->sentence = sentence;
            kanji->clozePrefix = sentence.left(index);
            kanji->clozeBody = kanji->character;
            kanji->clozeSuffix = sentence.mid(index + 1);
        }
    }

    int length = 0;
    if (terms)
    {
        length = terms->first()->clozeBody.size();
    }
    else if (kanji)
    {
        length = 1;
    }
    Q_EMIT searchDone(terms, kanji, position, length);
}

/* End Worker Implementation */
