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



#ifndef SVTOOLS_INC_TABLE_DEFAULTINPUTHANDLER_HXX
#define SVTOOLS_INC_TABLE_DEFAULTINPUTHANDLER_HXX

#include "svtools/table/tableinputhandler.hxx"
#include "svtools/table/tabletypes.hxx"

#include <boost/scoped_ptr.hpp>

//......................................................................................................................
namespace svt { namespace table
{
//......................................................................................................................

    struct DefaultInputHandler_Impl;

	//==================================================================================================================
	//= DefaultInputHandler
	//==================================================================================================================
    class DefaultInputHandler : public ITableInputHandler
	{
    private:
        ::boost::scoped_ptr< DefaultInputHandler_Impl > m_pImpl;

    public:
        DefaultInputHandler();
        ~DefaultInputHandler();

        virtual bool    MouseMove       ( ITableControl& _rControl, const MouseEvent& rMEvt );
        virtual bool    MouseButtonDown ( ITableControl& _rControl, const MouseEvent& rMEvt );
        virtual bool    MouseButtonUp   ( ITableControl& _rControl, const MouseEvent& rMEvt );
        virtual bool    KeyInput        ( ITableControl& _rControl, const KeyEvent& rKEvt );
        virtual bool    GetFocus        ( ITableControl& _rControl );
        virtual bool    LoseFocus       ( ITableControl& _rControl );
        virtual bool    RequestHelp     ( ITableControl& _rControl, const HelpEvent& rHEvt );
        virtual bool    Command         ( ITableControl& _rControl, const CommandEvent& rCEvt );
        virtual bool    PreNotify       ( ITableControl& _rControl, NotifyEvent& rNEvt );
        virtual bool    Notify          ( ITableControl& _rControl, NotifyEvent& rNEvt );
	};

//......................................................................................................................
} } // namespace svt::table
//......................................................................................................................

#endif // SVTOOLS_INC_TABLE_DEFAULTINPUTHANDLER_HXX
