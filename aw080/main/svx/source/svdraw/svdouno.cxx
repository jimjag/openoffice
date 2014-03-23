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



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svx.hxx"
#include <svx/sdr/contact/viewcontactofunocontrol.hxx>
#include <svx/sdr/contact/viewobjectcontactofunocontrol.hxx>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/io/XPersistObject.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XActiveDataSource.hpp>
#include <com/sun/star/io/XObjectOutputStream.hpp>
#include <com/sun/star/io/XObjectInputStream.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/types.hxx>
#include <vcl/pdfextoutdevdata.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdmodel.hxx>
#include "svx/svdglob.hxx"  // Stringcache
#include "svx/svdstr.hrc"   // Objektname
#include <svx/svdetc.hxx>
#include <svx/svdview.hxx>
#include <svx/svdorect.hxx>
#include <rtl/ref.hxx>
#include <set>
#include <memory>
#include <svx/sdrpagewindow.hxx>
#include <svx/sdrpaintwindow.hxx>
#include <tools/diagnose_ex.h>
#include <svx/svdograf.hxx>
#include <svx/svdlegacy.hxx>

using namespace ::com::sun::star;
using namespace ::sdr::contact;

//************************************************************
//   Defines
//************************************************************

//************************************************************
//   Hilfsklasse SdrControlEventListenerImpl
//************************************************************
#include <com/sun/star/lang/XEventListener.hpp>

#include <cppuhelper/implbase1.hxx>

// =============================================================================
class SdrControlEventListenerImpl : public ::cppu::WeakImplHelper1< ::com::sun::star::lang::XEventListener >
{
protected:
	SdrUnoObj*					pObj;

public:
	SdrControlEventListenerImpl(SdrUnoObj* _pObj)
	:	pObj(_pObj)
	{}

	// XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw(::com::sun::star::uno::RuntimeException);

	void StopListening(const uno::Reference< lang::XComponent >& xComp);
	void StartListening(const uno::Reference< lang::XComponent >& xComp);
};

// XEventListener
void SAL_CALL SdrControlEventListenerImpl::disposing( const ::com::sun::star::lang::EventObject& /*Source*/)
	throw(::com::sun::star::uno::RuntimeException)
{
	if (pObj)
	{
		pObj->xUnoControlModel = NULL;
	}
}

void SdrControlEventListenerImpl::StopListening(const uno::Reference< lang::XComponent >& xComp)
{
	if (xComp.is())
		xComp->removeEventListener(this);
}

void SdrControlEventListenerImpl::StartListening(const uno::Reference< lang::XComponent >& xComp)
{
	if (xComp.is())
		xComp->addEventListener(this);
}

// =============================================================================
struct SAL_DLLPRIVATE SdrUnoObjDataHolder
{
    mutable ::rtl::Reference< SdrControlEventListenerImpl >
                                    pEventListener;
};

// =============================================================================
namespace
{
    void lcl_ensureControlVisibility( SdrView* _pView, const SdrUnoObj* _pObject, bool _bVisible )
    {
        OSL_PRECOND( _pObject, "lcl_ensureControlVisibility: no object -> no survival!" );

        SdrPageView* pPageView = _pView ? _pView->GetSdrPageView() : NULL;
        DBG_ASSERT( pPageView, "lcl_ensureControlVisibility: no view found!" );
        if ( !pPageView )
            return;

        ViewContact& rUnoControlContact( _pObject->GetViewContact() );

        for ( sal_uInt32 i = 0; i < pPageView->PageWindowCount(); ++i )
        {
            const SdrPageWindow* pPageWindow = pPageView->GetPageWindow( i );
            DBG_ASSERT( pPageWindow, "lcl_ensureControlVisibility: invalid PageViewWindow!" );
            if ( !pPageWindow )
                continue;
            
            if ( !pPageWindow->HasObjectContact() )
                continue;

            ObjectContact& rPageViewContact( pPageWindow->GetObjectContact() );
            const ViewObjectContact& rViewObjectContact( rUnoControlContact.GetViewObjectContact( rPageViewContact ) );
            const ViewObjectContactOfUnoControl* pUnoControlContact = dynamic_cast< const ViewObjectContactOfUnoControl* >( &rViewObjectContact );
            DBG_ASSERT( pUnoControlContact, "lcl_ensureControlVisibility: wrong ViewObjectContact type!" );
            if ( !pUnoControlContact )
                continue;

            pUnoControlContact->ensureControlVisibility( _bVisible );
        }
    }
}

