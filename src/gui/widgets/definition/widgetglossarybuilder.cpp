#include "widgetglossarybuilder.h"

#include <QJsonArray>
#include <QJsonObject>



/* The maximum allowed width of images. This is chosen with consideration of
 * DefinitionWidget's default 500px width. */
#define MAX_WIDTH 350

#define KEY_TYPE                        "type"
#define KEY_CONTENT                     "content"
#define VALUE_TYPE_IMAGE                "image"
#define VALUE_TYPE_STRUCTURED_CONTENT   "structured-content"
#define VALUE_TYPE_TEXT                 "text"

void WidgetGlossaryBuilder::buildGlossary(const QJsonArray &definitions,
        QString basepath,
        Constants::GlossaryStyle style,
        QString& glossary,
        QStringList& compactGlossary)
{
#if defined(Q_OS_WIN)
    basepath.prepend('/');
    basepath.replace('\\', '/');
#endif
    basepath.prepend("file://");
    basepath += '/';
    
    WidgetGlossaryBuilder builder(style, basepath);
    builder.addDefinitions(definitions);
    glossary = builder.m_out;
    compactGlossary = builder.m_compactGlossary;
}
void WidgetGlossaryBuilder::addDefinitions(const QJsonArray &definitions)
{
    if (m_style == Constants::GlossaryStyle::Bullet)
    {
        m_out += "<ul>";
    }
    for (int i = 0; i < definitions.size(); ++i)
    {
        const QJsonValue &val = definitions[i];

        if (m_style == Constants::GlossaryStyle::Bullet)
        {
            m_out += "<li>";
        }

        switch (val.type())
        {
        case QJsonValue::Type::String:
        {
            if(m_style == Constants::GlossaryStyle::Pipe)
            {
                
                m_compactGlossary.push_back(val.toString());
            }
            else
            {
                m_out += val
                    .toString()
                    .replace(
                        '\n',
                        m_style == Constants::GlossaryStyle::Bullet ?
                            "</li><li>" : "<br>"
                    );
            }
            break;
        }
        case QJsonValue::Type::Object:
        {
            QJsonObject obj = val.toObject();
            if (obj[KEY_TYPE] == VALUE_TYPE_STRUCTURED_CONTENT)
            {
                addStructuredContent(obj[KEY_CONTENT]);
            }
            else if (obj[KEY_TYPE] == VALUE_TYPE_IMAGE)
            {
                addImage(obj);
            }
            else if (obj[KEY_TYPE] == VALUE_TYPE_TEXT)
            {
                addText(obj);
            }
            break;
        }
        default:
            break;
        }

        if (m_style == Constants::GlossaryStyle::Bullet)
        {
            m_out += "</li>";
        }
        else if (m_style == Constants::GlossaryStyle::LineBreak && 
            i < definitions.size() - 1)
        {
            m_out += "<br>";
        }
    }
    if (m_style == Constants::GlossaryStyle::Bullet)
    {
        m_out += "</ul>";
    }
}

#undef KEY_TYPE
#undef KEY_CONTENT
#undef VALUE_TYPE_IMAGE
#undef VALUE_TYPE_STRUCTURED_CONTENT
#undef VALUE_TYPE_TEXT

/* End Public Methods */
/* Begin Structured Content Parsing */

#define KEY_FONT_STYLE      "fontStyle"
#define KEY_FONT_WEIGHT     "fontWeight"
#define KEY_FONT_SIZE       "fontSize"
#define KEY_TEXT_DECORATION "textDecorationLine"
#define KEY_VERTICAL_ALIGN  "verticalAlign"
#define KEY_MARGIN_TOP      "marginTop"
#define KEY_MARGIN_LEFT     "marginLeft"
#define KEY_MARGIN_RIGHT    "marginRight"
#define KEY_MARGIN_BOTTOM   "marginBottom"

