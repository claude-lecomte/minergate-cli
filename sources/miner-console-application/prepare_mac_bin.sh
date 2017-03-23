#!/bin/bash
install_name_tool -change $2/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore.framework/Versions/5/QtCore $1
install_name_tool -change $2/lib/QtNetwork.framework/Versions/5/QtNetwork @rpath/QtNetwork.framework/Versions/5/QtNetwork $1
install_name_tool -change $2/lib/QtWebSockets.framework/Versions/5/QtWebSockets @rpath/QtWebSockets.framework/Versions/5/QtWebSockets $1
