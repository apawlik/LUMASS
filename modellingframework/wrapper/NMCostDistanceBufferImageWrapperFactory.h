/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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
 * NMCostDistanceBufferImageWrapperFactory.h
 *
 *  Created on: 2019-04-09
 *      Author: Alex Herzig
 */

#ifndef NMCostDistanceBufferImageWrapperFactory_H_
#define NMCostDistanceBufferImageWrapperFactory_H_

#include <QObject>
#include "NMWrapperFactory.h"

#include "nmcostdistancebufferimagewrapper_export.h"

class NMCOSTDISTANCEBUFFERIMAGEWRAPPER_EXPORT NMCostDistanceBufferImageWrapperFactory : public NMWrapperFactory
{
    Q_OBJECT
public:
    NMCostDistanceBufferImageWrapperFactory(QObject *parent = nullptr);

    NMProcess* createWrapper();
    bool isSinkProcess(void) {return true;}
    QString getWrapperClassName() {return "NMCostDistanceBufferImageWrapper";}
    QString getComponentAlias() {return QStringLiteral("CostDistanceBuffer");}
};

#endif // NMCostDistanceBufferImageWrapperFactory_H