void WidgetGlossaryBuilder::addStructuredStyle(
    const QJsonObject &obj)
{
    if (obj[KEY_FONT_STYLE].isString())
    {
        m_out += "font-style: ";
        m_out += obj[KEY_FONT_STYLE].toString("normal");
        m_out += ';';
    }

    if (obj[KEY_FONT_WEIGHT].isString())
    {
        m_out += "font-weight: ";
        m_out += obj[KEY_FONT_WEIGHT].toString("normal");
        m_out += ';';
    }

    if (obj[KEY_FONT_SIZE].isString())
    {
        m_out += "font-size: ";
        m_out += obj[KEY_FONT_SIZE].toString("medium");
        m_out += ';';
    }

    if (obj[KEY_TEXT_DECORATION].isArray())
    {
        m_out += "text-decoration: ";
        for (const QJsonValue &val : obj[KEY_TEXT_DECORATION].toArray())
        {
            m_out += val.toString("none");
            m_out += ' ';
        }
        m_out += ';';
    }
    else if (obj[KEY_TEXT_DECORATION].isString())
    {
        m_out += "text-decoration: ";
        m_out += obj[KEY_TEXT_DECORATION].toString("none");
        m_out += ';';
    }

    if (obj[KEY_VERTICAL_ALIGN].isString())
    {
        m_out += "vertical-align: ";
        m_out += obj[KEY_VERTICAL_ALIGN].toString("baseline");
        m_out += ';';
    }

    if (obj[KEY_MARGIN_TOP].isDouble())
    {
        m_out += "margin-top: ";
        m_out += QString::number((int)obj[KEY_MARGIN_TOP].toDouble(0.0));
        m_out += "px;";
    }

    if (obj[KEY_MARGIN_LEFT].isDouble())
    {
        m_out += "margin-left: ";
        m_out += QString::number((int)obj[KEY_MARGIN_LEFT].toDouble(0.0));
        m_out += "px;";
    }

    if (obj[KEY_MARGIN_RIGHT].isDouble())
    {
        m_out += "margin-right: ";
        m_out += QString::number((int)obj[KEY_MARGIN_RIGHT].toDouble(0.0));
        m_out += "px;";
    }

    if (obj[KEY_MARGIN_BOTTOM].isDouble())
    {
        m_out += "margin-bottom: ";
        m_out += QString::number((int)obj[KEY_MARGIN_BOTTOM].toDouble(0.0));
        m_out += "px;";
    }
}

#undef KEY_FONT_STYLE
#undef KEY_FONT_WEIGHT
#undef KEY_FONT_SIZE
#undef KEY_TEXT_DECORATION
#undef KEY_VERTICAL_ALIGN
#undef KEY_MARGIN_TOP
#undef KEY_MARGIN_LEFT
#undef KEY_MARGIN_RIGHT
#undef KEY_MARGIN_BOTTOM

void WidgetGlossaryBuilder::addStructuredContentHelper(const QString &str)
{
    m_out += QString(str).replace('\n', "<br>");
}

void WidgetGlossaryBuilder::addStructuredContentHelper(const QJsonArray &arr)
{
    for (const QJsonValue &val : arr)
    {
        addStructuredContent(val);
    }
}

#define KEY_TAG         "tag"
#define KEY_CONTENT     "content"
#define KEY_PATH        "path"
#define KEY_WIDTH       "width"
#define KEY_HEIGHT      "height"
#define KEY_UNITS       "sizeUnits"
#define KEY_VERT_ALIGN  "verticalAlign"
#define KEY_COLSPAN     "colSpan"
#define KEY_ROWSPAN     "rowSpan"
#define KEY_STYLE       "style"
#define KEY_DATA        "data"

void WidgetGlossaryBuilder::addStructuredContentHelper(const QJsonObject &obj)
{
    QString tag = obj[KEY_TAG].toString();

    QJsonObject data = obj[KEY_DATA].toObject();
    if(m_style == Constants::GlossaryStyle::Pipe &&
        !data.isEmpty() && 
        data[KEY_CONTENT].toString() == "glossary")
    {
        addCompactGlossary(obj[KEY_CONTENT]);
        return;
    }
    
    if (tag.isEmpty())
    {
        return;
    }
    else if (tag == "br")
    {
        m_out += "<br>";
    }
    else if (tag == "img")
    {
        m_out += "<img src=\"";
        m_out += m_basepath;
        m_out += '/';
        m_out += obj[KEY_PATH].toString();
        m_out += '"';

        if (obj[KEY_UNITS].isNull() || obj[KEY_UNITS].toString() == "px")
        {
            double width = -1;
            double height = -1;
            if (obj[KEY_WIDTH].isDouble())
            {
                width = obj[KEY_WIDTH].toDouble();
            }
            if (obj[KEY_HEIGHT].isDouble())
            {
                height = obj[KEY_HEIGHT].toDouble();
            }
            /*
             * As of right now, this code creates mustard gas because while
             * QTextEdit does inherit its font from stylesheets set in a parent
             * widget, there is no good way to get the font size. That means
             * there is no good way to scale the text according to ems.
             */
            /*
            if (obj[KEY_UNITS].toString() == "em")
            {
                int size = font().pixelSize();
                if (size < 0)
                {
                    size = (int)(font().pointSize() / 0.75);
                }
                width *= size;
                height *= size;
            }
            */
            if (width > MAX_WIDTH)
            {
                height = MAX_WIDTH * height / width;
                width = MAX_WIDTH;
            }
            if (width > 0)
            {
                m_out += " width=\"";
                m_out += QString::number((int)width);
                m_out += '"';
            }
            if (height > 0)
            {
                m_out += " height=\"";
                m_out += QString::number((int)height);
                m_out += '"';
            }
        }
        if (obj[KEY_VERT_ALIGN].isString())
        {
            m_out += " style=\"vertical-align: ";
            m_out += obj[KEY_VERT_ALIGN].toString();
            m_out += ";\"";
        }

        m_out += '>';
    }
    else if (tag == "span" || tag == "div")
    {
        m_out += '<';
        m_out += tag;
        if (obj[KEY_STYLE].isObject())
        {
            m_out += " style=\"";
            addStructuredStyle(obj[KEY_STYLE].toObject());
            m_out += '"';
        }
        m_out += '>';

        addStructuredContent(obj[KEY_CONTENT]);

        m_out += "</" + tag + '>';
    }
    else if (tag == "td" || tag == "th")
    {
        m_out += '<';
        m_out += tag;
        if (obj[KEY_COLSPAN].isDouble())
        {
            m_out += " colspan=\"";
            m_out += QString::number((int)obj[KEY_COLSPAN].toDouble());
            m_out += '"';
        }
        if (obj[KEY_ROWSPAN].isDouble())
        {
            m_out += " rowspan=\"";
            m_out += QString::number((int)obj[KEY_ROWSPAN].toDouble());
            m_out += '"';
        }
        if (obj[KEY_STYLE].isObject())
        {
            m_out += " style=\"";
            addStructuredStyle(obj[KEY_STYLE].toObject());
            m_out += '"';
        }
        m_out += '>';

        addStructuredContent(obj[KEY_CONTENT]);

        m_out += "</" + tag + '>';
    }
    else
    {
        m_out += '<' + tag + '>';
        addStructuredContent(obj[KEY_CONTENT]);
        m_out += "</" + tag + '>';
    }
}

