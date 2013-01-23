/**
 * @file class_aperture_macro.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <trigo.h>
#include <gr_basic.h>

#include <gerbview.h>
#include <class_GERBER.h>



/**
 * Function scaletoIU
 * converts a distance given in floating point to our internal units
 */
extern int scaletoIU( double aCoord, bool isMetric );       // defined it rs274d_read_XY_and_IJ_coordiantes.cpp

/**
 * Function mapPt
 * translates a point from the aperture macro coordinate system to our
 * deci-mils coordinate system.
 * @return wxPoint - The GerbView coordinate system vector.
 */
static wxPoint mapPt( double x, double y, bool isMetric )
{
    wxPoint ret( scaletoIU( x, isMetric ), scaletoIU( y, isMetric ) );

    return ret;
}

/**
 * Function mapExposure
 * translates the first parameter from an aperture macro into a current
 * exposure setting.
 * @param aParent = a GERBER_DRAW_ITEM that handle:
 *    ** m_Exposure A dynamic setting which can change throughout the
 *          reading of the gerber file, and it indicates whether the current tool
 *          is lit or not.
 *    ** m_ImageNegative A dynamic setting which can change throughout the reading
 *          of the gerber file, and it indicates whether the current D codes are to
 *          be interpreted as erasures or not.
 * @return true to draw with current color, false to draw with alt color (erase)
 */
bool AM_PRIMITIVE::mapExposure( GERBER_DRAW_ITEM* aParent )
{
    bool exposure;
    switch( primitive_id )
    {
    case AMP_CIRCLE:
    case AMP_LINE2:
    case AMP_LINE20:
    case AMP_LINE_CENTER:
    case AMP_LINE_LOWER_LEFT:
    case AMP_OUTLINE:
    case AMP_THERMAL:
    case AMP_POLYGON:
        // All have an exposure parameter and can return true or false
        switch( GetExposure(aParent) )
        {
        case 0:     // exposure always OFF
            exposure = false;
            break;

        default:
        case 1:     // exposure always OON
            exposure = true;
            break;

        case 2:     // reverse exposure
            exposure = !aParent->GetLayerPolarity();
        }
        break;

    case AMP_MOIRE:
    case AMP_EOF:
    case AMP_UNKNOWN:
    default:
        return true;    // All have no exposure parameter and must return true (no change for exposure)
        break;
    }

    return exposure ^ aParent->m_imageParams->m_ImageNegative;
}


/**
 * Function GetExposure
 * returns the first parameter in integer form.  Some but not all primitives
 * use the first parameter as an exposure control.
 */
int AM_PRIMITIVE::GetExposure(GERBER_DRAW_ITEM* aParent) const
{
    // No D_CODE* for GetValue()
    wxASSERT( params.size() && params[0].IsImmediate() );
    return (int) params[0].GetValue( aParent->GetDcodeDescr() );
}

/**
 * Function DrawBasicShape
 * Draw the primitive shape for flashed items.
 */
