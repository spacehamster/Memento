////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 Ripose
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

#ifndef TERMWIDGET_H
#define TERMWIDGET_H

#include <QWidget>

#include <QMutex>

#include "../../../anki/ankiclient.h"
#include "../../../dict/expression.h"
#include "../common/flowlayout.h"

struct AudioSource;
class QMenu;

namespace Ui
{
    class TermWidget;
}

/**
 * Widget used for displaying Term information.
 */
class TermWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor for TermWidget.
     * @param term        The term to display. Does not take ownership.
     * @param sources     A list of audio sources.
     * @param jsonSources True if there are json audio sources.
     * @param list        True if GlossaryWidget should display newlines as a
     *                    bulletted list, false otherwise.
     * @param parent      The parent of this widget.
     */
    TermWidget(
        std::shared_ptr<const Term> term,
        QList<AudioSource> &sources,
        int jsonSources,
        bool list,
        QWidget *parent = nullptr);
    ~TermWidget();

    /**
     * Sets this widget to be addable.
     * @param value If true, Anki add button is visible. If false Anki search
     *              button is visible.
     */
    void setAddable(bool value);

Q_SIGNALS:
    /**
     * Emitted when a kanji is searched by the user and found.
     * @param kanji The found kanji.
     */
    void kanjiSearched(std::shared_ptr<const Kanji> kanji);

    /**
     * Emitted when all audio sources are finished loading.
     */
    void audioSourcesLoaded() const;

private Q_SLOTS:
    /**
     * Adds the term belonging to this widget to Anki.
     */
    void addNote();

    /**
     * Adds the term belonging to this widget to Anki.
     * @param src The audio source to add.
     */
    void addNote(const AudioSource &src);

    /**
     * Opens an Anki window searching the current configured term card deck for
     * the expression of the current term.
     */
    void searchAnki();

    /**
     * Plays the audio from the first available source.
     */
    void playAudio();

    /**
     * Plays audio for the term from the audio source.
     * @param src The audio source to play.
     */
    void playAudio(const AudioSource &src);

    /**
     * Opens up a context menu with every playable audio source.
     * @param pos The position (relative to the audio button) to open the
     *            context menu.
     */
    void showPlayableAudioSources(const QPoint &pos);

    /**
     * Opens up a context menu with every addable audio source.
     * @param pos The position (relative to the add button) to open the
     *            context menu.
     */
    void showAddableAudioSources(const QPoint &pos);

    /**
     * Searches a clicked kanji to see if there is an entry.
     * @param ch The kanji to search.
     */
    void searchKanji(const QString &ch);

private:
    /**
     * Puts term information into the UI.
     * @param term The term to populate the UI with.
     * @param list Whether GlossaryWidget displays newlines in an ordered list.
     */
    void initUi(const Term &term, const bool list);

    /**
     * Creates a Jisho link from an expression.
     * @param exp The expression to create a Jisho link for.
     * @return An HTML formatted Jisho link for the expression.
     */
    inline QString generateJishoLink(const QString &exp);

    /**
     * Loads all remote audio sources.
     */
    void loadAudioSources();

    /**
     * Populates an audio source menu that calls the handler with the specific
     * audio source.
     * @param menu    The menu to populate.
     * @param handler The handler to call.
     */
    void populateAudioSourceMenu(
        QMenu *menu,
        std::function<void(const AudioSource &)> handler);

    /**
     * Returns the first valid file-type audio source.
     * @return A pointer to the first file-type audio source. nullptr if all are
     *         invalid;
     */
    AudioSource *getFirstAudioSource();

    /* UI object containing all the widgets. */
    Ui::TermWidget *m_ui;

    /* The term this widget is displaying. */
    std::shared_ptr<const Term> m_term;

    /* Saved pointer to the global AnkiClient. */
    AnkiClient *m_client;

    /* The list of current audio sources */
    QList<AudioSource> m_sources;

    /* Protects concurrent accesses to the m_sources list */
    QMutex m_lockSources;

    /* The number of json audio sources that haven't been parsed */
    int m_jsonSources;

    /* Lock JSON sources */
    QMutex m_lockJsonSources;

    /* Layout used for holding term tags. */
    FlowLayout *m_layoutTermTags;

    /* Layout used for holding frequency tags. */
    FlowLayout *m_layoutFreqTags;

    /* Layout used hold pitch tags and pitches. */
    QVBoxLayout *m_layoutPitches;

    /* Layout used for holding glossary entries. */
    QVBoxLayout *m_layoutGlossary;

    /* The context menu for the add button */
    QMenu *m_menuAdd;

    /* The context menu for the sound button */
    QMenu *m_menuAudio;
};

#endif // TERMWIDGET_H