/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMQtOtbAttributeTableModel.cpp
 *
 *  Created on: 25/10/2013
 *      Author: alex
 */

#include "NMQtOtbAttributeTableModel.h"

#include <QColor>

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(QObject* parent)
	: mKey(""), mKeyIdx(-1)
{
	this->setParent(parent);
}

NMQtOtbAttributeTableModel::NMQtOtbAttributeTableModel(
		otb::AttributeTable::Pointer tab, QObject* parent)
	: mKey(""), mKeyIdx(-1)
{
	this->setParent(parent);
	if (tab.IsNotNull())
		this->mTable = tab;
}

NMQtOtbAttributeTableModel::~NMQtOtbAttributeTableModel()
{
}

int
NMQtOtbAttributeTableModel::rowCount(const QModelIndex& parent) const
{
	if (mTable.IsNull())
		return 0;

	return mTable->GetNumRows();
}

int
NMQtOtbAttributeTableModel::columnCount(const QModelIndex& parent) const
{
	if (mTable.IsNull())
		return 0;

	return mTable->GetNumCols();
}

void
NMQtOtbAttributeTableModel::setKeyColumn(const QString& key)
{
	int idx = this->mTable->ColumnExists(key.toStdString());
	if (idx >= 0)
	{
		this->mKey = key;
		this->mKeyIdx = idx;
	}
	else
	{
		this->mKey.clear();
		this->mKeyIdx = -1;
	}
}

QVariant
NMQtOtbAttributeTableModel::data(const QModelIndex& index, int role) const
{
	if ( mTable.IsNull() || !index.isValid() )
	{
		return QVariant();
	}

	// everything we do, depends on the column  type
	otb::AttributeTable::TableColumnType type = mTable->GetColumnType(index.column());
	switch(role)
	{
		case Qt::EditRole:
		case Qt::DisplayRole:
		{
			switch(type)
			{
			case otb::AttributeTable::ATTYPE_STRING:
				return QVariant(mTable->GetStrValue(index.column(), index.row()).c_str());
				break;
			case otb::AttributeTable::ATTYPE_INT:
				return QVariant((qlonglong)mTable->GetIntValue(index.column(), index.row()));
						//QString("%1")
						//.arg(mTable->GetIntValue(index.column(), index.row())));
				break;
			case otb::AttributeTable::ATTYPE_DOUBLE:
				return QVariant(mTable->GetDblValue(index.column(), index.row()));
						//QString("%L1")
						//.arg(mTable->GetDblValue(index.column(), index.row()), 0, 'G', 4));
				break;
			default:
				return QVariant();
				break;
			}
		}
		break;

		case Qt::TextAlignmentRole:
		{
			switch(type)
			{
			case otb::AttributeTable::ATTYPE_INT:
			case otb::AttributeTable::ATTYPE_DOUBLE:
				return QVariant(Qt::AlignRight | Qt::AlignVCenter);
				break;
			default:
				return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
				break;
			}
		}
		break;

		case Qt::ForegroundRole:
			{
				return QVariant(QColor(0,0,0));
			}
			break;



		default:
			break;
	}
	return QVariant();
}

QVariant
NMQtOtbAttributeTableModel::headerData(int section,
		Qt::Orientation orientation,
		int role) const
{
	if (mTable.IsNull() || role != Qt::DisplayRole)
		return QVariant();

	switch(orientation)
	{
	case Qt::Horizontal:
		return QVariant(mTable->GetColumnName(section).c_str());
		break;
	case Qt::Vertical:
		return QVariant(mTable->GetStrValue(mKeyIdx, section).c_str());
	default:
		break;
	}

	return QVariant();
}


bool
NMQtOtbAttributeTableModel::setData(const QModelIndex& index,
		const QVariant& value,
		int role) //=Qt::EditRole)
{
	if (	mTable.IsNull()
		||  role != Qt::EditRole
		||  !index.isValid())
	{
		return false;
	}

	bool bok = true;
	switch(value.type())
	{
	case QVariant::Char:
	case QVariant::Int:
		mTable->SetValue(index.column(), index.row(), (long)value.toInt(&bok));
		break;

	case QVariant::Double:
		mTable->SetValue(index.column(), index.row(), value.toDouble(&bok));
		break;

	case QVariant::Date:
	case QVariant::DateTime:
	case QVariant::Time:
	case QVariant::Url:
	case QVariant::String:
		mTable->SetValue(index.column(), index.row(), value.toString().toStdString());
		break;

	default:
		return false;
		break;
	}

	emit dataChanged(index, index);
	return true;
}


Qt::ItemFlags
NMQtOtbAttributeTableModel::flags(const QModelIndex& index) const
{
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return 0;
}

//QModelIndex
//NMQtOtbAttributeTableModel::index(int row, int column,
//		const QModelIndex& parent) const
//{
//	if (mTable.IsNull())
//		return QModelIndex();
//
//	// we use the internal data member of the parent to wrap the column type
//	// of the index
//	return  createIndex(row, column, mTable->GetColumnType(column));
//}

//bool
//NMQtOtbAttributeTableModel::insertColumn(int column,
//		const QModelIndex& parent)
//{
//	if (mTable.IsNull())
//		return false;
//
//	// since we only insert columns at the (right-hand side)
//	// end of the table, we expect the 'column' to
//	// denote the otbAttributeTable::TableColumnType
//	// of the new column
//
//
//
//}
//bool removeColumn(int column, const QModelIndex& parent=QModelIndex());

// additional public interface to
void
NMQtOtbAttributeTableModel::setTable(otb::AttributeTable::Pointer table)
{
	this->mTable = table;

	emit this->reset();
}



