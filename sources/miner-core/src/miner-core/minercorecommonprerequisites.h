#pragma once

#include <QSharedPointer>
#include <QMetaType>
#include <QVector>

namespace MinerGate {

class AbstractMinerEngine;
class AbstractMinerPlugin;
class Coin;
class BenchmarkData;
class AbstractMinerImpl;

typedef QSharedPointer<AbstractMinerEngine> AbstractMinerEnginePtr;
typedef QSharedPointer<AbstractMinerPlugin> AbstractMinerPluginPtr;
typedef QSharedPointer<Coin> CoinPtr;
typedef QSharedPointer<BenchmarkData> TestDataPtr;
typedef QSharedPointer<AbstractMinerImpl> MinerImplPtr;

typedef QList<QPair<quint32, QString> > SpeedFactorList;
enum class CpuMinerState : quint8 { Unknown, Stopped, Running, Error, Dead, MergedMining };
#ifndef Q_OS_MAC
enum class GpuState : quint8 { Stopped, Running, Error, NotAvailable, MergedMining };
#endif
enum class Error : quint8 {NoError = 0, UnknownError, InvalidEmailOrPass, TotpRequired, WrongTotp, EmailMiss, PassMiss, NetworkProblem, PasswordsDidntMatch,
                           GPUNotFound, CPUOverload, MissingArgument, InvalidPaymentId, PaymentIdRequired, WithdrawalsDisabledForUser, InvalidAddress,
                           UserAlreadyExists, InvalidEmail, TokenInvalid, NotEnoughMoney, LowDiskSpace, PreparingError, LoginMaxCountExceeded, AmountBelowLimit,
                           LowGpuMemory};

enum class AuthState : quint8 { LoggedOut, LoggedIn, TokenExpired };

const QString FAMILY_CRYPTONIGHT("cryptonight");
const QString FAMILY_CRYPTONIGHT_LITE("cryptonight-lite");
const QString FAMILY_ETHEREUM("ethereum");

}

Q_DECLARE_METATYPE(MinerGate::CpuMinerState)
#ifndef Q_OS_MAC
Q_DECLARE_METATYPE(MinerGate::GpuState)
#endif
Q_DECLARE_METATYPE(MinerGate::Error)

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#if _WIN32
#	if defined( __MINGW32__ )
#		define _MinerCoreCommonExport
#	else
#		if defined MINER_CORE_EXPORT
#			define _MinerCoreCommonExport Q_DECL_EXPORT
#		else
#			define _MinerCoreCommonExport Q_DECL_IMPORT
#		endif
#	endif
#else
#	define _MinerCoreCommonExport
#endif


#if _WIN32
#	if defined( __MINGW32__ )
#		define _MinerPluginCommonExport
#	else
#		define _MinerPluginCommonExport Q_DECL_EXPORT
#	endif
#else
#	define _MinerPluginCommonExport
#endif
