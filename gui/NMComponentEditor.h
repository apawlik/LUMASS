/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef NMCOMPONENTEDITOR_H
#define NMCOMPONENTEDITOR_H

#include <string>
#include <iostream>

#include <QWidget>
#include <QVariant>
#include <QtTreePropertyBrowser>
#include <QtGroupBoxPropertyBrowser>

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"

#include "nmlog.h"
#include "NMProcess.h"

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

class NMComponentEditor : public QWidget
{
    Q_OBJECT
    Q_ENUMS(NMCompEditorMode)

public:

    enum NMCompEditorMode {
        NM_COMPEDITOR_GRPBOX,
        NM_COMPEDITOR_TREE
    };

    explicit NMComponentEditor(QWidget *parent = 0,
                               NMCompEditorMode mode = NM_COMPEDITOR_TREE);

    void setObject(QObject* obj);

#ifdef BUILD_RASSUPPORT
    void setRasdamanConnectorWrapper(NMRasdamanConnectorWrapper* wrap)
        {this->mRasConn = wrap;}
#endif

signals:
    void finishedEditing(QObject* obj);

public slots:
    void update(void);
    void clear(void)
        {if(mPropBrowser) mPropBrowser->clear();}

private slots:
    void applySettings(QtProperty* prop, const QVariant& val);

private:
    void updateSubComponents(const QStringList& compList);
    QVariant nestedListFromStringList(const QStringList& strList);

    void createPropertyEdit(const QMetaProperty& property,
            QObject* obj);
    void setComponentProperty(const QtProperty* prop,
            QObject* obj);
    void closeEvent(QCloseEvent* event);
    void readComponentProperties(QObject* obj, NMModelComponent* comp,
            NMProcess* proc);


    QObject* mObj;
    NMModelComponent* comp;
    NMProcess* proc;
    static const std::string ctx;
    NMCompEditorMode mEditorMode;
    QtAbstractPropertyBrowser* mPropBrowser;


#ifdef BUILD_RASSUPPORT
    NMRasdamanConnectorWrapper* mRasConn;
#endif

};

#endif // NMCOMPONENTEDITOR_H