void AM_PRIMITIVE::DrawBasicShape( GERBER_DRAW_ITEM* aParent,
                                   EDA_RECT* aClipBox,
                                   wxDC* aDC,
                                   EDA_COLOR_T aColor, EDA_COLOR_T aAltColor,
                                   wxPoint aShapePos,
                                   bool aFilledShape )
{
    static std::vector<wxPoint> polybuffer;     // create a static buffer to avoid a lot of memory reallocation
    polybuffer.clear();

    wxPoint curPos = aShapePos;
    D_CODE* tool   = aParent->GetDcodeDescr();
    int rotation;
    if( mapExposure( aParent ) == false )
    {
        EXCHG(aColor, aAltColor);
    }

    switch( primitive_id )
    {
    case AMP_CIRCLE:        // Circle, given diameter and position
    {
        /* Generated by an aperture macro declaration like:
         * "1,1,0.3,0.5, 1.0*"
         * type (1), exposure, diameter, pos.x, pos.y
         * type is not stored in parameters list, so the first parameter is exposure
         */
        curPos += mapPt( params[2].GetValue( tool ), params[3].GetValue( tool ), m_GerbMetric );
        curPos = aParent->GetABPosition( curPos );
        int radius = scaletoIU( params[1].GetValue( tool ), m_GerbMetric ) / 2;
        if( !aFilledShape )
            GRCircle( aClipBox, aDC, curPos, radius, 0, aColor );
        else
            GRFilledCircle( aClipBox, aDC, curPos, radius, aColor );
    }
    break;

    case AMP_LINE2:
    case AMP_LINE20:        // Line with rectangle ends. (Width, start and end pos + rotation)
    {
        /* Generated by an aperture macro declaration like:
         * "2,1,0.3,0,0, 0.5, 1.0,-135*"
         * type (2), exposure, width, start.x, start.y, end.x, end.y, rotation
         * type is not stored in parameters list, so the first parameter is exposure
         */
        ConvertShapeToPolygon( aParent, polybuffer );

        // shape rotation:
        rotation = KiROUND( params[6].GetValue( tool ) * 10.0 );
        if( rotation )
        {
            for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
                RotatePoint( &polybuffer[ii], -rotation );
        }

        // Move to current position:
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }

        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
    }
    break;

    case AMP_LINE_CENTER:
    {
        /* Generated by an aperture macro declaration like:
         * "21,1,0.3,0.03,0,0,-135*"
         * type (21), exposure, ,width, height, center pos.x, center pos.y, rotation
         * type is not stored in parameters list, so the first parameter is exposure
         */
        ConvertShapeToPolygon( aParent, polybuffer );

        // shape rotation:
        rotation = KiROUND( params[5].GetValue( tool ) * 10.0 );
        if( rotation )
        {
            for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
                RotatePoint( &polybuffer[ii], -rotation );
        }

        // Move to current position:
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }

        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
    }
    break;

    case AMP_LINE_LOWER_LEFT:
    {
        /* Generated by an aperture macro declaration like:
         * "22,1,0.3,0.03,0,0,-135*"
         * type (22), exposure, ,width, height, corner pos.x, corner pos.y, rotation
         * type is not stored in parameters list, so the first parameter is exposure
         */
        ConvertShapeToPolygon( aParent, polybuffer );

        // shape rotation:
        rotation = KiROUND( params[5].GetValue( tool ) * 10.0 );
        if( rotation )
        {
            for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
                RotatePoint( &polybuffer[ii], -rotation );
        }

        // Move to current position:
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }

        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
    }
    break;

    case AMP_THERMAL:
    {
        /* Generated by an aperture macro declaration like:
         * "7, 0,0,1.0,0.3,0.01,-13*"
         * type (7), center.x , center.y, outside diam, inside diam, crosshair thickness, rotation
         * type is not stored in parameters list, so the first parameter is center.x
         */
        curPos += mapPt( params[0].GetValue( tool ), params[1].GetValue( tool ), m_GerbMetric );
        ConvertShapeToPolygon( aParent, polybuffer );

        // shape rotation:
        rotation = KiROUND( params[5].GetValue( tool ) * 10.0 );

        // Because a thermal shape has 4 identical sub-shapes, only one is created in polybuffer.
        // We must draw 4 sub-shapes rotated by 90 deg
        std::vector<wxPoint> subshape_poly;
        for( int ii = 0; ii < 4; ii++ )
        {
            subshape_poly = polybuffer;
            int sub_rotation = rotation + 900 * ii;
            for( unsigned jj = 0; jj < subshape_poly.size(); jj++ )
                RotatePoint( &subshape_poly[jj], -sub_rotation );

            // Move to current position:
            for( unsigned jj = 0; jj < subshape_poly.size(); jj++ )
            {
                subshape_poly[jj] += curPos;
                subshape_poly[jj] = aParent->GetABPosition( subshape_poly[jj] );
            }

            GRClosedPoly( aClipBox, aDC,
                          subshape_poly.size(), &subshape_poly[0], true, aAltColor,
                          aAltColor );
        }
    }
    break;

    case AMP_MOIRE:     // A cross hair with n concentric circles
    {
        curPos += mapPt( params[0].GetValue( tool ), params[1].GetValue( tool ),
                         m_GerbMetric );

        /* Generated by an aperture macro declaration like:
         * "6,0,0,0.125,.01,0.01,3,0.003,0.150,0"
         * type(6), pos.x, pos.y, diam, penwidth, gap, circlecount, crosshair thickness, crosshaire len, rotation
         * type is not stored in parameters list, so the first parameter is pos.x
         */
        int outerDiam    = scaletoIU( params[2].GetValue( tool ), m_GerbMetric );
        int penThickness = scaletoIU( params[3].GetValue( tool ), m_GerbMetric );
        int gap = scaletoIU( params[4].GetValue( tool ), m_GerbMetric );
        int numCircles = KiROUND( params[5].GetValue( tool ) );

        // Draw circles:
        wxPoint center = aParent->GetABPosition( curPos );
        // adjust outerDiam by this on each nested circle
        int diamAdjust = (gap + penThickness); //*2;     //Should we use * 2 ?
        for( int i = 0; i < numCircles; ++i, outerDiam -= diamAdjust )
        {
            if( outerDiam <= 0 )
                break;
            if( !aFilledShape )
            {
                // draw the border of the pen's path using two circles, each as narrow as possible
                GRCircle( aClipBox, aDC, center, outerDiam / 2, 0, aColor );
                GRCircle( aClipBox, aDC, center, outerDiam / 2 - penThickness, 0, aColor );
            }
            else    // Filled mode
            {
                GRCircle( aClipBox, aDC, center,
                          (outerDiam - penThickness) / 2, penThickness, aColor );
            }
        }

        // Draw the cross:
        ConvertShapeToPolygon( aParent, polybuffer );

        rotation = KiROUND( params[8].GetValue( tool ) * 10.0 );
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            // shape rotation:
            RotatePoint( &polybuffer[ii], -rotation );
            // Move to current position:
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }

        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
    }
    break;

    case AMP_OUTLINE:
    {
        /* Generated by an aperture macro declaration like:
         * "4,1,3,0.0,0.0,0.0,0.5,0.5,0.5,0.5,0.0,-25"
         * type(4), exposure, corners count, corner1.x, corner.1y, ..., rotation
         * type is not stored in parameters list, so the first parameter is exposure
         */
        int numPoints = (int) params[1].GetValue( tool );
        rotation  = KiROUND( params[numPoints * 2 + 4].GetValue( tool ) * 10.0 );
        wxPoint pos;
        // Read points. numPoints does not include the starting point, so add 1.
        for( int i = 0; i<numPoints + 1; ++i )
        {
            int jj = i * 2 + 2;
            pos.x = scaletoIU( params[jj].GetValue( tool ), m_GerbMetric );
            pos.y = scaletoIU( params[jj + 1].GetValue( tool ), m_GerbMetric );
            polybuffer.push_back(pos);
        }
        // rotate polygon and move it to the actual position
        // shape rotation:
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            RotatePoint( &polybuffer[ii], -rotation );
       }

        // Move to current position:
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }

        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
    }
    break;

    case AMP_POLYGON:   // Is a regular polygon
        /* Generated by an aperture macro declaration like:
         * "5,1,0.6,0,0,0.5,25"
         * type(5), exposure, vertices count, pox.x, pos.y, diameter, rotation
         * type is not stored in parameters list, so the first parameter is exposure
         */
        curPos += mapPt( params[2].GetValue( tool ), params[3].GetValue( tool ), m_GerbMetric );
        // Creates the shape:
        ConvertShapeToPolygon( aParent, polybuffer );

        // rotate polygon and move it to the actual position
        rotation  = KiROUND( params[5].GetValue( tool ) * 10.0 );
        for( unsigned ii = 0; ii < polybuffer.size(); ii++ )
        {
            RotatePoint( &polybuffer[ii], -rotation );
            polybuffer[ii] += curPos;
            polybuffer[ii] = aParent->GetABPosition( polybuffer[ii] );
        }
        GRClosedPoly( aClipBox, aDC,
                      polybuffer.size(), &polybuffer[0], aFilledShape, aColor, aColor );
        break;

    case AMP_EOF:
        // not yet supported, waiting for you.
        break;

    case AMP_UNKNOWN:
    default:
        D( printf( "AM_PRIMITIVE::DrawBasicShape() err: unknown prim id %d\n",primitive_id) );
        break;
    }
}


