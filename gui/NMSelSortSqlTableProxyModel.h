/******************************************************************************
* Created by Alexander Herzig
* Copyright 2015 Landcare Research New Zealand Ltd
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
/*
* NMSelSortSqlTableProxyModel.h
*
*  Created on: 29/09/2015
*      Author: alex
*/

#ifndef NMSELSORTSQLTABLEPROXYMODEL_H_
#define NMSELSORTSQLTABLEPROXYMODEL_H_

#include <qabstractproxymodel.h>

#include "NMSqlTableModel.h"

#include <QObject>
#include <QSqlTableModel>
#include <QItemSelection>
#include <QDateTime>

#include <sqlite3.h>
#include <spatialite.h>


class NMLogger;

class NMSelSortSqlTableProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    NMSelSortSqlTableProxyModel(QObject *parent=0);
    ~NMSelSortSqlTableProxyModel();

    /*
     * Re-implemented methods
     *
     */

    void setSourceModel(QAbstractItemModel *sourceModel);

    QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;
    QModelIndex parent(const QModelIndex& idx) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation,
            int role) const;
    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex mapFromSource(const QModelIndex& srcIdx) const;
    QModelIndex mapToSource(const QModelIndex& proxyIdx) const;

    /*! inserts a column into the data base table
     *  note: the column parameter denotes the type of the column
     *  coded as QVariant::Type
     */

    bool insertColumn(const QString& name, const QVariant::Type& type);
    bool removeColumn(const QString& name);
    int updateData(int colidx, const QString& column, const QString &expr, QString& error);
    bool addRow();
    bool addRows(unsigned int nrows);

    bool createColumnIndex(int colidx);
    void sort(int column, Qt::SortOrder order);
    void reloadSourceModel(void);


    /*
     *  extended NM-API
     */


    bool selectRows(const QString& queryString, bool showSelRecsOnly);
    void clearSelection(void);
    int getNumTableRecords(void);
    long getSelCount(void) {return mLastSelCount;}

    bool joinTable(const QString& joinTableName, const QString& joinFieldName,
                   const QString& tarFieldName);

    QString getRandomString(int len=15);
    QString getFilter(void){return mLastFilter;}
    Qt::SortOrder getSortOrder(){return mLastColSort.second;}
    int getSortColumn(){return mLastColSort.first;}
    QString getSourcePK(void);
    int getMinPKVal(void){return mMinPKVal;}
    QAbstractItemModel* sourceModel(void) const {return mSourceModel;}
    QItemSelection getProxySelection(void);
    QItemSelection getSourceSelection(void);
    QItemSelection getSelectAll(void);

    /*! removes any temporary tables used for ID mapping of sorted tables */
    void removeTempTables(void);

    /*! closes the current db connection and re-opens it in read/write model */
    bool openWriteModel();
    /*! closes the current db connection and re-opens it in readonly model */
    void openReadModel();

protected:


    bool updateSelection(QItemSelection& sel, bool bProxySelection=true);
    void resetSourceModel();
    bool createMappingTable();
    void updateSourceModel(const QString& table);

    QString mDbFileName;
    QString mTableName;

    std::pair<int, Qt::SortOrder> mLastColSort;
    QString mLastFilter;
    QString mTempTableName;
    long mLastSelCount;

    bool mUpdateProxySelection;
    bool mUpdateSourceSelection;

    QItemSelection mProxySelection;
    QItemSelection mSourceSelection;
    NMSqlTableModel* mSourceModel;
    //QSqlDatabase mTempDb;
    QString mSourcePK;
    QString mProxyPK;

    bool mLastSelRecsOnly;
    bool mPKIsFstCol;

    int mMinPKVal;

    NMLogger* mLogger;


private:
    static const std::string ctx;


};

#endif /* NMSELSORTSQLTABLEPROXYMODEL_H_ */
