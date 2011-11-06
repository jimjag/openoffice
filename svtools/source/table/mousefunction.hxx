/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef SVTOOLS_MOUSEFUNCTION_HXX
#define SVTOOLS_MOUSEFUNCTION_HXX

#include "svtools/table/tabletypes.hxx"

#include <rtl/ref.hxx>

#include <boost/noncopyable.hpp>

class MouseEvent;

//......................................................................................................................
namespace svt { namespace table
{
//......................................................................................................................

    class ITableControl;

	//==================================================================================================================
	//= FunctionResult
	//==================================================================================================================
    enum FunctionResult
    {
        ActivateFunction,
        ContinueFunction,
        DeactivateFunction,

        SkipFunction
    };

	//==================================================================================================================
	//= IMouseFunction
	//==================================================================================================================
    class IMouseFunction : public ::rtl::IReference, public ::boost::noncopyable
	{
    public:
        virtual FunctionResult  handleMouseMove( ITableControl& i_tableControl, MouseEvent const & i_event ) = 0;
        virtual FunctionResult  handleMouseDown( ITableControl& i_tableControl, MouseEvent const & i_event ) = 0;
        virtual FunctionResult  handleMouseUp( ITableControl& i_tableControl, MouseEvent const & i_event ) = 0;

    protected:
        virtual ~IMouseFunction() { }
	};

	//==================================================================================================================
	//= MouseFunction
	//==================================================================================================================
    class MouseFunction : public IMouseFunction
    {
    public:
        MouseFunction()
            :m_refCount( 0 )
        {
        }
    protected:
        ~MouseFunction()
        {
        }

    public:
	    virtual oslInterlockedCount SAL_CALL acquire();
	    virtual oslInterlockedCount SAL_CALL release();

    private:
        oslInterlockedCount m_refCount;
    };

	//==================================================================================================================
	//= ColumnResize
	//==================================================================================================================
    class ColumnResize : public MouseFunction
    {
    public:
        ColumnResize()
            :m_nResizingColumn( COL_INVALID )
        {
        }

    public:
        // IMouseFunction
        virtual FunctionResult  handleMouseMove( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseDown( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseUp( ITableControl& i_tableControl, MouseEvent const & i_event );

    private:
        ColPos  m_nResizingColumn;
    };

	//==================================================================================================================
	//= RowSelection
	//==================================================================================================================
    class RowSelection : public MouseFunction
    {
    public:
        RowSelection()
            :m_bActive( false )
        {
        }

    public:
        // IMouseFunction
        virtual FunctionResult  handleMouseMove( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseDown( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseUp( ITableControl& i_tableControl, MouseEvent const & i_event );

    private:
        bool    m_bActive;
    };

	//==================================================================================================================
	//= ColumnSortHandler
	//==================================================================================================================
    class ColumnSortHandler : public MouseFunction
    {
    public:
        ColumnSortHandler()
            :m_nActiveColumn( COL_INVALID )
        {
        }

    public:
        // IMouseFunction
        virtual FunctionResult  handleMouseMove( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseDown( ITableControl& i_tableControl, MouseEvent const & i_event );
        virtual FunctionResult  handleMouseUp( ITableControl& i_tableControl, MouseEvent const & i_event );

    private:
        ColPos  m_nActiveColumn;
    };

//......................................................................................................................
} } // namespace svt::table
//......................................................................................................................

#endif // SVTOOLS_MOUSEFUNCTION_HXX
