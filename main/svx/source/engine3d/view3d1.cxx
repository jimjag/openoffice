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

#include <tools/shl.hxx>
#include "svx/svditer.hxx"
#include <svx/svdpool.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svxids.hrc>
#include <svx/xtable.hxx>
#include <svx/fmview.hxx>
#include <svx/dialogs.hrc>
#include <svx/dialmgr.hxx>
#include "svx/globl3d.hxx"
#include <svx/obj3d.hxx>
#include <svx/polysc3d.hxx>
#include <svx/e3ditem.hxx>
#include <editeng/colritem.hxx>
#include <svx/lathe3d.hxx>
#include <svx/sphere3d.hxx>
#include <svx/extrud3d.hxx>
#include <svx/e3dundo.hxx>
#include <svx/view3d.hxx>
#include <svx/cube3d.hxx>
#include <svx/xflclit.hxx>
#include <svx/svdogrp.hxx>
#include <svx/e3dsceneupdater.hxx>

/*************************************************************************
|*
|* Konvertierung in Polygone
|*
\************************************************************************/

void E3dView::ConvertMarkedToPolyObj(sal_Bool bLineToArea)
{
	SdrObject* pNewObj = NULL;

	if (GetMarkedObjectCount() == 1)
	{
		SdrObject* pObj = GetMarkedObjectByIndex(0);

		if (pObj && pObj->ISA(E3dPolyScene))
		{
			sal_Bool bBezier = sal_False;
			pNewObj = ((E3dPolyScene*) pObj)->ConvertToPolyObj(bBezier, bLineToArea);

			if (pNewObj)
			{
				BegUndo(SVX_RESSTR(RID_SVX_3D_UNDO_EXTRUDE));
				ReplaceObjectAtView(pObj, *GetSdrPageView(), pNewObj);
				EndUndo();
			}
		}
	}

	if (!pNewObj)
	{
		SdrView::ConvertMarkedToPolyObj(bLineToArea);
	}
}

/*************************************************************************
|*
|* Get3DAttributes
|*
\************************************************************************/

void Imp_E3dView_InorderRun3DObjects(const SdrObject* pObj, sal_uInt32& rMask)
{
	if(pObj->ISA(E3dLatheObj))
	{
		rMask |= 0x0001;
	}
	else if(pObj->ISA(E3dExtrudeObj))
	{
		rMask |= 0x0002;
	}
	else if(pObj->ISA(E3dSphereObj))
	{
		rMask |= 0x0004;
	}
	else if(pObj->ISA(E3dCubeObj))
	{
		rMask |= 0x0008;
	}
	else if(pObj->IsGroupObject())
	{
		SdrObjList* pList = pObj->GetSubList();
		for(sal_uInt32 a(0); a < pList->GetObjCount(); a++)
			Imp_E3dView_InorderRun3DObjects(pList->GetObj(a), rMask);
	}
}

SfxItemSet E3dView::Get3DAttributes(E3dScene* pInScene,	sal_Bool /*bOnly3DAttr*/) const
{
	// ItemSet mit entspr. Bereich anlegen
	SfxItemSet aSet(
		pMod->GetItemPool(),
		SDRATTR_START,		SDRATTR_END,
		SID_ATTR_3D_INTERN,	SID_ATTR_3D_INTERN,
		0, 0);

	sal_uInt32 nSelectedItems(0L);

	if(pInScene)
	{
		// special scene
		aSet.Put(pInScene->GetMergedItemSet());
	}
	else
	{
		// get attributes from all selected objects
		MergeAttrFromMarked(aSet, sal_False);

		// calc flags for SID_ATTR_3D_INTERN
		const SdrMarkList& rMarkList = GetMarkedObjectList();
		sal_uInt32 nMarkCnt(rMarkList.GetMarkCount());

		for(sal_uInt32 a(0); a < nMarkCnt; a++)
		{
			SdrObject* pObj = GetMarkedObjectByIndex(a);
			Imp_E3dView_InorderRun3DObjects(pObj, nSelectedItems);
		}
	}

	// setze SID_ATTR_3D_INTERN auf den Status der selektierten Objekte
	aSet.Put(SfxUInt32Item(SID_ATTR_3D_INTERN, nSelectedItems));

	// DefaultValues pflegen
	if(!nSelectedItems  && !pInScene)
	{
		// Defaults holen und hinzufuegen
		SfxItemSet aDefaultSet(pMod->GetItemPool(), SDRATTR_3D_FIRST, SDRATTR_3D_LAST);
		GetAttributes(aDefaultSet);
		aSet.Put(aDefaultSet);

		// ... aber keine Linien fuer 3D
		aSet.Put(XLineStyleItem (XLINE_NONE));

		// #84061# new defaults for distance and focal length
		aSet.Put(Svx3DDistanceItem(100));
		aSet.Put(Svx3DFocalLengthItem(10000));
	}

	// ItemSet zurueckgeben
	return(aSet);
}

/*************************************************************************
|*
|* Set3DAttributes:
|*
\************************************************************************/

void E3dView::Set3DAttributes( const SfxItemSet& rAttr,	E3dScene* pInScene, sal_Bool bReplaceAll)
{
	sal_uInt32 nSelectedItems(0L);

	if(pInScene)
	{
		//pInScene->SetItemSetAndBroadcast(rAttr, bReplaceAll);
		pInScene->SetMergedItemSetAndBroadcast(rAttr, bReplaceAll);
	}
	else
	{
        // #i94832# removed usage of E3DModifySceneSnapRectUpdater here.
        // They are not needed here, they are already handled in SetAttrToMarked

        // set at selected objects
		SetAttrToMarked(rAttr, bReplaceAll);

        // old run
        const SdrMarkList& rMarkList = GetMarkedObjectList();
		const sal_uInt32 nMarkCnt(rMarkList.GetMarkCount());

        for(sal_uInt32 a(0); a < nMarkCnt; a++)
        {
			SdrObject* pObj = GetMarkedObjectByIndex(a);
			Imp_E3dView_InorderRun3DObjects(pObj, nSelectedItems);
        }
	}

	// DefaultValues pflegen
	if(!nSelectedItems && !pInScene)
	{
		// Defaults setzen
		SfxItemSet aDefaultSet(pMod->GetItemPool(), SDRATTR_3D_FIRST, SDRATTR_3D_LAST);
		aDefaultSet.Put(rAttr);
		SetAttributes(aDefaultSet);

	}
}

double E3dView::GetDefaultCamPosZ()
{
	return (double)((const SfxUInt32Item&)pMod->GetItemPool().GetDefaultItem(SDRATTR_3DSCENE_DISTANCE)).GetValue();
}

double E3dView::GetDefaultCamFocal()
{
	return (double)((const SfxUInt32Item&)pMod->GetItemPool().GetDefaultItem(SDRATTR_3DSCENE_FOCAL_LENGTH)).GetValue();
}