/**
 * Function ConvertShapeToPolygon (virtual)
 * convert a shape to an equivalent polygon.
 * Arcs and circles are approximated by segments
 * Useful when a shape is not a graphic primitive (shape with hole,
 * rotated shape ... ) and cannot be easily drawn.
 * note for some schapes conbining circles and solid lines (rectangles), only rectangles are converted
 * because circles are very easy to draw (no rotation problem) so convert them in polygons,
 * and draw them as polygons is not a good idea.
 */
void AM_PRIMITIVE::ConvertShapeToPolygon( GERBER_DRAW_ITEM*     aParent,
                                          std::vector<wxPoint>& aBuffer )
{
    D_CODE* tool = aParent->GetDcodeDescr();

    switch( primitive_id )
    {
    case AMP_CIRCLE:        // Circle, currently convertion not needed
        break;

    case AMP_LINE2:
    case AMP_LINE20:        // Line with rectangle ends. (Width, start and end pos + rotation)
    {
        int     width = scaletoIU( params[1].GetValue( tool ), m_GerbMetric );
        wxPoint start = mapPt( params[2].GetValue( tool ),
                               params[3].GetValue( tool ), m_GerbMetric );
        wxPoint end = mapPt( params[4].GetValue( tool ),
                             params[5].GetValue( tool ), m_GerbMetric );
        wxPoint delta = end - start;
        int     len   = KiROUND( hypot( delta.x, delta.y ) );

        // To build the polygon, we must create a horizonta polygon starting to "start"
        // and rotate it to have it end point to "end"
        wxPoint currpt;
        currpt.y += width / 2;          // Upper left
        aBuffer.push_back( currpt );
        currpt.x = len;                 // Upper right
        aBuffer.push_back( currpt );
        currpt.y -= width;              // lower right
        aBuffer.push_back( currpt );
        currpt.x = 0;                   // Upper left
        aBuffer.push_back( currpt );

        // Rotate rectangle and move it to the actual start point
        int angle = KiROUND( atan2( (double) delta.y, (double) delta.x ) * 1800.0 / M_PI );

        for( unsigned ii = 0; ii < 4; ii++ )
        {
            RotatePoint( &aBuffer[ii], -angle );
            aBuffer[ii] += start;
        }
    }
    break;

    case AMP_LINE_CENTER:
    {
        wxPoint size = mapPt( params[1].GetValue( tool ), params[2].GetValue( tool ), m_GerbMetric );
        wxPoint pos  = mapPt( params[3].GetValue( tool ), params[4].GetValue( tool ), m_GerbMetric );

        // Build poly:
        pos.x -= size.x / 2;
        pos.y -= size.y / 2;        // Lower left
        aBuffer.push_back( pos );
        pos.y += size.y;            // Upper left
        aBuffer.push_back( pos );
        pos.x += size.x;            // Upper right
        aBuffer.push_back( pos );
        pos.y -= size.y;            // lower right
        aBuffer.push_back( pos );
    }
    break;

    case AMP_LINE_LOWER_LEFT:
    {
        wxPoint size = mapPt( params[1].GetValue( tool ), params[2].GetValue( tool ), m_GerbMetric );
        wxPoint lowerLeft = mapPt( params[3].GetValue( tool ), params[4].GetValue(
                                       tool ), m_GerbMetric );

        // Build poly:
        aBuffer.push_back( lowerLeft );
        lowerLeft.y += size.y;          // Upper left
        aBuffer.push_back( lowerLeft );
        lowerLeft.x += size.x;          // Upper right
        aBuffer.push_back( lowerLeft );
        lowerLeft.y -= size.y;          // lower right
        aBuffer.push_back( lowerLeft );
    }
    break;

    case AMP_THERMAL:
    {
        // Only 1/4 of the full shape is built, because the other 3 shapes will be draw from this first
        // rotated by 90, 180 and 270 deg.
        // params = center.x (unused here), center.y (unused here), outside diam, inside diam, crosshair thickness
        int outerRadius   = scaletoIU( params[2].GetValue( tool ), m_GerbMetric ) / 2;
        int innerRadius   = scaletoIU( params[3].GetValue( tool ), m_GerbMetric ) / 2;
        int halfthickness = scaletoIU( params[4].GetValue( tool ), m_GerbMetric ) / 2;
        int angle_start   = KiROUND( asin(
                                         (double) halfthickness / innerRadius ) * 1800 / M_PI );

        // Draw shape in the first cadrant (X and Y > 0)
        wxPoint pos, startpos;

        // Inner arc
        startpos.x = innerRadius;
        int angle_end = 900 - angle_start;
        int angle;
        for( angle = angle_start; angle < angle_end; angle += 100 )
        {
            pos = startpos;
            RotatePoint( &pos, angle );
            aBuffer.push_back( pos );
        }

        // Last point
        pos = startpos;
        RotatePoint( &pos, angle_end );
        aBuffer.push_back( pos );

        // outer arc
        startpos.x  = outerRadius;
        startpos.y  = 0;
        angle_start = KiROUND( asin( (double) halfthickness / outerRadius ) * 1800 / M_PI );
        angle_end   = 900 - angle_start;

        // First point, near Y axis, outer arc
        for( angle = angle_end; angle > angle_start; angle -= 100 )
        {
            pos = startpos;
            RotatePoint( &pos, angle );
            aBuffer.push_back( pos );
        }

        // last point
        pos = startpos;
        RotatePoint( &pos, angle_start );
        aBuffer.push_back( pos );

        aBuffer.push_back( aBuffer[0] );  // Close poly
    }
    break;

    case AMP_MOIRE:     // A cross hair with n concentric circles. Only the cros is build as polygon
                        // because circles can be drawn easily
    {
        int crossHairThickness = scaletoIU( params[6].GetValue( tool ), m_GerbMetric );
        int crossHairLength    = scaletoIU( params[7].GetValue( tool ), m_GerbMetric );

        // Create cross. First create 1/4 of the shape.
        // Others point are the same, totated by 90, 180 and 270 deg
        wxPoint pos( crossHairThickness / 2, crossHairLength / 2 );
        aBuffer.push_back( pos );
        pos.y = crossHairThickness / 2;
        aBuffer.push_back( pos );
        pos.x = -crossHairLength / 2;
        aBuffer.push_back( pos );
        pos.y = -crossHairThickness / 2;
        aBuffer.push_back( pos );

        // Copy the 4 shape, rotated by 90, 180 and 270 deg
        for( int jj = 1; jj <= 3; jj ++ )
        {
            for( int ii = 0; ii < 4; ii++ )
            {
                pos = aBuffer[ii];
                RotatePoint( &pos, jj*900 );
                aBuffer.push_back( pos );
            }
        }
    }
    break;

    case AMP_OUTLINE:
        // already is a polygon. Do nothing
        break;

    case AMP_POLYGON:   // Creates a regular polygon
    {
        int vertexcount = KiROUND( params[1].GetValue( tool ) );
        int radius    = scaletoIU( params[4].GetValue( tool ), m_GerbMetric ) / 2;
        // rs274x said: vertex count = 3 ... 10, and the first corner is on the X axis
        if( vertexcount < 3 )
            vertexcount = 3;
        if( vertexcount > 10 )
            vertexcount = 10;
        for( int ii = 0; ii <= vertexcount; ii++ )
        {
            wxPoint pos( radius, 0);
            RotatePoint( &pos, ii * 3600 / vertexcount );
            aBuffer.push_back( pos );
        }
    }
        break;

    case AMP_COMMENT:
    case AMP_UNKNOWN:
    case AMP_EOF:
        break;
    }
}

