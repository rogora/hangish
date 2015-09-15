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

#include "imagehandler.h"
#include <utime.h>

QString galleryPath;
QString cachePath;

/*
 * Some general notes:
 * the method getImageAddr may be called by different threads at the same time; since buffer and Q* are shared, this would be a problem.
 * Thus I implemented a basic locking mechanism that makes it executable by 1 thread at a time.
 * The signal emitted at the end will trigger the update for the ther images, that will then the processed serially
 *
 * Google puts the jpg extension for all the images, whether they are JPEGs or not;
 * this is not digested by Sailfish (both the native Jolla gallery and the QImage object), that looks at the suffix, and, if it doesn't find a JPEG it complains
 * The workaround for this is to simply remove the suffix .jpg for all the images
 */

ImageHandler::ImageHandler(QObject *parent) :
    QObject(parent)
{
    lock = 0;
    galleryPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/";
    cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/";
    nam = new QNetworkAccessManager();
}

void ImageHandler::setAuthenticator(OAuth2Auth *pAuth)
{
    auth = pAuth;
}

QString ImageHandler::getImageAddr(QString imgUrl)
{
    QString imgname = imgUrl;
    imgname.replace('/', '_');
    if (imgname.endsWith(".jpg"))
        imgname = imgname.left(imgname.size() - 4);
    QString path = QString(cachePath + imgname);
    QFile img(path);
    if (img.exists()) {
        qDebug() << "Img exists " << imgname;
        //Modify the lastModified field; this is necessary because otherwise the lastRead field is not updated
        //Hope this is ok for Jolla QA
        utime(QString(path).toStdString().c_str(), 0);
        return path;
    }
    else {
        if (lock.load()==1) {
            qDebug() << "Another download in progress, returning url";
            return imgUrl;
        }

        lock.store(1);
        buffer.clear();
        qDebug() << "Ok, let's download this";
        QUrl url(imgUrl);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(auth->getCookies()));
        QNetworkReply *reply = nam->get(req);
        //qDebug() << "Getting " << req.url().toString();
        QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(dataAvail()));
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(gotImage()));
        return "";
    }
    return "";
}

void ImageHandler::dataAvail()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    //qDebug() << "DA " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    buffer.append(reply->readAll());
}

void ImageHandler::gotImage()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    //qDebug() << "GI " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    buffer.append(reply->readAll());

    QString imgpath = QString(cachePath + reply->url().toString().replace('/', '_'));
    if (imgpath.endsWith(".jpg"))
        imgpath = imgpath.left(imgpath.size() - 4);
    QFile img(imgpath);
    img.open(QIODevice::WriteOnly);
    img.write(buffer);
    img.close();
    //qDebug() << "Created ";
    emit imgReady(reply->url().toString());
    lock.store(0);
}

void delay()
{
    QTime dieTime= QTime::currentTime().addSecs(1);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

bool ImageHandler::saveImageToGallery(QString imgUrl)
{
    //qDebug() << "Saving";
    QString savedAddr;

    do {
        savedAddr = getImageAddr(imgUrl);
        delay();
    } while (savedAddr=="" || savedAddr.startsWith("https://"));

    QString fileName = imgUrl.right(imgUrl.size() - imgUrl.lastIndexOf('/')-1);
    //qDebug() << fileName;
    if (fileName.endsWith(".jpg"))
        fileName = fileName.left(fileName.size() - 4);

    QFile::copy(savedAddr, galleryPath + fileName);
    emit savedToGallery();
}

void ImageHandler::cleanCache()
{
    qDebug() << "Cleaning images cache";
    //Delete all the files that have not been accessed in the last 7 days
    QDateTime now = QDateTime::currentDateTime();

    QDir dir(cachePath);
    dir.setNameFilters(QStringList() << "*.*");
    dir.setFilter(QDir::Files);
    QFileInfo fileInfo;

    foreach(QString dirFile, dir.entryList())
    {
        //qDebug() << dirFile;
        fileInfo.setFile(dir.absolutePath() + '/' + dirFile);
        fileInfo.refresh();
        QDateTime created = fileInfo.lastRead();
        //qDebug() << created.toString();
        if (created.addDays(7) < now)
            dir.remove(dirFile);
    }
}
