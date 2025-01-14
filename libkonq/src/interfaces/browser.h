/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQINTERFACES_BROWSER_H
#define KONQINTERFACES_BROWSER_H

#include "libkonq_export.h"

#include <QObject>

namespace KonqInterfaces {

class CookieJar;

/**
 * @brief Abstract class representing the Konqueror browser
 */
class LIBKONQ_EXPORT Browser : public QObject
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     *
     * @param parent the parent object
     */
    Browser(QObject* parent = nullptr);
    virtual ~Browser(); ///< Destructor

    /**
     * @brief Sets the object to use for cookies management
     *
     * @note this object *wont't* take ownership of @p jar.
     *
     * @param jar the object to use for cookie management
     */
    virtual void setCookieJar(CookieJar *jar) = 0;

    /**
     * @brief The object to use for cookie management
     * @return the object to use for cookie management
     */
    virtual CookieJar* cookieJar() const = 0;

    /**
     * @brief The standard user agent used by Konqueror
     * @return The standard user agent used by Konqueror
     */
    virtual QString konqUserAgent() const = 0;

    /**
     * @brief The default user agent
     * @return The default user agent
     */
    virtual QString defaultUserAgent() const = 0;

    /**
     * @brief The user agent currently in use
     *
     * This can be either defaultUserAgent() or a temporary user agent set with setTemporaryUserAgent()
     * @return The user agent currently in use
     * @see setTemporaryUserAgent()
     */
    virtual QString userAgent() const = 0;

    /**
     * @brief Sets a temporary user agent to be used instead of the default one
     *
     * The temporary user agent remains in use until the application is closed or this is called again.
     *
     * Implementations should emit the userAgentChanged() signal, unless the new user
     * agent is effectively equal to to old. If switching from the default user agent to a
     * temporary user agent but the two are equal (or vice versa) the signal shouldn't be emitted
     * @param newUA the new user agent string.
     */
    virtual void setTemporaryUserAgent(const QString &newUA) = 0;

    /**
     * @brief Stops using a temporary user agent and returns to the default one
     */
    virtual void clearTemporaryUserAgent() = 0;

    /**
     * @brief Casts the given object or one of its children to a Browser
     *
     * This is similar to
     * @code
     * obj->findChild<Browser*>();
     * @endcode
     * except that if @p obj derives from Browser, it will be returned, regardless of whether any
     * of its children also derive from it.
     * @param obj the object to cast to a Browser
     * @return @p obj or one of its children as a Browser* or `nullptr` if neither @p obj nor its children derive from Browser
     */
    static Browser* browser(QObject* obj);

signals:
    void configurationChanged(); ///< Signal emitted after the configuration has changed

    /**
     * @brief Signal emitted when the user agent changes
     *
     * This signal can be emitted in several situations:
     * - the default user agent is in use and the user changes it
     * - the user switched from the default user agent to a temporary one (or vice versa) and the two are different
     * - the user switched from a temporary user agent to a different temporary user agent
     *
     * @param currentUA the new current user agent
     * @see setTemporaryUserAgent()
     */
    void userAgentChanged(const QString &currentUA);
};

}

#endif // KONQINTERFACES_BROWSER_H