//************************************************************
//   SdrUnoObj
//************************************************************

SdrUnoObj::SdrUnoObj(
	SdrModel& rSdrModel,
	const String& rModelName, 
	bool _bOwnUnoControlModel)
:	SdrRectObj(rSdrModel),
	m_pImpl( new SdrUnoObjDataHolder ),
	bOwnUnoControlModel( _bOwnUnoControlModel )
{
	m_pImpl->pEventListener = new SdrControlEventListenerImpl(this);

	// nur ein owner darf eigenstaendig erzeugen
	if (rModelName.Len())
		CreateUnoControlModel(rModelName);
}

SdrUnoObj::SdrUnoObj(
	SdrModel& rSdrModel,
	const String& rModelName,
					 const uno::Reference< lang::XMultiServiceFactory >& rxSFac,
	bool _bOwnUnoControlModel)
:	SdrRectObj(rSdrModel),
	m_pImpl( new SdrUnoObjDataHolder ),
	bOwnUnoControlModel( _bOwnUnoControlModel )
{
	m_pImpl->pEventListener = new SdrControlEventListenerImpl(this);

	// nur ein owner darf eigenstaendig erzeugen
	if (rModelName.Len())
		CreateUnoControlModel(rModelName,rxSFac);
}

SdrUnoObj::~SdrUnoObj()
{
    try
    {
        // clean up the control model
	    uno::Reference< lang::XComponent > xComp(xUnoControlModel, uno::UNO_QUERY);
        if (xComp.is())
        {
            // is the control model owned by it's environment?
            uno::Reference< container::XChild > xContent(xUnoControlModel, uno::UNO_QUERY);
            if (xContent.is() && !xContent->getParent().is())
                xComp->dispose();
            else
                m_pImpl->pEventListener->StopListening(xComp);
        }
    }
    catch( const uno::Exception& )
    {
    	OSL_ENSURE( sal_False, "SdrUnoObj::~SdrUnoObj: caught an exception!" );
    }
    delete m_pImpl;
}

void SdrUnoObj::copyDataFromSdrObject(const SdrObject& rSource)
{
	if(this != &rSource)
	{
		const SdrUnoObj* pSource = dynamic_cast< const SdrUnoObj* >(&rSource);

		if(pSource)
		{
			// call parent
			SdrRectObj::copyDataFromSdrObject(rSource);

			// release the reference to the current control model
			SetUnoControlModel(uno::Reference< awt::XControlModel >());

			aUnoControlModelTypeName = pSource->aUnoControlModelTypeName;
			aUnoControlTypeName = pSource->aUnoControlTypeName;

			// copy the uno control model
			uno::Reference< awt::XControlModel > xCtrl( pSource->GetUnoControlModel(), uno::UNO_QUERY );
			uno::Reference< util::XCloneable > xClone( xCtrl, uno::UNO_QUERY );

			if ( xClone.is() )
			{
				// copy the model by cloning
				uno::Reference< awt::XControlModel > xNewModel( xClone->createClone(), uno::UNO_QUERY );
				DBG_ASSERT( xNewModel.is(), "SdrUnoObj::operator =, no control model!");
				xUnoControlModel = xNewModel;
			}
			else
			{
				// copy the model by streaming
				uno::Reference< io::XPersistObject > xObj( xCtrl, uno::UNO_QUERY );
				uno::Reference< lang::XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );

				if ( xObj.is() && xFactory.is() )
				{
					// creating a pipe
					uno::Reference< io::XOutputStream > xOutPipe(xFactory->createInstance( rtl::OUString::createFromAscii("com.sun.star.io.Pipe")), uno::UNO_QUERY);
					uno::Reference< io::XInputStream > xInPipe(xOutPipe, uno::UNO_QUERY);

					// creating the mark streams
					uno::Reference< io::XInputStream > xMarkIn(xFactory->createInstance( rtl::OUString::createFromAscii("com.sun.star.io.MarkableInputStream")), uno::UNO_QUERY);
					uno::Reference< io::XActiveDataSink > xMarkSink(xMarkIn, uno::UNO_QUERY);

					uno::Reference< io::XOutputStream > xMarkOut(xFactory->createInstance( rtl::OUString::createFromAscii("com.sun.star.io.MarkableOutputStream")), uno::UNO_QUERY);
					uno::Reference< io::XActiveDataSource > xMarkSource(xMarkOut, uno::UNO_QUERY);

					// connect mark and sink
					uno::Reference< io::XActiveDataSink > xSink(xFactory->createInstance( rtl::OUString::createFromAscii("com.sun.star.io.ObjectInputStream")), uno::UNO_QUERY);

					// connect mark and source
					uno::Reference< io::XActiveDataSource > xSource(xFactory->createInstance( rtl::OUString::createFromAscii("com.sun.star.io.ObjectOutputStream")), uno::UNO_QUERY);

					uno::Reference< io::XObjectOutputStream > xOutStrm(xSource, uno::UNO_QUERY);
					uno::Reference< io::XObjectInputStream > xInStrm(xSink, uno::UNO_QUERY);

					if (xMarkSink.is() && xMarkSource.is() && xSink.is() && xSource.is())
					{
						xMarkSink->setInputStream(xInPipe);
						xMarkSource->setOutputStream(xOutPipe);
						xSink->setInputStream(xMarkIn);
						xSource->setOutputStream(xMarkOut);

						// write the object to source
						xOutStrm->writeObject(xObj);
						xOutStrm->closeOutput();
						// read the object
						uno::Reference< awt::XControlModel > xModel(xInStrm->readObject(), uno::UNO_QUERY);
						xInStrm->closeInput();

						DBG_ASSERT(xModel.is(), "SdrUnoObj::operator =, keine Model erzeugt");

						xUnoControlModel = xModel;
					}
				}
			}

			// get service name of the control from the control model
			uno::Reference< beans::XPropertySet > xSet(xUnoControlModel, uno::UNO_QUERY);
			if (xSet.is())
            {
				uno::Any aValue( xSet->getPropertyValue( rtl::OUString::createFromAscii("DefaultControl")) );
				::rtl::OUString aStr;

				if( aValue >>= aStr )
					aUnoControlTypeName = String(aStr);
            }

			uno::Reference< lang::XComponent > xComp(xUnoControlModel, uno::UNO_QUERY);
			if (xComp.is())
				m_pImpl->pEventListener->StartListening(xComp);
		}
		else
		{
			OSL_ENSURE(false, "copyDataFromSdrObject with ObjectType of Source different from Target (!)");
		}
	}
}