/** GetShapeDim
 * Calculate a value that can be used to evaluate the size of text
 * when displaying the D-Code of an item
 * due to the complexity of the shape of some primitives
 * one cannot calculate the "size" of a shape (only abounding box)
 * but here, the "dimension" of the shape is the diameter of the primitive
 * or for lines the width of the line
 * @param aParent = the parent GERBER_DRAW_ITEM which is actually drawn
 * @return a dimension, or -1 if no dim to calculate
  */
int AM_PRIMITIVE::GetShapeDim( GERBER_DRAW_ITEM* aParent )
{
    int dim = -1;
    D_CODE* tool = aParent->GetDcodeDescr();

    switch( primitive_id )
    {
    case AMP_CIRCLE:
        // params = exposure, diameter, pos.x, pos.y
        dim = scaletoIU( params[1].GetValue( tool ), m_GerbMetric );     // Diameter
        break;

    case AMP_LINE2:
    case AMP_LINE20:        // Line with rectangle ends. (Width, start and end pos + rotation)
        dim  = scaletoIU( params[1].GetValue( tool ), m_GerbMetric );   // linne width
    break;

    case AMP_LINE_CENTER:
    {
        wxPoint size = mapPt( params[1].GetValue( tool ), params[2].GetValue( tool ), m_GerbMetric );
        dim = std::min(size.x, size.y);
    }
    break;

    case AMP_LINE_LOWER_LEFT:
    {
        wxPoint size = mapPt( params[1].GetValue( tool ), params[2].GetValue( tool ), m_GerbMetric );
        dim = std::min(size.x, size.y);
    }
    break;

    case AMP_THERMAL:
    {
        // Only 1/4 of the full shape is built, because the other 3 shapes will be draw from this first
        // rotated by 90, 180 and 270 deg.
        // params = center.x (unused here), center.y (unused here), outside diam, inside diam, crosshair thickness
        dim   = scaletoIU( params[2].GetValue( tool ), m_GerbMetric ) / 2;  // Outer diam
    }
    break;

    case AMP_MOIRE:     // A cross hair with n concentric circles.
        dim = scaletoIU( params[7].GetValue( tool ), m_GerbMetric );    // = cross hair len
        break;

    case AMP_OUTLINE:   // a free polygon :
    // dim = min side of the bounding box (this is a poor criteria, but what is a good criteria b?)
    {
        // exposure, corners count, corner1.x, corner.1y, ..., rotation
        int numPoints = (int) params[1].GetValue( tool );
        // Read points. numPoints does not include the starting point, so add 1.
        // and calculate the bounding box;
        wxSize pos_min, pos_max, pos;
        for( int i = 0; i<numPoints + 1; ++i )
        {
            int jj = i * 2 + 2;
            pos.x = scaletoIU( params[jj].GetValue( tool ), m_GerbMetric );
            pos.y = scaletoIU( params[jj + 1].GetValue( tool ), m_GerbMetric );
            if( i == 0 )
                pos_min = pos_max = pos;
            else
            {
                // upper right corner:
                if( pos_min.x > pos.x )
                    pos_min.x = pos.x;
                if( pos_min.y > pos.y )
                    pos_min.y = pos.y;
                // lower left corner:
                if( pos_max.x < pos.x )
                    pos_max.x = pos.x;
                if( pos_max.y < pos.y )
                    pos_max.y = pos.y;
            }
        }
        // calculate dim
        wxSize size;
        size.x = pos_max.x - pos_min.x;
        size.y = pos_max.y - pos_min.y;
        dim = std::min( size.x, size.y );
    }
        break;

    case AMP_POLYGON:   // Regular polygon
        dim = scaletoIU( params[4].GetValue( tool ), m_GerbMetric ) / 2;      // Radius
        break;

    case AMP_COMMENT:
    case AMP_UNKNOWN:
    case AMP_EOF:
        break;
    }
    return dim;
}


