/*

Hanghish
Copyright (C) 2015 Daniele Rogora

This file is part of Hangish.

Hangish is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hangish is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with hangish.  If not, see <http://www.gnu.org/licenses/>

*/

#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <QObject>
#include "qtimports.h"
#include "oauth2auth.h"

class ImageHandler : public QObject
{
    Q_OBJECT

private:
    QNetworkAccessManager *nam;
    QByteArray buffer;
    QAtomicInt lock;
    OAuth2Auth *auth;

public:
    explicit ImageHandler(QObject *parent = 0);
    Q_INVOKABLE QString getImageAddr(QString imgUrl);
    Q_INVOKABLE bool saveImageToGallery(QString imgUrl);
    void cleanCache();
    void setAuthenticator(OAuth2Auth *pAuth);

signals:
    void imgReady(QString path);
    void savedToGallery();

public slots:

private slots:
    void gotImage();
    void dataAvail();

};

#endif // IMAGEHANDLER_H