SdrObject* SdrUnoObj::CloneSdrObject(SdrModel* pTargetModel) const
{
	SdrUnoObj* pClone = new SdrUnoObj(
		pTargetModel ? *pTargetModel : getSdrModelFromSdrObject(), 
		String());
	OSL_ENSURE(pClone, "CloneSdrObject error (!)");
	pClone->copyDataFromSdrObject(*this);

	return pClone;
}

//bool SdrUnoObj::IsSdrUnoObj() const
//{
//    return true;
//}

void SdrUnoObj::TakeObjInfo(SdrObjTransformInfoRec& rInfo) const
{
	rInfo.mbRotateFreeAllowed = false;
	rInfo.mbRotate90Allowed = false;
	rInfo.mbMirrorFreeAllowed = false;
	rInfo.mbMirror45Allowed = false;
	rInfo.mbMirror90Allowed = false;
	rInfo.mbTransparenceAllowed = false;
	rInfo.mbGradientAllowed = false;
	rInfo.mbShearAllowed = false;
	rInfo.mbEdgeRadiusAllowed = false;
	rInfo.mbNoOrthoDesired = false;
	rInfo.mbCanConvToPath = false;
	rInfo.mbCanConvToPoly = false;
	rInfo.mbCanConvToPathLineToArea = false;
	rInfo.mbCanConvToPolyLineToArea = false;
	rInfo.mbCanConvToContour = false;
}

sal_uInt16 SdrUnoObj::GetObjIdentifier() const
{
	return sal_uInt16(OBJ_UNO);
}

