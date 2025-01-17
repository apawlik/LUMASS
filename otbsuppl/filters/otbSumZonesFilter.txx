/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010-2014 Landcare Research New Zealand Ltd
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



#ifndef __otbSumZonesFilter_txx
#define __otbSumZonesFilter_txx

#include "nmlog.h"
#include "otbSumZonesFilter.h"

#include "itkImageScanlineConstIterator.h"
#include "itkImageScanlineIterator.h"
//#include "itkImageRegionIterator.h"
//#include "itkImageRegionConstIterator.h"
//#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkProgressReporter.h"
#include "itkMacro.h"

namespace otb
{

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::SumZonesFilter()
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    m_HaveMaxKeyRows = false;
    m_IgnoreNodataValue = true;
    m_NodataValue = itk::NumericTraits<InputPixelType>::NonpositiveMin();

    m_NextZoneId = 0;
    mStreamingProc = false;
    m_ZoneTableFileName = "";

    mZoneTable = SQLiteTable::New();
}

template< class TInputImage, class TOutputImage >
SumZonesFilter< TInputImage, TOutputImage >
::~SumZonesFilter()
{
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream& os, itk::Indent indent) const
{

}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetValueImage(const InputImageType* image)
{
    if (image->GetNumberOfComponentsPerPixel() > 1)
    {
        itkExceptionMacro(<< "SetValueImage - This process component does not "
                             "accept multi-band images!");
    }

    mValueImage = const_cast<TInputImage*>(image);
    this->SetNthInput(1, const_cast<TInputImage*>(image));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneImage(const OutputImageType* image)
{
    if (image->GetNumberOfComponentsPerPixel() > 1)
    {
        itkExceptionMacro(<< "SetZoneImage - This process component does not "
                             "accept multi-band images!");
    }

    mZoneImage = const_cast<TOutputImage*>(image);
    this->SetNthInput(0, const_cast<TOutputImage*>(image));
    this->GraftOutput(static_cast<TOutputImage*>(mZoneImage));
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetZoneTableFileName(const std::string &tableFileName)
{
    if (tableFileName.empty())
    {
        m_dropTmpDBs = true;
    }
    else
    {
        m_dropTmpDBs = false;
    }

    m_ZoneTableFileName = tableFileName;
    this->ResetPipeline();
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer SumZonesFilter< TInputImage, TOutputImage >
::GetZoneTable(void)
{
    AttributeTable::Pointer tab = mZoneTable.GetPointer();
    return tab;
}

template< class TInputImage, class TOutputImage >
AttributeTable::Pointer SumZonesFilter< TInputImage, TOutputImage >
::getRAT(unsigned int idx)
{
    AttributeTable::Pointer tab = mZoneTable.GetPointer();
    return tab;
}


template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetHaveMaxKeyRows(bool maxkeyrows)
{
    m_HaveMaxKeyRows = maxkeyrows;
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetIgnoreNodataValue(bool ignore)
{
    m_IgnoreNodataValue = ignore;
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::SetNodataValue(InputPixelType nodata)
{
    m_NodataValue = nodata;
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
    typename itk::ProcessObject::NameArray inputNames = this->GetInputNames();
    for (int n=0; n < inputNames.size(); ++n)
    {
        if (inputNames.at(n).find("zone") != std::string::npos)
        {
            mZoneImage = dynamic_cast<OutputImageType*>(this->GetInputs()[n].GetPointer());
        }

        if (inputNames.at(n).find("value") != std::string::npos)
        {
            mValueImage = dynamic_cast<InputImageType*>(this->GetInputs()[n].GetPointer());
        }
    }

    NMDebugCtx(ctx, << "...");
    if (mZoneImage.IsNull())
    {
        itkExceptionMacro(<< "Zone Image has not been specified!");
        return;
    }

    if (mValueImage.IsNull())
    {
        itkDebugMacro(<< "Just summarising the zone imgae itself!");
        NMDebugAI(<< "Just summarising the zone imgae itself..." << std::endl);
    }
    else
    {
        NMDebugAI(<< "Summarise values per zone..." << std::endl);
    }

    NMDebugAI( << "  IgnoreNodataValues = " << m_IgnoreNodataValue << std::endl);
    NMDebugAI( << "  NodataValue        = " << m_NodataValue << std::endl);
    NMDebugAI( << "  HaveMaxKeyRows     = " << m_HaveMaxKeyRows << std::endl);


    // create the zone table (db)
    std::string tempZtName = m_ZoneTableFileName;
    if (!mStreamingProc)
    {
        if (tempZtName.empty())
        {
            if (!m_Workspace.empty())
            {
                tempZtName = m_Workspace + "/" + otb::SQLiteTable::GetRandomString(10) + ".ldb";
            }
            else
            {
                tempZtName = otb::SQLiteTable::GetRandomString(10) + ".ldb";
            }

            // if we're using a temp data base, we want to know the name for
            // to open the same table again during sequential processing
            m_dropTmpDBs = true;
        }

        NMDebugAI( << "Creating the zone table ..." << std::endl);
        if (mZoneTable->CreateTable(tempZtName, "1") == otb::SQLiteTable::ATCREATE_ERROR)
        {
            itkExceptionMacro(<< "Failed to create the zone table: "
                              << mZoneTable->getLastLogMsg());
            return;
        }

        m_ZoneTableFileName = mZoneTable->GetDbFileName();

        NMDebugAI(<< "clearing value stores ..." << std::endl);
        mGlobalValueStore.clear();
        mTotalPixCount = 0;
        mLPRPixCount = mZoneImage->GetLargestPossibleRegion().GetNumberOfPixels();

        NMDebugAI(<< "Adding columns to zone table ..." << std::endl);
        mZoneTable->BeginTransaction();
        mZoneTable->AddColumn("zone_id", AttributeTable::ATTYPE_INT);
        mZoneTable->AddColumn("count", AttributeTable::ATTYPE_INT);
        mZoneTable->AddColumn("min", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->AddColumn("max", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->AddColumn("mean", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->AddColumn("stddev", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->AddColumn("sum", AttributeTable::ATTYPE_DOUBLE);
        mZoneTable->AddColumn("minX", AttributeTable::ATTYPE_INT);
        mZoneTable->AddColumn("minY", AttributeTable::ATTYPE_INT);
        mZoneTable->AddColumn("maxX", AttributeTable::ATTYPE_INT);
        mZoneTable->AddColumn("maxY", AttributeTable::ATTYPE_INT);
        mZoneTable->EndTransaction();


        mStreamingProc = true;
    }
    else // re-open connection
    {
        if (!mZoneTable->openConnection())
        {
            itkExceptionMacro(<< "Failed to open connection to "
                         << mZoneTable->GetDbFileName() << ": "
                         << mZoneTable->getLastLogMsg());
        }

        if (!mZoneTable->PopulateTableAdmin())
        {
            itkExceptionMacro(<< "Failed to read table "
                              << mZoneTable->GetTableName() << ": "
                              << mZoneTable->getLastLogMsg());
        }

    }

    // for now, the size of the input regions must be exactly the same
    if (mValueImage.IsNotNull())
    {
        if (	(	mZoneImage->GetLargestPossibleRegion().GetSize(0)
                 !=  mValueImage->GetLargestPossibleRegion().GetSize(0))
            ||  (   mZoneImage->GetLargestPossibleRegion().GetSize(1)
                 !=  mValueImage->GetLargestPossibleRegion().GetSize(1))
           )
        {
            itkExceptionMacro(<< "ZoneImage and ValueImage dimensions don't match!");
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }

    // prepare thread specific stores, valid for one pass only
    unsigned int numThreads = this->GetNumberOfThreads();
    mThreadPixCount.clear();
    mThreadValueStore.clear();
    for (int t=0; t < numThreads; ++t)
    {
        mThreadValueStore.push_back(ZoneMapType());
        mThreadPixCount.push_back(0);
    }

    NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId)
{
    if (threadId == 0)
    {
        NMDebugCtx(ctx, << "...");
        unsigned int xsize = outputRegionForThread.GetSize(0);
        unsigned int ysize = outputRegionForThread.GetSize(1);
        NMDebugAI(<< "analysing region of " << xsize << " x " << ysize
                << " pixels ..." << std::endl);
        //NMDebugAI(<< "number of threads: " << this->GetNumberOfThreads() << std::endl);
    }
    //typedef itk::ImageRegionConstIteratorWithIndex<TOutputImage> InputIterType;
    using InputIterType = itk::ImageScanlineConstIterator<TOutputImage>;
    InputIterType zoneIt(mZoneImage, outputRegionForThread);
    typename InputIterType::IndexType pixIdx;

    mThreadPixCount[threadId] += outputRegionForThread.GetNumberOfPixels();
    ZoneMapType& vMap = mThreadValueStore[threadId];
    ZoneMapTypeIterator mapIt;

    itk::ProgressReporter progress(this, threadId,
            outputRegionForThread.GetNumberOfPixels());

    if (threadId == 0)
    {
        NMDebugAI(<< "start summarising ..." << std::endl);
    }

    zoneIt.GoToBegin();

    if (mValueImage.IsNotNull())
    {
        //typedef itk::ImageRegionConstIterator<TInputImage> OutputIterType;
        using OutputIterType = itk::ImageScanlineConstIterator<TInputImage>;
        OutputIterType valueIt(mValueImage, outputRegionForThread);

        valueIt.GoToBegin();
        while (!zoneIt.IsAtEnd() && !valueIt.IsAtEnd() && !this->GetAbortGenerateData())
        {
            while( !valueIt.IsAtEndOfLine() )
            {
                const ZoneKeyType zone = static_cast<ZoneKeyType>(zoneIt.Get());
                const double val = static_cast<double>(valueIt.Get());
                if (	m_IgnoreNodataValue
                        &&	val == static_cast<double>(m_NodataValue)
                        )
                {
                    ++zoneIt;
                    ++valueIt;

                    progress.CompletedPixel();
                    continue;
                }


                mapIt = vMap.find(zone);
                if (mapIt == vMap.end())
                {
                    // idx 0, 1, 2. 3, 4 are min, max, sum, count, sum^2
                    vMap[zone] = std::vector<double>(9,0);
                    vMap[zone][0] = itk::NumericTraits<double>::max();
                    vMap[zone][1] = itk::NumericTraits<double>::NonpositiveMin();

                    // idx 5, 6, 7, 8 are minX, minY, maxX, maxY
                    vMap[zone][5] = itk::NumericTraits<double>::max();
                    vMap[zone][6] = itk::NumericTraits<double>::max();
                    vMap[zone][7] = itk::NumericTraits<double>::NonpositiveMin();
                    vMap[zone][8] = itk::NumericTraits<double>::NonpositiveMin();
                }
                std::vector<double>& params = vMap[zone];

                // min
                params[0] = params[0] < val ? params[0] : val;
                // max
                params[1] = params[1] > val ? params[1] : val;
                // sum_val
                params[2] += val;
                // count
                params[3] += 1;
                // sum_val^2
                params[4] += val * val;

                pixIdx = zoneIt.GetIndex();

                params[5] = pixIdx[0] < params[5] ? pixIdx[0] : params[5];
                params[6] = pixIdx[1] < params[6] ? pixIdx[1] : params[6];
                params[7] = pixIdx[0] > params[7] ? pixIdx[0] : params[7];
                params[8] = pixIdx[1] > params[8] ? pixIdx[1] : params[8];

                ++zoneIt;
                ++valueIt;

                progress.CompletedPixel();
            }
            zoneIt.NextLine();
            valueIt.NextLine();
        }
    }
    else
    {
        while (!zoneIt.IsAtEnd() && !this->GetAbortGenerateData())
        {
            while (!zoneIt.IsAtEndOfLine())
            {
                const ZoneKeyType zone = static_cast<ZoneKeyType>(zoneIt.Get());
                double val = static_cast<double>(zone);
                mapIt = vMap.find(zone);
                if (mapIt == vMap.end())
                {
                    vMap[zone] = std::vector<double>(9,0);

                    // idx 5, 6, 7, 8 are minX, minY, maxX, maxY
                    vMap[zone][5] = itk::NumericTraits<double>::max();
                    vMap[zone][6] = itk::NumericTraits<double>::max();
                    vMap[zone][7] = itk::NumericTraits<double>::NonpositiveMin();
                    vMap[zone][8] = itk::NumericTraits<double>::NonpositiveMin();
                }
                std::vector<double>& params = vMap[zone];

                // min
                params[0] = params[0] < val ? params[0] : val;
                // max
                params[1] = params[1] > val ? params[1] : val;
                // sum_val
                params[2] += val;
                // count
                params[3] += 1;
                // sum_val^2
                params[4] += val * val;

                pixIdx = zoneIt.GetIndex();

                params[5] = pixIdx[0] < params[5] ? pixIdx[0] : params[5];
                params[6] = pixIdx[1] < params[6] ? pixIdx[1] : params[6];
                params[7] = pixIdx[0] > params[7] ? pixIdx[0] : params[7];
                params[8] = pixIdx[1] > params[8] ? pixIdx[1] : params[8];


                ++zoneIt;
                progress.CompletedPixel();
            }
            zoneIt.NextLine();
        }
    }

    if (threadId == 0)
    {
        NMDebugAI( << "mStreamingProc = " << mStreamingProc << std::endl);
        NMDebugCtx(ctx, << "done!");
    }
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::AfterThreadedGenerateData()
{
    NMDebugCtx(ctx, << "...");

    if (mZoneTable.IsNull())
    {
        NMProcErr(<< ctx << ": oops, don't have a zone table - shouldn't have happened!");
        itk::ExceptionObject eo;
        eo.SetDescription("no table found! How could that happen?!");
        throw eo;
    }

    // =============================================================
    // MERGING THREAD DATA
    // =============================================================

    // merging the data gathered in different threads into one
    // global map

    NMDebugAI(<< "update set of zones - adding: ");
    ZoneKeyType numzones = mGlobalValueStore.size();
    ZoneKeyType newzones = 0;
    ZoneKeyType maxKey = 0;
    ZoneMapTypeIterator mapIt;
    ZoneMapTypeIterator globalIt;

    /*  keep track of zones ...
     *  0: min
     *  1: max
     *  2: sum_v
     *  3: count
     *  4: sum_v^2
     *  5: minX
     *  6: minY
     *  7: maxX
     *  8: maxY
     */

    for (int t=0; t < this->GetNumberOfThreads(); ++t)
    {
        mTotalPixCount += mThreadPixCount[t];
        ZoneMapType& vMap = mThreadValueStore[t];
        mapIt = vMap.begin();
        while(mapIt != vMap.end())
        {
            ZoneKeyType zone = mapIt->first;
            globalIt = mGlobalValueStore.find(zone);
            if (globalIt == mGlobalValueStore.end())
            {
                mGlobalValueStore[zone] = mapIt->second;

                // keep track of max key and number of new keys
                ++newzones;
            }
            else
            {
                // ..................................
                // update parameters for current zone
                // ..................................

                std::vector<double>&  params = globalIt->second;

                // min
                params[0] = params[0] < mapIt->second[0] ?
                            params[0] : mapIt->second[0];
                // max
                params[1] = params[1] > mapIt->second[1] ?
                            params[1] : mapIt->second[1];
                // sum_val
                params[2] += mapIt->second[2];

                // count
                params[3] += mapIt->second[3];

                // sum_val^2
                params[4] += mapIt->second[4];

                // minX
                params[5] = params[5] < mapIt->second[5] ?
                            params[5] : mapIt->second[5];

                // minY
                params[6] = params[6] < mapIt->second[6] ?
                            params[6] : mapIt->second[6];

                // maxX
                params[7] = params[7] > mapIt->second[7] ?
                            params[7] : mapIt->second[7];

                // maxY
                params[8] = params[8] > mapIt->second[8] ?
                            params[8] : mapIt->second[8];

            }

            ++mapIt;
        }
    }

    // get the maximum key up this point
    globalIt = mGlobalValueStore.end();
    --globalIt;
    maxKey = globalIt->first;

    NMDebug(<< std::endl);
    NMDebugAI(<< "Merged threads ... " << std::endl);
    NMDebugAI(<< "... new zones  = " << newzones << std::endl);
    NMDebugAI(<< "... num zones  = " << numzones+newzones << std::endl);
    NMDebugAI(<< "... maxKey     = " << maxKey << std::endl);
    NMDebugAI(<< "... " << mTotalPixCount << " of "
                        << mLPRPixCount << " pixel processed ..." << std::endl);

    // =============================================================
    // WRITING OUT THE VALUES ONCE ALL PIXELS HAVE BEEN PROCESSED
    // =============================================================
    if (mTotalPixCount == mLPRPixCount)
    {
        // --------------------------------------------------------------------------
        // prepare prepared update/insert statements for summary table
        // --------------------------------------------------------------------------

        std::vector<std::string> colnames;
        colnames.push_back(mZoneTable->GetPrimaryKey()); // 0
        colnames.push_back("zone_id");                   // 1
        colnames.push_back("count");                     // 2
        colnames.push_back("min");                       // 3
        colnames.push_back("max");                       // 4
        colnames.push_back("mean");                      // 5
        colnames.push_back("stddev");                    // 6
        colnames.push_back("sum");                       // 7
        colnames.push_back("minX");                      // 8
        colnames.push_back("minY");                      // 9
        colnames.push_back("maxX");                      // 10
        colnames.push_back("maxY");                      // 11

        int numIntCols =  3;
        std::vector<otb::AttributeTable::ColumnValue> values;
        std::vector<otb::AttributeTable::ColumnValue> fillIns;
        for (int i=0; i < numIntCols; ++i)
        {
            otb::AttributeTable::ColumnValue v;
            v.type = otb::AttributeTable::ATTYPE_INT;
            values.push_back(v);
            v.ival = 0;
            fillIns.push_back(v);
        }
        fillIns[1].ival = -1;

        for (int i=0; i < 5; ++i)
        {
            otb::AttributeTable::ColumnValue v;
            v.type = otb::AttributeTable::ATTYPE_DOUBLE;
            values.push_back(v);
            v.dval = 0;
            fillIns.push_back(v);
        }

        // add the 'extent' columns
        for (int h=0; h < 4; ++h)
        {
            otb::AttributeTable::ColumnValue v;
            v.type = otb::AttributeTable::ATTYPE_INT;
            values.push_back(v);
            v.ival = 0;
            fillIns.push_back(v);
        }

        NMDebugAI(<< "writing zone table ..." << std::endl);
        mZoneTable->BeginTransaction();
        mZoneTable->PrepareBulkSet(colnames, true);
        globalIt = mGlobalValueStore.begin();

        m_NextZoneId = 0;
        while (globalIt != mGlobalValueStore.end() && !this->GetAbortGenerateData())
        {
            if (m_HaveMaxKeyRows)
            {
                while (globalIt->first > m_NextZoneId)
                {
                    fillIns[0].ival = m_NextZoneId;
                    mZoneTable->DoBulkSet(fillIns);
                    ++m_NextZoneId;
                }
            }

            std::vector<double>& p = globalIt->second;
            values[0].ival = globalIt->first;                // rowidx
            values[1].ival = m_NextZoneId;                   // zone id
            values[2].ival = static_cast<long long>(p[3]);   // count
            values[3].dval = p[0];                           // min
            values[4].dval = p[1];                           // max
            values[5].dval = p[2] / (p[3] > 0 ? p[3] : 1.0); // mean
//            values[6].dval = values[5].dval != p[2]
//                             ? ::sqrt( (p[4] / p[3]) - (values[5].dval * values[5].dval))
//                             : 0;                            // stddev
            values[6].dval = ::sqrt((p[4] / (p[3] > 0 ? p[3] : 1.0)) - (values[5].dval * values[5].dval));
            values[7].dval = p[2];                           // sum

            // set the extent
            values[8].ival = p[5];                          // minX
            values[9].ival = p[6];                          // minY
            values[10].ival = p[7];                         // maxX
            values[11].ival = p[8];                         // maxY

            mZoneTable->DoBulkSet(values);
            ++globalIt;
            ++m_NextZoneId;
        }
        mZoneTable->EndTransaction();
    }

    mZoneTable->CloseTable();
    NMDebugAI(<< "Got " << numzones << " zones on record now ..." << std::endl);

    this->GraftOutput(static_cast<TOutputImage*>(mZoneImage));

    NMDebugCtx(ctx, << "done!");
}

template< class TInputImage, class TOutputImage >
void SumZonesFilter< TInputImage, TOutputImage >
::ResetPipeline()
{
    NMDebugCtx(ctx, << "...");
    mStreamingProc = false;

    if (m_dropTmpDBs)
    {
        mZoneTable->CloseTable(true);
    }
    mZoneTable = 0;
    m_NextZoneId = 0;

    mZoneTable = SQLiteTable::New();

    Superclass::ResetPipeline();
    NMDebugCtx(ctx, << "done!");
}

} // end namespace otb

#endif // __otbSumZonesFilter_txx
