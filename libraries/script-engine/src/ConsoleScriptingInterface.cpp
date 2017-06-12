//
//  ConsoleScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by volansystech on 6/1/17.
//  Copyright 2014 High Fidelity, Inc.
//
//  ConsoleScriptingInterface is responsible to print logs 
//  using "console" object on debug Window and Logs/log file with proper tags.
//  Used in Javascripts file.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConsoleScriptingInterface.h"
#include "ScriptEngine.h"

void ConsoleScriptingInterface::info(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptInfoMessage(message);
    }
}

void ConsoleScriptingInterface::log(QString message) {
    if (_isGroupEnd) {
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            scriptEngine->scriptPrintedMessage(message);
        }
    } else {
        consoleGroupLog(_selectedGroup, message);
    }
}

void ConsoleScriptingInterface::debug(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage(message);
    }
}

void ConsoleScriptingInterface::warn(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptWarningMessage(message);
    }
}

void ConsoleScriptingInterface::error(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptErrorMessage(message);
    }
}

void ConsoleScriptingInterface::exception(QString message) {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->logConsoleException(message);
    }
    
}

bool ConsoleScriptingInterface:: hasCorrectSyntax(QScriptProgram& program) {
    const auto syntaxCheck = QScriptEngine::checkSyntax(program.sourceCode());
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        const auto error = syntaxCheck.errorMessage();
        const auto line = QString::number(syntaxCheck.errorLineNumber());
        const auto column = QString::number(syntaxCheck.errorColumnNumber());
        const auto message = QString("[SyntaxError] %1 in %2:%3(%4)").arg(error, program.fileName(), line, column);
        qCritical() << qPrintable(message);
        return false;
    }
    return true;
}

QString ConsoleScriptingInterface::hadUncaughtExceptions(QScriptEngine& engine, QString& fileName) {
    log("4");
    //if (engine.hasUncaughtException()) {
        const auto backtrace = engine.uncaughtExceptionBacktrace();
        const auto exception = engine.uncaughtException().toString();
        const auto line = QString::number(engine.uncaughtExceptionLineNumber());
        engine.clearExceptions();
        log("5");
        static const QString SCRIPT_EXCEPTION_FORMAT = "[UncaughtException] %1 in %2:%3";
        auto message = QString(SCRIPT_EXCEPTION_FORMAT).arg(exception, fileName, line);
        if (!backtrace.empty()) {
            static const auto lineSeparator = "\n    ";
            message += QString("\n[Backtrace]%1%2").arg(lineSeparator, backtrace.join(lineSeparator));
        }
        // qCritical() << qPrintable(message);
        log("6");
        return message;
    //}
    //return false;
}


void  ConsoleScriptingInterface::time(QString labelName) {
    QDateTime _currentTime = QDateTime::currentDateTime().toUTC();
    _listOfTimeValues.insert(labelName, _currentTime.toUTC());
}

void ConsoleScriptingInterface::timeEnd(QString labelName) {
    QDateTime _dateTimeOfLabel;
    QString message;
    qint64 millisecondsDiff = 0;

    bool _labelInList = _listOfTimeValues.contains(labelName);

    //Check if not exist items in list
    if (!_labelInList) {
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            message = "No such label found " + labelName;
            scriptEngine->scriptErrorMessage(message);
        }
        return;
    }

    QDateTime _currentDateTimeValue = QDateTime::currentDateTime().toUTC();
    _dateTimeOfLabel = _listOfTimeValues.value(labelName);

    if (!_dateTimeOfLabel.isNull()) {
        _listOfTimeValues.remove(labelName);
    }

    if (_dateTimeOfLabel.toString(TIME_FORMAT) == _currentDateTimeValue.toString(TIME_FORMAT)) {
        millisecondsDiff = _dateTimeOfLabel.msecsTo(_currentDateTimeValue);
        message = QString::number(millisecondsDiff) + " ms";
    }
    else {
        message = secondsToString(_dateTimeOfLabel.secsTo(_currentDateTimeValue));
    }

    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->scriptPrintedMessage("Time : " + message);
    }
}

QString ConsoleScriptingInterface::secondsToString(qint64 seconds)
{
    qint64 days = seconds / DAY;
    QTime timeDuration = QTime(0, 0).addSecs(seconds % DAY);

    return QString("%1 days, %2 hours, %3 minutes, %4 seconds")
        .arg(QString::number(days)).arg(QString::number(timeDuration.hour())).arg(QString::number(timeDuration.minute())).arg(QString::number(timeDuration.second()));
}

void  ConsoleScriptingInterface::asserts(bool condition, QString message) {
    if (!condition) {
        QString assertFailed = "Assertion failed";
        if (message.isEmpty()) {
            message = assertFailed;
        }
        else {
            if (typeid(message) != typeid(QString)) {
                message = assertFailed;
            }
            else {
                message = assertFailed + " " + message;
            }
        }
        if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
            scriptEngine->scriptErrorMessage(message);
        }
    }
}

void  ConsoleScriptingInterface::trace() {    
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->logTraceException();
    }
}

void  ConsoleScriptingInterface::clear() {
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->clearConsole();
    }
}

void ConsoleScriptingInterface::group(QString groupName) {
    _selectedGroup = groupName;
    consoleGroupLog(GROUP, groupName);
}

void ConsoleScriptingInterface::groupCollapsed(QString  groupName) {
    _selectedGroup = groupName;
    consoleGroupLog(GROUPCOLLAPSED, groupName);
}

void ConsoleScriptingInterface::groupEnd() {
    consoleGroupLog(_selectedGroup, GROUPEND);
}

void ConsoleScriptingInterface::consoleGroupLog(QString currentGroup, QString groupName) {
    QPair<QString, QString> groupData;
    QString groupKeyName;

    groupData = qMakePair(currentGroup, groupName);
    groupKeyName = groupData.first;
    if (groupData.first == GROUP || groupData.first == GROUPCOLLAPSED) {
        groupKeyName = groupData.second;
        if (_isSameLevel) {
            _addSpace = _addSpace.mid(0, _addSpace.length() - 3);
        }
        if (_isGroupEnd) {
            _addSpace = "";
            _isGroupEnd = false;
        } else {
            _addSpace += "   ";
        }                
        debug(_addSpace.mid(0, _addSpace.length() - 1) + groupData.second);
        _isSameLevel = false;
    } else {
        if (groupData.first == groupKeyName && groupData.first != GROUPCOLLAPSED && groupData.first != GROUP && groupData.second != GROUPEND) {
            if (!_isSameLevel) {
                _addSpace += "   ";  //space inner element
                _isSameLevel = true; //same level log entry
            }
            debug(_addSpace + groupData.second);
        }
    }
    if (groupData.second == GROUPEND) {
        _addSpace = _addSpace.mid(0, _addSpace.length() - 3);
        if (_addSpace.length() == 0) {
            _isGroupEnd = true;
        }
    }
}
