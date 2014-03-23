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



#include "precompiled_svx.hxx"
#include <svx/sdr/primitive2d/sdrpathprimitive2d.hxx>
#include <svx/sdr/primitive2d/sdrdecompositiontools.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <svx/sdr/primitive2d/svx_primitivetypes2d.hxx>
#include <drawinglayer/primitive2d/sdrdecompositiontools2d.hxx>

//////////////////////////////////////////////////////////////////////////////

using namespace com::sun::star;

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
	namespace primitive2d
	{
		Primitive2DSequence SdrPathPrimitive2D::create2DDecomposition(const geometry::ViewInformation2D& /*aViewInformation*/) const
		{
			Primitive2DSequence aRetval;

			// add fill
			if(!getSdrLFSTAttribute().getFill().isDefault() 
				&& getUnitPolyPolygon().isClosed())
			{
                // #i108255# no need to use correctOrientations here; target is
                // straight visualisation
                basegfx::B2DPolyPolygon aTransformed(getUnitPolyPolygon());

                aTransformed.transform(getTransform());
                appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, 
                    createPolyPolygonFillPrimitive(
                        aTransformed, 
                        getSdrLFSTAttribute().getFill(), 
                        getSdrLFSTAttribute().getFillFloatTransGradient()));
			}

			// add line
			if(getSdrLFSTAttribute().getLine().isDefault())
			{
                // if initially no line is defined, create one for HitTest and BoundRect
                appendPrimitive2DReferenceToPrimitive2DSequence(aRetval,
                    createHiddenGeometryPrimitives2D(
                        false,
                        getUnitPolyPolygon(),
                        getTransform()));
            }
            else
            {
                Primitive2DSequence aTemp(getUnitPolyPolygon().count());

                for(sal_uInt32 a(0); a < getUnitPolyPolygon().count(); a++)
                {
                    basegfx::B2DPolygon aTransformed(getUnitPolyPolygon().getB2DPolygon(a));

                    aTransformed.transform(getTransform());
                    aTemp[a] = createPolygonLinePrimitive(
                        aTransformed, 
                        getSdrLFSTAttribute().getLine(), 
                        getSdrLFSTAttribute().getLineStartEnd());
                }

                appendPrimitive2DSequenceToPrimitive2DSequence(aRetval, aTemp);
            }

			// add text
			if(!getSdrLFSTAttribute().getText().isDefault())
			{
				appendPrimitive2DReferenceToPrimitive2DSequence(aRetval, 
                    createTextPrimitive(
                        getUnitPolyPolygon(), 
                        getTransform(), 
                        getSdrLFSTAttribute().getText(), 
                        getSdrLFSTAttribute().getLine(), 
                        false, 
                        false, 
                        false));
			}

			// add shadow
			if(!getSdrLFSTAttribute().getShadow().isDefault())
			{
                aRetval = createEmbeddedShadowPrimitive(
                    aRetval, 
                    getSdrLFSTAttribute().getShadow());
			}

			return aRetval;
		}

		SdrPathPrimitive2D::SdrPathPrimitive2D(
			const basegfx::B2DHomMatrix& rTransform, 
			const attribute::SdrLineFillShadowTextAttribute& rSdrLFSTAttribute,
			const basegfx::B2DPolyPolygon& rUnitPolyPolygon)
		:	BufferedDecompositionPrimitive2D(),
			maTransform(rTransform),
			maSdrLFSTAttribute(rSdrLFSTAttribute),
			maUnitPolyPolygon(rUnitPolyPolygon)
		{
		}

		// provide unique ID
		ImplPrimitrive2DIDBlock(SdrPathPrimitive2D, PRIMITIVE2D_ID_SDRPATHPRIMITIVE2D)

	} // end of namespace primitive2d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// eof