/**
 * Function DrawApertureMacroShape
 * Draw the primitive shape for flashed items.
 * When an item is flashed, this is the shape of the item
 */
void APERTURE_MACRO::DrawApertureMacroShape( GERBER_DRAW_ITEM* aParent,
                                             EDA_RECT* aClipBox, wxDC* aDC,
                                             EDA_COLOR_T aColor, EDA_COLOR_T aAltColor,
                                             wxPoint aShapePos, bool aFilledShape )
{
    for( AM_PRIMITIVES::iterator prim_macro = primitives.begin();
         prim_macro != primitives.end(); ++prim_macro )
    {
        prim_macro->DrawBasicShape( aParent, aClipBox, aDC,
                                    aColor, aAltColor,
                                    aShapePos,
                                    aFilledShape );
    }
}

/* Function HasNegativeItems
 * return true if this macro has at least one aperture primitives
 * that must be drawn in background color
 * used to optimize screen refresh
 */
bool APERTURE_MACRO::HasNegativeItems( GERBER_DRAW_ITEM* aParent )
{
    for( AM_PRIMITIVES::iterator prim_macro = primitives.begin();
         prim_macro != primitives.end(); ++prim_macro )
    {
        if( prim_macro->mapExposure( aParent ) == false )   // = is negative
            return true;
    }

    return false;
}


