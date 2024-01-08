#include "filereader.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

FileReader::FileReader(QObject *parent) : QObject(parent)
{

}

bool FileReader::readFile(const QString &filename, QString &source)
{
    source = QString();
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << qPrintable(tr("Unable to open file: %1").arg(filename));
        return false;
    }

    QTextStream in(&file);
    source = in.readAll();
    return true;
}
