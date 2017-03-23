#!/bin/bash
install_name_tool -change /builds/Qt-5.6.0-shared/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore.framework/Versions/5/QtCore $1
install_name_tool -change /builds/Qt-5.6.0-shared/lib/QtNetwork.framework/Versions/5/QtNetwork @rpath/QtNetwork.framework/Versions/5/QtNetwork $1
install_name_tool -change /builds/Qt-5.6.0-shared/lib/QtWebSockets.framework/Versions/5/QtWidgets @rpath/QtWidgets.framework/Versions/5/QtWidgets $1
install_name_tool -change /builds/Qt-5.6.0-shared/lib/QtSql.framework/Versions/5/QtGui @rpath/QtGui.framework/Versions/5/QtGui $1