/** GetShapeDim
 * Calculate a value that can be used to evaluate the size of text
 * when displaying the D-Code of an item
 * due to the complexity of a shape using many primitives
 * one cannot calculate the "size" of a shape (only abounding box)
 * but most of aperture macro are using one or few primitives
 * and the "dimension" of the shape is the diameter of the primitive
 * (or the max diameter of primitives)
 * @return a dimension, or -1 if no dim to calculate
 */
int APERTURE_MACRO::GetShapeDim( GERBER_DRAW_ITEM* aParent )
{
    int dim = -1;
    for( AM_PRIMITIVES::iterator prim_macro = primitives.begin();
         prim_macro != primitives.end(); ++prim_macro )
    {
        int pdim = prim_macro->GetShapeDim( aParent );
        if( dim < pdim )
            dim = pdim;
    }

    return dim;
}


/**
 * function GetLocalParam
 * Usually, parameters are defined inside the aperture primitive
 * using immediate mode or defered mode.
 * in defered mode the value is defined in a DCODE that want to use the aperture macro.
 * But some parameters are defined outside the aperture primitive
 * and are local to the aperture macro
 * @return the value of a defered parameter defined inside the aperture macro
 * @param aParamId = the param id (defined by $3 or $5 ..) to evaluate
 */
double APERTURE_MACRO::GetLocalParam( const D_CODE* aDcode, unsigned aParamId ) const
{
    // find parameter descr.
    const AM_PARAM * param = NULL;
    for( unsigned ii = 0; ii < m_localparamStack.size(); ii ++ )
    {
        if( m_localparamStack[ii].GetIndex() == aParamId )
        {
            param = &m_localparamStack[ii];
            break;
        }
    }
    if ( param == NULL )    // not found
        return 0.0;
    // Evaluate parameter
    double value = param->GetValue( aDcode );
    return value;
}