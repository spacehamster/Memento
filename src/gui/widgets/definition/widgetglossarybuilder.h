#ifndef WIDGETGLOSSARYBUILDER_H
#define WIDGETGLOSSARYBUILDER_H

#include <QString>
#include <QPair>
#include <QList>
#include <QJsonArray>

#include "util/constants.h"

class WidgetGlossaryBuilder
{
public:
    static void buildGlossary(
        const QJsonArray &definitions,
        QString basepath,
        Constants::GlossaryStyle style,
        QString& glossary,
        QStringList& compactGlossary);

private:

    Constants::GlossaryStyle m_style;

    QStringList m_compactGlossary;

    QString m_basepath;

    QString m_out;

    WidgetGlossaryBuilder(
        Constants::GlossaryStyle style,
        QString basepath
    ) : m_style(style), m_basepath(basepath) {}

    void addDefinitions(const QJsonArray &definitions);

    /**
     * @brief Adds structured style objects.
     * @param      obj The structured style object.
     */
    void addStructuredStyle(const QJsonObject &obj);
    
    /**
     * Adds string structured content.
     * @param      str The string to add.
     */
    void addStructuredContentHelper(const QString &str);

    /**
     * Adds an array of structured content.
     * @param      arr      The array of structured content.
     */
    void addStructuredContentHelper(const QJsonArray &arr);

    /**
     * Adds an object of structured content.
     * @param      obj      The object of structured content.
     */
    void addStructuredContentHelper(const QJsonObject &obj);

    /**
     * Parses and outputs structured content to HTML.
     * The root of the structured content add parser.
     * @param      val      The JSON value of the structured content.
     */
    void addStructuredContent(const QJsonValue &val);

    /**
     * Displays an image type object.
     * @param      obj      The image object.
     */
    void addImage(const QJsonObject &obj);

    /**
     * Adds a text object to the HTML document.
     * @param      obj The text object.
     */
    void addText(const QJsonObject &obj);

    void addCompactGlossary(const QJsonValue &val);
};

#endif // WIDGETGLOSSARYBUILDER_H