#undef KEY_TAG
#undef KEY_CONTENT
#undef KEY_PATH
#undef KEY_WIDTH
#undef KEY_HEIGHT
#undef KEY_UNITS
#undef KEY_COLSPAN
#undef KEY_ROWSPAN
#undef KEY_STYLE

void WidgetGlossaryBuilder::addStructuredContent(const QJsonValue &val)
{
    switch (val.type())
    {
    case QJsonValue::Type::String:
        addStructuredContentHelper(val.toString());
        break;
    case QJsonValue::Type::Array:
        addStructuredContentHelper(val.toArray());
        break;
    case QJsonValue::Type::Object:
        addStructuredContentHelper(val.toObject());
        break;
    default:
        break;
    }
}

/* End Structured Content Parsing */
/* Begin Other Object Parsers */

#define KEY_IMAGE       "image"
#define KEY_PATH        "path"
#define KEY_WIDTH       "width"
#define KEY_HEIGHT      "height"
#define KEY_DESCRIPTION "description"

void WidgetGlossaryBuilder::addImage(const QJsonObject &obj)
{
    m_out += "<img src=\"";
    m_out += m_basepath;
    m_out += '/';
    m_out += obj[KEY_PATH].toString();
    m_out += '"';

    int width = MAX_WIDTH;
    int height = -1;
    if (obj[KEY_WIDTH].isDouble())
    {
        width = (int)obj[KEY_WIDTH].toDouble();
    }
    if (obj[KEY_HEIGHT].isDouble())
    {
        height = (int)obj[KEY_HEIGHT].toDouble();
    }

    if (width < 0 || width > MAX_WIDTH)
    {
        height = (int)((height * MAX_WIDTH) / ((double)width));
        width = MAX_WIDTH;
    }

    m_out += " width=\"";
    m_out += QString::number(width);
    m_out += '"';
    if (height > 0)
    {
        m_out += " height=\"";
        m_out += QString::number(height);
        m_out += '"';
    }

    m_out += '>';

    if (obj[KEY_DESCRIPTION].isString())
    {
        m_out += "<br>";
        m_out += obj[KEY_DESCRIPTION].toString().replace('\n', "<br>");
    }
}

#undef KEY_IMAGE
#undef KEY_PATH
#undef KEY_WIDTH
#undef KEY_HEIGHT
#undef KEY_DESCRIPTION

#define KEY_TEXT "text"

void WidgetGlossaryBuilder::addText(const QJsonObject &obj)
{
    m_out += obj[KEY_TEXT]
        .toString()
        .replace(
            '\n',
            m_style == Constants::GlossaryStyle::Bullet ? "</li><li>" : "<br>"
        );
}

#undef KEY_TEXT


#define KEY_CONTENT "content"
void WidgetGlossaryBuilder::addCompactGlossary(const QJsonValue &val)
{
    switch (val.type())
    {
    case QJsonValue::Type::String:
        m_compactGlossary.push_back(val.toString());
        break;
    case QJsonValue::Type::Array:
        {
            for(const QJsonValue &entry : val.toArray())
            {
                addCompactGlossary(entry);
            }
                    break;
        }
    case QJsonValue::Type::Object:
        addCompactGlossary(val.toObject()[KEY_CONTENT]);
        break;
    default:
        break;
    }
}