void SdrUnoObj::SetContextWritingMode( const sal_Int16 _nContextWritingMode )
{
    try
    {
        uno::Reference< beans::XPropertySet > xModelProperties( GetUnoControlModel(), uno::UNO_QUERY_THROW );
        xModelProperties->setPropertyValue(
            ::rtl::OUString::intern( RTL_CONSTASCII_USTRINGPARAM( "ContextWritingMode" ) ),
            uno::makeAny( _nContextWritingMode )
        );
    }
    catch( const uno::Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

// ----------------------------------------------------------------------------
namespace
{
    /** helper class to restore graphics at <awt::XView> object after <SdrUnoObj::Paint>

        OD 08.05.2003 #109432#
        Restoration of graphics necessary to assure that paint on a window

        @author OD
    */
    class RestoreXViewGraphics
    {
        private:
            uno::Reference< awt::XView >        m_rXView;
            uno::Reference< awt::XGraphics >    m_rXGraphics;

        public:
            RestoreXViewGraphics( const uno::Reference< awt::XView >& _rXView )
            {
                m_rXView = _rXView;
                m_rXGraphics = m_rXView->getGraphics();
            }
            ~RestoreXViewGraphics()
            {
                m_rXView->setGraphics( m_rXGraphics );
            }
    };
}

void SdrUnoObj::TakeObjNameSingul(XubString& rName) const
{
	rName = ImpGetResStr(STR_ObjNameSingulUno);

	String aName( GetName() );
	if(aName.Len())
	{
		rName += sal_Unicode(' ');
		rName += sal_Unicode('\'');
		rName += aName;
		rName += sal_Unicode('\'');
	}
}

void SdrUnoObj::TakeObjNamePlural(XubString& rName) const
{
	rName = ImpGetResStr(STR_ObjNamePluralUno);
}

bool SdrUnoObj::hasSpecialDrag() const
{
    // no special drag; we have no rounding rect and
    // do want frame handles
    return false;
}

bool SdrUnoObj::supportsFullDrag() const
{
    // overloaded to have the possibility to enable/disable in debug and
    // to ckeck some things out. Current solution is working, so default is
    // enabled
    static bool bDoSupportFullDrag(true);
    
    return bDoSupportFullDrag;
}

SdrObject* SdrUnoObj::getFullDragClone() const
{
    SdrObject* pRetval = 0;
    static bool bHandleSpecial(false);

    if(bHandleSpecial)
    {
	    // special handling for SdrUnoObj (FormControl). Create a SdrGrafObj 
	    // for drag containing the graphical representation. This does not work too
        // well, so the default is to simply clone
	    pRetval = new SdrGrafObj(
			getSdrModelFromSdrObject(), 
			GetObjGraphic(*this), 
			getSdrObjectTransformation());
    }
    else
    {
        // call parent (simply clone)
        pRetval = SdrRectObj::getFullDragClone();
    }

	return pRetval;
}

// -----------------------------------------------------------------------------
void SdrUnoObj::SetLayer( SdrLayerID _nLayer )
{
	if(_nLayer != GetLayer())
    {
		// call parent
	    SdrRectObj::SetLayer( _nLayer );

		// check visibility
		const ::std::set< SdrView* > aViews(getSdrModelFromSdrObject().getSdrViews());

		for(::std::set< SdrView* >::const_iterator aLoop(aViews.begin()); 
			aLoop != aViews.end(); aLoop++)
        {
			SdrPageView* pPageView = (*aLoop)->GetSdrPageView();

			if(pPageView)
            {
				const SetOfByte& rVisibleLayers = pPageView->GetVisibleLayers();
				const bool bVisible(rVisibleLayers.IsSet(GetLayer()));

				lcl_ensureControlVisibility( *aLoop, this, bVisible );
			}
		}
    }
}

void SdrUnoObj::CreateUnoControlModel(const String& rModelName)
{
	DBG_ASSERT(!xUnoControlModel.is(), "model already exists");

	aUnoControlModelTypeName = rModelName;

	uno::Reference< awt::XControlModel >   xModel;
	uno::Reference< lang::XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );
	if (aUnoControlModelTypeName.Len() && xFactory.is() )
	{
		xModel = uno::Reference< awt::XControlModel >(xFactory->createInstance(
			aUnoControlModelTypeName), uno::UNO_QUERY);

		if (xModel.is())
			SetChanged();
	}

	SetUnoControlModel(xModel);
}

void SdrUnoObj::CreateUnoControlModel(const String& rModelName,
									  const uno::Reference< lang::XMultiServiceFactory >& rxSFac)
{
	DBG_ASSERT(!xUnoControlModel.is(), "model already exists");

	aUnoControlModelTypeName = rModelName;

	uno::Reference< awt::XControlModel >   xModel;
	if (aUnoControlModelTypeName.Len() && rxSFac.is() )
	{
		xModel = uno::Reference< awt::XControlModel >(rxSFac->createInstance(
			aUnoControlModelTypeName), uno::UNO_QUERY);

		if (xModel.is())
			SetChanged();
	}

	SetUnoControlModel(xModel);
}

void SdrUnoObj::SetUnoControlModel( const uno::Reference< awt::XControlModel >& xModel)
{
	if (xUnoControlModel.is())
	{
		uno::Reference< lang::XComponent > xComp(xUnoControlModel, uno::UNO_QUERY);
		if (xComp.is())
			m_pImpl->pEventListener->StopListening(xComp);
	}

	xUnoControlModel = xModel;

	// control model muss servicename des controls enthalten
	if (xUnoControlModel.is())
	{
		uno::Reference< beans::XPropertySet > xSet(xUnoControlModel, uno::UNO_QUERY);
		if (xSet.is())
		{
			uno::Any aValue( xSet->getPropertyValue(String("DefaultControl", gsl_getSystemTextEncoding())) );
            ::rtl::OUString aStr;
			if( aValue >>= aStr )
				aUnoControlTypeName = String(aStr);
		}

		uno::Reference< lang::XComponent > xComp(xUnoControlModel, uno::UNO_QUERY);
		if (xComp.is())
			m_pImpl->pEventListener->StartListening(xComp);
	}

    // invalidate all ViewObject contacts
    ViewContactOfUnoControl* pVC = NULL;
    if ( impl_getViewContact( pVC ) )
	{
		// flushViewObjectContacts() removes all existing VOCs for the local DrawHierarchy. This 
        // is always allowed since they will be re-created on demand (and with the changed model)
        GetViewContact().flushViewObjectContacts(true);
	}
}

//------------------------------------------------------------------------
uno::Reference< awt::XControl > SdrUnoObj::GetUnoControl(const SdrView& _rView, const OutputDevice& _rOut) const
{
    uno::Reference< awt::XControl > xControl;

    SdrPageView* pPageView = _rView.GetSdrPageView();

	if(!pPageView)
		return NULL;

    OSL_ENSURE( getSdrPageFromSdrObject() == &pPageView->getSdrPageFromSdrPageView(), 
		"SdrUnoObj::GetUnoControl: This object is not displayed in that particular view!" );
    if ( getSdrPageFromSdrObject() != &pPageView->getSdrPageFromSdrPageView() )
        return NULL;

    SdrPageWindow* pPageWindow = pPageView ? pPageView->FindPageWindow( _rOut ) : NULL;
    OSL_ENSURE( pPageWindow, "SdrUnoObj::GetUnoControl: did not find my SdrPageWindow!" );
    if ( !pPageWindow )
        return NULL;

    ViewObjectContact& rViewObjectContact( GetViewContact().GetViewObjectContact( pPageWindow->GetObjectContact() ) );
    ViewObjectContactOfUnoControl* pUnoContact = dynamic_cast< ViewObjectContactOfUnoControl* >( &rViewObjectContact );
    OSL_ENSURE( pUnoContact, "SdrUnoObj::GetUnoControl: wrong contact type!" );
    if ( pUnoContact )
        xControl = pUnoContact->getControl();
  
    return xControl;
}

//------------------------------------------------------------------------
uno::Reference< awt::XControl > SdrUnoObj::GetTemporaryControlForWindow(
    const Window& _rWindow, uno::Reference< awt::XControlContainer >& _inout_ControlContainer ) const
{
    uno::Reference< awt::XControl > xControl;
  
    ViewContactOfUnoControl* pVC = NULL;
    if ( impl_getViewContact( pVC ) )
        xControl = pVC->getTemporaryControlForWindow( _rWindow, _inout_ControlContainer );
  
    return xControl;
}

//------------------------------------------------------------------------
bool SdrUnoObj::impl_getViewContact( ViewContactOfUnoControl*& _out_rpContact ) const
{
    ViewContact& rViewContact( GetViewContact() );
    _out_rpContact = dynamic_cast< ViewContactOfUnoControl* >( &rViewContact );
    DBG_ASSERT( _out_rpContact, "SdrUnoObj::impl_getViewContact: could not find my ViewContact!" );
    return ( _out_rpContact != NULL );
}

//------------------------------------------------------------------------
::sdr::contact::ViewContact* SdrUnoObj::CreateObjectSpecificViewContact()
{
  return new ::sdr::contact::ViewContactOfUnoControl( *this );
}

// -----------------------------------------------------------------------------
// eof