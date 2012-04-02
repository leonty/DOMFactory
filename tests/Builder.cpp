#include <ctime>
#include <iostream>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

#include "DomFactory.h"

int main(int argc, char *argv[])
{
//	QCoreApplication a(argc, argv);

	QFile file(argv[1]);
	if (!file.open(QIODevice::ReadOnly)) {
		std::cout << "Error while opening " << file.fileName().toAscii().data() << std::endl;
		return 1;
	}
	QByteArray data = file.readAll();
	std::clock_t clock1;
	std::clock_t clock2;

/*
	DomFactory::Builder builder(data);

	for (int i = 0; i < 10000; ++i) {
		QDomDocument document;
		int cost = builder.build("/root/operators/operator", document, i);
		if (cost < 0) {
			std::cout << builder.lastErrorMessage().toLocal8Bit().data() << std::endl;
			break;
		} else {
			std::cout << document.documentElement().attribute("id", "zzz").toLocal8Bit().data() << std::endl;
		}
	}
*/

	DomFactory::Factory factory;
	factory.addData(file.fileName(), data);
	QDomDocument document;
/*
	bool result = factory.build(file.fileName(), "/a/c", document, 1);
	std::cout << document.toString().toLocal8Bit().data() << std::endl;
*/
	std::cout << QT_TR_NOOP("Starting sequential read test...") << std::endl;
	clock1 = std::clock();
	int i;

	for (i = 0; i < 100000; ++i) {
		QDomDocument document;
		bool result = factory.build(file.fileName(), argv[2], document, i);
		if (!result) {
			std::cout << "STOP: " << factory.lastErrorMessage(file.fileName()).toLocal8Bit().data() << std::endl;
			break;
		} else {
			QString s = document.documentElement().attribute("id", "zzz");
//			std::cout << document.documentElement().attribute("id", "zzz").toLocal8Bit().data() << " ";
		}
	}
	std::cout << std::endl;

	clock2 = std::clock();
	std::cout << "Parsed " << i << " nodes" << std::endl << "Time: " << (clock2-clock1)/1000.f << std::endl;
//    return a.exec();
}
