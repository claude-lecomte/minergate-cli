#include <QCoreApplication>
#include <QCommandLineParser>

#include <cudaminer/cudamanager.h>
#include <cudaminer/cudaminer.h>

#include "gputester.h"

#include <stdio.h>

using namespace std;
using namespace MinerGate;

int main(int argc, char *argv[]) {
//  qInstallMessageHandler(GPUTester::messageHandler);
  QCoreApplication a(argc, argv);
  QCoreApplication::setApplicationVersion(QString("%1.%2").arg(TESTER_MAJOR_VERSION).arg(TESTER_MINOR_VERSION));

  QCommandLineParser parser;
  parser.setApplicationDescription("MinerGate GPU tester");
  parser.addVersionOption();
  parser.addPositionalArgument("type", "CUDA or OpenCL");
  parser.addPositionalArgument("deviceNo", "order number of device (start with 0)");
  parser.addPositionalArgument("family", "1 - Cryptonotes, 3 - Ethereum");
  parser.addPositionalArgument("first", "first GPU param"); //TODO: create descr
  parser.addPositionalArgument("second", "second GPU param");

  parser.process(a);

  qputenv("GPU_SINGLE_ALLOC_PERCENT", "98");

  const QStringList args = parser.positionalArguments();
  if (args.count() != 5) {
    parser.showHelp(1);
    return 1;
  }

  bool ok;
  QString sType = args.at(0);
  if (sType != "CUDA" && sType != "OpenCL") {
    parser.showHelp(1);
    return 1;
  }
  ImplType implType = sType=="CUDA"?ImplType::CUDA:ImplType::OpenCL;

  int deviceNo = args.at(1).toInt(&ok);
  if (!ok || deviceNo < 0) {
    parser.showHelp(1);
    return 1;
  }

  int iFamily = args.at(2).toInt(&ok);
  if (!ok || (iFamily != 1 && iFamily != 3)) {
    parser.showHelp(1);
    return 1;
  }
  HashFamily family = static_cast<HashFamily>(iFamily);

  int firstParam = args.at(3).toInt(&ok);
  if (!ok) {
    parser.showHelp(1);
    return 1;
  }

  int secondParam = args.at(4).toInt(&ok);
  if (!ok) {
    parser.showHelp(1);
    return 1;
  }

  {
    GPUTester tester(implType, family, deviceNo, firstParam, secondParam);
    quint32 result;
    QString error;
    if (tester.run(result, error))
      fputs(QString::number(result).toLocal8Bit().constData(), stdout);
    else
      fputs(error.toLocal8Bit().constData(), stdout);
  }
  return 0;
}
