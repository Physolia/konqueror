/*
    SPDX-FileCopyrightText: 2009 Jakub Wieczorek <faw217@gmail.com>
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "OpenSearchReader.h"

#include "OpenSearchEngine.h"

#include <QIODevice>

#include <KLocalizedString>

OpenSearchReader::OpenSearchReader()
    : QXmlStreamReader()
{
}

OpenSearchEngine *OpenSearchReader::read(const QByteArray &data)
{
    clear();

    addData(data);

    return read();
}

OpenSearchEngine *OpenSearchReader::read(QIODevice *device)
{
    clear();

    if (!device->isOpen()) {
        device->open(QIODevice::ReadOnly);
    }

    setDevice(device);
    return read();
}

OpenSearchEngine *OpenSearchReader::read()
{
    OpenSearchEngine *engine = new OpenSearchEngine();

    while (!isStartElement() && !atEnd()) {
        readNext();
    }

    if (name() != QLatin1String("OpenSearchDescription")
            || namespaceUri() != QLatin1String("http://a9.com/-/spec/opensearch/1.1/")) {
        raiseError(i18n("The file is not an OpenSearch 1.1 file."));
        return engine;
    }

    while (!(isEndElement() && name() == QLatin1String("OpenSearchDescription")) && !atEnd()) {
        readNext();

        if (!isStartElement()) {
            continue;
        }

        if (name() == QLatin1String("ShortName")) {
            engine->setName(readElementText());
        } else if (name() == QLatin1String("Description")) {
            engine->setDescription(readElementText());
        } else if (name() == QLatin1String("Url")) {
            QString type = attributes().value(QLatin1String("type")).toString();
            QString url = attributes().value(QLatin1String("template")).toString();

            if (url.isEmpty()) {
                continue;
            }

            QList<OpenSearchEngine::Parameter> parameters;

            readNext();

            while (!(isEndElement() && name() == QLatin1String("Url"))) {
                if (!isStartElement() || (name() != QLatin1String("Param") && name() != QLatin1String("Parameter"))) {
                    readNext();
                    continue;
                }

                QString key = attributes().value(QLatin1String("name")).toString();
                QString value = attributes().value(QLatin1String("value")).toString();

                if (!key.isEmpty() && !value.isEmpty()) {
                    parameters.append(OpenSearchEngine::Parameter(key, value));
                }

                while (!isEndElement()) {
                    readNext();
                }
            }

            if (type == QLatin1String("application/x-suggestions+json")) {
                engine->setSuggestionsUrlTemplate(url);
                engine->setSuggestionsParameters(parameters);
            } else {
                engine->setSearchUrlTemplate(url);
                engine->setSearchParameters(parameters);
            }
        } else if (name() == QLatin1String("Image")) {
            engine->setImageUrl(readElementText());
        }

        if (!engine->name().isEmpty()
                && !engine->description().isEmpty()
                && !engine->suggestionsUrlTemplate().isEmpty()
                && !engine->searchUrlTemplate().isEmpty()
                && !engine->imageUrl().isEmpty()) {
            break;
        }
    }

    return engine;
}

