// ===========================================================================
// Purpose:     wxGraphics* classes
// Author:      Toshi Nagata
// Created:     2019/10/19
// Copyright:   (c) 2019 Toshi Nagata. All rights reserved.
// Licence:     wxWidgets licence
// wxWidgets:   3.0.3
// ===========================================================================

#include "wx/graphics.h"

enum wxAntialiasMode
{
    wxANTIALIAS_NONE,
    wxANTIALIAS_DEFAULT
};

enum wxInterpolationQuality
{
    wxINTERPOLATION_DEFAULT,
    wxINTERPOLATION_NONE, 
    wxINTERPOLATION_FAST,
    wxINTERPOLATION_GOOD,
    wxINTERPOLATION_BEST
};

enum wxCompositionMode
{
    wxCOMPOSITION_INVALID,
    wxCOMPOSITION_CLEAR,
    wxCOMPOSITION_SOURCE,
    wxCOMPOSITION_OVER,
    wxCOMPOSITION_IN,
    wxCOMPOSITION_OUT,
    wxCOMPOSITION_ATOP,
    wxCOMPOSITION_DEST,
    wxCOMPOSITION_DEST_OVER,
    wxCOMPOSITION_DEST_IN,
    wxCOMPOSITION_DEST_OUT,
    wxCOMPOSITION_DEST_ATOP,
    wxCOMPOSITION_XOR,
    wxCOMPOSITION_ADD
};

struct %delete wxMatrix2D
{
    wxMatrix2D(wxDouble v11 = 1,
               wxDouble v12 = 0,
               wxDouble v21 = 0,
               wxDouble v22 = 1);
    
    wxDouble m_11;
    wxDouble m_12;
    wxDouble m_21;
    wxDouble m_22;
};

// A 2x3 matrix representing an affine 2D transformation.
//
// This is an abstract base class implemented by wxAffineMatrix2D only so far,
// but in the future we also plan to derive wxGraphicsMatrix from it (it should
// also be documented then as currently only wxAffineMatrix2D itself is).
class %delete wxAffineMatrix2DBase
{
public:
//    wxAffineMatrix2DBase();    //  wxAffineMatrix2DBase is abstract
//    virtual ~wxAffineMatrix2DBase() {}
    
    // sets the matrix to the respective values
    virtual void Set(const wxMatrix2D& mat2D, const wxPoint2DDouble& tr);
    
    // gets the component valuess of the matrix
    // %override [wxMatrix2D mat2D, wxPoint2DDouble tr] Get();
    // C++ Func: virtual void Get(wxMatrix2D* mat2D, wxPoint2DDouble* tr) const;
    void Get() const;
    
    // concatenates the matrix
    virtual void Concat(const wxAffineMatrix2DBase& t);
    
    // makes this the inverse matrix
    virtual bool Invert();
    
    // return true if this is the identity matrix
    virtual bool IsIdentity() const;
    
    // returns true if the elements of the transformation matrix are equal ?
    virtual bool IsEqual(const wxAffineMatrix2DBase& t) const;
    bool operator==(const wxAffineMatrix2DBase& t) const;
    bool operator!=(const wxAffineMatrix2DBase& t) const;
    
    
    //
    // transformations
    //
    
    // add the translation to this matrix
    virtual void Translate(wxDouble dx, wxDouble dy);
    
    // add the scale to this matrix
    virtual void Scale(wxDouble xScale, wxDouble yScale);
    
    // add the rotation to this matrix (counter clockwise, radians)
    virtual void Rotate(wxDouble ccRadians);
    
    // add mirroring to this matrix
    void Mirror(int direction = wxHORIZONTAL);
    
    
    // applies that matrix to the point
    wxPoint2DDouble TransformPoint(const wxPoint2DDouble& src) const;
    
    void TransformPoint(wxDouble* x, wxDouble* y) const;
    
    // applies the matrix except for translations
    wxPoint2DDouble TransformDistance(const wxPoint2DDouble& src) const;
    
    void TransformDistance(wxDouble* dx, wxDouble* dy) const;
    
protected:
//    virtual wxPoint2DDouble DoTransformPoint(const wxPoint2DDouble& p) const;
//    virtual wxPoint2DDouble DoTransformDistance(const wxPoint2DDouble& p) const;
};

class %delete wxAffineMatrix2D : public wxAffineMatrix2DBase
{
   wxAffineMatrix2D();
};

class %delete wxArrayDouble
{
    wxArrayDouble();
    wxArrayDouble(const wxArrayDouble& array);
    
    // %override [Lua table] wxArrayDouble::ToLuaTable() const;
    // returns a table array of the doubles
    int ToLuaTable() const;
    
    void Add(double val);
    void Alloc(size_t count);
    void Clear();
    void Empty();
    int  GetCount() const;
    bool IsEmpty() const;
    int  Index(double val, bool searchFromEnd = false);
    void Insert(double val, size_t n, size_t copies = 1);
    double Item(size_t n);
    void Remove(double val);
    void RemoveAt(size_t index);
    void Shrink();
    
    double operator[](size_t nIndex);
};

class %delete wxGraphicsObject : public wxObject
{
public:
    wxGraphicsObject();
    wxGraphicsObject( wxGraphicsRenderer* renderer );
    //    virtual ~wxGraphicsObject();

    bool IsNull() const;

    // returns the renderer that was used to create this instance, or NULL if it has not been initialized yet
    wxGraphicsRenderer* GetRenderer() const;
//    wxGraphicsObjectRefData* GetGraphicsData() const;
protected:
//    virtual wxObjectRefData* CreateRefData() const;
//    virtual wxObjectRefData* CloneRefData(const wxObjectRefData* data) const;

    //    DECLARE_DYNAMIC_CLASS(wxGraphicsObject)
};

class %delete wxGraphicsPen : public wxGraphicsObject
{
public:
    wxGraphicsPen();
    //    virtual ~wxGraphicsPen() {}
private:
    //    DECLARE_DYNAMIC_CLASS(wxGraphicsPen)
};

#define_object wxGraphicsPen wxNullGraphicsPen

class %delete wxGraphicsBrush : public wxGraphicsObject
{
public:
    wxGraphicsBrush();
//    virtual ~wxGraphicsBrush() {}
    private:
//    DECLARE_DYNAMIC_CLASS(wxGraphicsBrush)
};
    
#define_object wxGraphicsBrush wxNullGraphicsBrush

class %delete wxGraphicsFont : public wxGraphicsObject
{
public:
    wxGraphicsFont();
//    virtual ~wxGraphicsFont() {}
private:
//    DECLARE_DYNAMIC_CLASS(wxGraphicsFont)
};

#define_object wxGraphicsFont wxNullGraphicsFont

class %delete wxGraphicsBitmap : public wxGraphicsObject
{
public:
    wxGraphicsBitmap()
//    virtual ~wxGraphicsBitmap() {}
    
    // Convert bitmap to wxImage: this is more efficient than converting to
    // wxBitmap first and then to wxImage and also works without X server
    // connection under Unix that wxBitmap requires.
#if wxUSE_IMAGE
    wxImage ConvertToImage() const;
#endif // wxUSE_IMAGE
    void* GetNativeBitmap() const;
//    const wxGraphicsBitmapData* GetBitmapData() const;
//    wxGraphicsBitmapData* GetBitmapData();
    
private:
//    DECLARE_DYNAMIC_CLASS(wxGraphicsBitmap)
};

#define_object wxGraphicsBitmap wxNullGraphicsBitmap

class %delete wxGraphicsMatrix : public wxGraphicsObject
{
public:
    wxGraphicsMatrix();
    
//    virtual ~wxGraphicsMatrix() {}
    
    // concatenates the matrix
    virtual void Concat( const wxGraphicsMatrix *t );
    void Concat( const wxGraphicsMatrix &t );
    
    // sets the matrix to the respective values
    virtual void Set(wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);
        
    // gets the component valuess of the matrix
    // %override [Lua table] Get();
    // C++ Func: virtual void Get(wxDouble* a=NULL, wxDouble* b=NULL,  wxDouble* c=NULL, wxDouble* d=NULL, wxDouble* tx=NULL, wxDouble* ty=NULL) const;
    virtual void Get() const;
                     
    // makes this the inverse matrix
    virtual void Invert();
    
    // returns true if the elements of the transformation matrix are equal ?
    virtual bool IsEqual( const wxGraphicsMatrix* t) const;
    bool IsEqual( const wxGraphicsMatrix& t) const;
    
    // return true if this is the identity matrix
    virtual bool IsIdentity() const;
    
    //
    // transformation
    //
    
    // add the translation to this matrix
    virtual void Translate( wxDouble dx , wxDouble dy );
    
    // add the scale to this matrix
    virtual void Scale( wxDouble xScale , wxDouble yScale );
    
    // add the rotation to this matrix (radians)
    virtual void Rotate( wxDouble angle );
    
    //
    // apply the transforms
    //
    
    // applies that matrix to the point
    // %override wxPoint2DDouble TransformPoint(wxDouble x, wxDouble y)
    // %override wxPoint2DDouble TransformPoint(wxPoint2DDouble point)
    //  C++ func: virtual void TransformPoint( wxDouble *x, wxDouble *y ) const;
    wxPoint2DDouble TransformPoint(wxDouble x, wxDouble y) const;
    wxPoint2DDouble TransformPoint(wxPoint2DDouble point) const;
    
    // applies the matrix except for translations
    // %override wxPoint2DDouble TransformDistance(wxDouble x, wxDouble y)
    // %override wxPoint2DDouble TransformDistance(wxPoint2DDouble point)
    // C++ func: void TransformDistance( wxDouble *dx, wxDouble *dy ) const;
    wxPoint2DDouble TransformDistance(wxDouble x, wxDouble y) const;
    wxPoint2DDouble TransformDistance(wxPoint2DDouble point) const;
    
    // returns the native representation
    virtual void * GetNativeMatrix() const;
    
//    const wxGraphicsMatrixData* GetMatrixData() const
//    { return (const wxGraphicsMatrixData*) GetRefData(); }
//    wxGraphicsMatrixData* GetMatrixData()
//    { return (wxGraphicsMatrixData*) GetRefData(); }
    
//    private:
//    DECLARE_DYNAMIC_CLASS(wxGraphicsMatrix)
};

#define_object wxGraphicsMatrix wxNullGraphicsMatrix

class %delete wxGraphicsPath : public wxGraphicsObject
{
public:
    wxGraphicsPath();
//    virtual ~wxGraphicsPath() {}
    
    //
    // These are the path primitives from which everything else can be constructed
    //
    
    // begins a new subpath at (x,y)
    virtual void MoveToPoint( wxDouble x, wxDouble y );
    void MoveToPoint( const wxPoint2DDouble& p);
    
    // adds a straight line from the current point to (x,y)
    virtual void AddLineToPoint( wxDouble x, wxDouble y );
    void AddLineToPoint( const wxPoint2DDouble& p);
    
    // adds a cubic Bezier curve from the current point, using two control points and an end point
    virtual void AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y );
    void AddCurveToPoint( const wxPoint2DDouble& c1, const wxPoint2DDouble& c2, const wxPoint2DDouble& e);
    
    // adds another path
    virtual void AddPath( const wxGraphicsPath& path );
    
    // closes the current sub-path
    virtual void CloseSubpath();
    
    // gets the last point of the current path, (0,0) if not yet set
    virtual void GetCurrentPoint( wxDouble* x, wxDouble* y) const;
    wxPoint2DDouble GetCurrentPoint() const;
    
    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    virtual void AddArc( wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise );
    void AddArc( const wxPoint2DDouble& c, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise);
    
    //
    // These are convenience functions which - if not available natively will be assembled
    // using the primitives from above
    //
    
    // adds a quadratic Bezier curve from the current point, using a control point and an end point
    virtual void AddQuadCurveToPoint( wxDouble cx, wxDouble cy, wxDouble x, wxDouble y );
    
    // appends a rectangle as a new closed subpath
    virtual void AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    
    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    virtual void AddCircle( wxDouble x, wxDouble y, wxDouble r );
    
    // appends a an arc to two tangents connecting (current) to (x1,y1) and (x1,y1) to (x2,y2), also a straight line from (current) to (x1,y1)
    virtual void AddArcToPoint( wxDouble x1, wxDouble y1 , wxDouble x2, wxDouble y2, wxDouble r );
    
    // appends an ellipse
    virtual void AddEllipse( wxDouble x, wxDouble y, wxDouble w, wxDouble h);
    
    // appends a rounded rectangle
    virtual void AddRoundedRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius);
    
    // returns the native path
    virtual void * GetNativePath() const;
    
    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    virtual void UnGetNativePath(void *p)const;
    
    // transforms each point of this path by the matrix
    virtual void Transform( const wxGraphicsMatrix& matrix );
    
    // gets the bounding box enclosing all points (possibly including control points)
    virtual void GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h)const;
    wxRect2DDouble GetBox()const;
    
    virtual bool Contains( wxDouble x, wxDouble y, wxPolygonFillMode fillStyle = wxODDEVEN_RULE)const;
    bool Contains( const wxPoint2DDouble& c, wxPolygonFillMode fillStyle = wxODDEVEN_RULE)const;
    
//    const wxGraphicsPathData* GetPathData() const
//    { return (const wxGraphicsPathData*) GetRefData(); }
//    wxGraphicsPathData* GetPathData()
//    { return (wxGraphicsPathData*) GetRefData(); }
    
//    private:
//    DECLARE_DYNAMIC_CLASS(wxGraphicsPath)
};

#define_object wxGraphicsPath wxNullGraphicsPath

// Describes a single gradient stop.
class wxGraphicsGradientStop
{
public:
    wxGraphicsGradientStop(wxColour col = wxTransparentColour, float pos = 0.);
    
    // default copy ctor, assignment operator and dtor are ok
    
    const wxColour& GetColour() const;
    void SetColour(const wxColour& col);
    
    float GetPosition() const;
    void SetPosition(float pos);
    
private:
    // The colour of this gradient band.
    wxColour m_col;
    
    // Its starting position: 0 is the beginning and 1 is the end.
    float m_pos;
};

// A collection of gradient stops ordered by their positions (from lowest to
// highest). The first stop (index 0, position 0.0) is always the starting
// colour and the last one (index GetCount() - 1, position 1.0) is the end
// colour.
class %delete wxGraphicsGradientStops
{
public:
    wxGraphicsGradientStops(wxColour startCol = wxTransparentColour,
                            wxColour endCol = wxTransparentColour);
    
    // default copy ctor, assignment operator and dtor are ok for this class
    
    
    // Add a stop in correct order.
    void Add(const wxGraphicsGradientStop& stop);
    void Add(wxColour col, float pos);
    
    // Get the number of stops.
    size_t GetCount() const;
    
    // Return the stop at the given index (which must be valid).
    wxGraphicsGradientStop Item(unsigned int n) const;
    
    // Get/set start and end colours.
    void SetStartColour(wxColour col);
    wxColour GetStartColour() const;
    void SetEndColour(wxColour col);
    wxColour GetEndColour() const;
        
//        private:
    // All the stops stored in ascending order of positions.
//    wxVector<wxGraphicsGradientStop> m_stops;
};

class %delete wxGraphicsContext : public wxGraphicsObject
{
public:
//    wxGraphicsContext(wxGraphicsRenderer* renderer);
    
//    virtual ~wxGraphicsContext();
    
    static %gc wxGraphicsContext* Create( const wxWindowDC& dc);
    static %gc wxGraphicsContext * Create( const wxMemoryDC& dc);
#if wxUSE_PRINTING_ARCHITECTURE
    static %gc wxGraphicsContext * Create( const wxPrinterDC& dc);
#endif
#if defined(__WXMSW__)
#if wxUSE_ENH_METAFILE
    // static %gc wxGraphicsContext * Create( const wxEnhMetaFileDC& dc);
#endif
#endif
    
    static %gc wxGraphicsContext* CreateFromNative( void * context );
    
    static %gc wxGraphicsContext* CreateFromNativeWindow( void * window );
    
    static %gc wxGraphicsContext* Create( wxWindow* window );
    
#if wxUSE_IMAGE
    // Create a context for drawing onto a wxImage. The image life time must be
    // greater than that of the context itself as when the context is destroyed
    // it will copy its contents to the specified image.
    static %gc wxGraphicsContext* Create(wxImage& image);
#endif // wxUSE_IMAGE
    
    // create a context that can be used for measuring texts only, no drawing allowed
    static %gc wxGraphicsContext * Create();
    
    // begin a new document (relevant only for printing / pdf etc) if there is a progress dialog, message will be shown
    virtual bool StartDoc( const wxString& message );
    
    // done with that document (relevant only for printing / pdf etc)
    virtual void EndDoc();
    
    // opens a new page  (relevant only for printing / pdf etc) with the given size in points
    // (if both are null the default page size will be used)
    virtual void StartPage( wxDouble width = 0, wxDouble height = 0 );
    
    // ends the current page  (relevant only for printing / pdf etc)
    virtual void EndPage();
    
    // make sure that the current content of this context is immediately visible
    virtual void Flush();
    
    wxGraphicsPath CreatePath() const;
    
    virtual wxGraphicsPen CreatePen(const wxPen& pen) const;
    
    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush ) const;
    
    // sets the brush to a linear gradient, starting at (x1,y1) and ending at
    // (x2,y2) with the given boundary colours or the specified stops
    wxGraphicsBrush CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                              wxDouble x2, wxDouble y2,
                              const wxColour& c1, const wxColour& c2) const;
    wxGraphicsBrush CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                              wxDouble x2, wxDouble y2,
                              const wxGraphicsGradientStops& stops) const;
                              
    // sets the brush to a radial gradient originating at (xo,yc) and ending
    // on a circle around (xc,yc) with the given radius; the colours may be
    // specified by just the two extremes or the full array of gradient stops
    wxGraphicsBrush CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                              wxDouble xc, wxDouble yc, wxDouble radius,
                              const wxColour& oColor, const wxColour& cColor) const;
                              
    wxGraphicsBrush CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                              wxDouble xc, wxDouble yc, wxDouble radius,
                              const wxGraphicsGradientStops& stops) const;
                              
    // creates a font
    // NOTE: wxBLACK_INSTANCE is #define'd to (*wxBLACK)
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = wxBLACK_INSTANCE ) const;
    virtual wxGraphicsFont CreateFont(double sizeInPixels,
                                      const wxString& facename,
                                      int flags = wxFONTFLAG_DEFAULT,
                                      const wxColour& col = wxBLACK_INSTANCE) const;
                                      
    // create a native bitmap representation
    virtual wxGraphicsBitmap CreateBitmap( const wxBitmap &bitmap ) const;
#if wxUSE_IMAGE
    wxGraphicsBitmap CreateBitmapFromImage(const wxImage& image) const;
#endif // wxUSE_IMAGE
    
    // create a native bitmap representation
    virtual wxGraphicsBitmap CreateSubBitmap( const wxGraphicsBitmap &bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h  ) const;
    
    // create a 'native' matrix corresponding to these values
    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0) const;
        
    wxGraphicsMatrix CreateMatrix( const wxAffineMatrix2DBase& mat ) const;
    
    // push the current state of the context, ie the transformation matrix on a stack
    virtual void PushState();
    
    // pops a stored state from the stack
    virtual void PopState();
    
    // clips drawings to the region intersected with the current clipping region
    virtual void Clip( const wxRegion &region );
    
    // clips drawings to the rect intersected with the current clipping region
    virtual void Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    
    // resets the clipping to original extent
    virtual void ResetClip();
    
    // returns the native context
    virtual void * GetNativeContext();
    
    // returns the current shape antialiasing mode
    virtual wxAntialiasMode GetAntialiasMode() const;
    
    // sets the antialiasing mode, returns true if it supported
    virtual bool SetAntialiasMode(wxAntialiasMode antialias);
    
    // returns the current interpolation quality
    virtual wxInterpolationQuality GetInterpolationQuality() const;
    
    // sets the interpolation quality, returns true if it supported
    virtual bool SetInterpolationQuality(wxInterpolationQuality interpolation);
    
    // returns the current compositing operator
    virtual wxCompositionMode GetCompositionMode() const;
    
    // sets the compositing operator, returns true if it supported
    virtual bool SetCompositionMode(wxCompositionMode op);
    
    // returns the size of the graphics context in device coordinates
    void GetSize(wxDouble* width, wxDouble* height) const;
    
    // returns the resolution of the graphics context in device points per inch
    virtual void GetDPI( wxDouble* dpiX, wxDouble* dpiY);
    
#if 0
    // sets the current alpha on this context
    virtual void SetAlpha( wxDouble alpha );
    
    // returns the alpha on this context
    virtual wxDouble GetAlpha() const;
#endif
    
    // all rendering is done into a fully transparent temporary context
    virtual void BeginLayer(wxDouble opacity);
    
    // composites back the drawings into the context with the opacity given at
    // the BeginLayer call
    virtual void EndLayer();
    
    //
    // transformation : changes the current transformation matrix CTM of the context
    //
    
    // translate
    virtual void Translate( wxDouble dx , wxDouble dy );
    
    // scale
    virtual void Scale( wxDouble xScale , wxDouble yScale );
    
    // rotate (radians)
    virtual void Rotate( wxDouble angle );
    
    // concatenates this transform with the current transform of this context
    virtual void ConcatTransform( const wxGraphicsMatrix& matrix );
    
    // sets the transform of this context
    virtual void SetTransform( const wxGraphicsMatrix& matrix );
    
    // gets the matrix of this context
    virtual wxGraphicsMatrix GetTransform() const;
    //
    // setting the paint
    //
    
    // sets the pen
    virtual void SetPen( const wxGraphicsPen& pen );
    
    void SetPen( const wxPen& pen );
    
    // sets the brush for filling
    virtual void SetBrush( const wxGraphicsBrush& brush );
    
    void SetBrush( const wxBrush& brush );
    
    // sets the font
    virtual void SetFont( const wxGraphicsFont& font );
    
    void SetFont( const wxFont& font, const wxColour& colour );
    
    
    // strokes along a path with the current pen
    virtual void StrokePath( const wxGraphicsPath& path );
    
    // fills a path with the current brush
    virtual void FillPath( const wxGraphicsPath& path, wxPolygonFillMode fillStyle = wxODDEVEN_RULE );
    
    // draws a path by first filling and then stroking
    virtual void DrawPath( const wxGraphicsPath& path, wxPolygonFillMode fillStyle = wxODDEVEN_RULE );
    
    //
    // text
    //
    
    void DrawText( const wxString &str, wxDouble x, wxDouble y );
        
    void DrawText( const wxString &str, wxDouble x, wxDouble y, wxDouble angle );
        
    void DrawText( const wxString &str, wxDouble x, wxDouble y,
                   const wxGraphicsBrush& backgroundBrush );
        
    void DrawText( const wxString &str, wxDouble x, wxDouble y,
                   wxDouble angle, const wxGraphicsBrush& backgroundBrush );
        
        
    virtual void GetTextExtent( const wxString &text, wxDouble *width, wxDouble *height,
        wxDouble *descent = NULL, wxDouble *externalLeading = NULL ) const;
        
    virtual void GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const;
    
    //
    // image support
    //
    
    virtual void DrawBitmap( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    
    virtual void DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    
    virtual void DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    
    //
    // convenience methods
    //
    
    // strokes a single line
    virtual void StrokeLine( wxDouble x1, wxDouble y1, wxDouble x2, wxDouble y2);
    
    // stroke lines connecting each of the points
    virtual void StrokeLines( size_t n, const wxPoint2DDouble *points);
    
    // stroke disconnected lines from begin to end points
    virtual void StrokeLines( size_t n, const wxPoint2DDouble *beginPoints, const wxPoint2DDouble *endPoints);
    
    // draws a polygon
    virtual void DrawLines( size_t n, const wxPoint2DDouble *points, wxPolygonFillMode fillStyle = wxODDEVEN_RULE );
    
    // draws a rectangle
    virtual void DrawRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h);
    
    // draws an ellipse
    virtual void DrawEllipse( wxDouble x, wxDouble y, wxDouble w, wxDouble h);
    
    // draws a rounded rectangle
    virtual void DrawRoundedRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius);
    
     // wrappers using wxPoint2DDouble TODO
     
    // helper to determine if a 0.5 offset should be applied for the drawing operation
    virtual bool ShouldOffset() const;
    
    // indicates whether the context should try to offset for pixel boundaries, this only makes sense on 
    // bitmap devices like screen, by default this is turned off
    virtual void EnableOffset(bool enable = true);
    
    void DisableOffset();
    bool OffsetEnabled();
    
    protected:
    // These fields must be initialized in the derived class ctors.
    wxDouble m_width,
             m_height;
             
    wxGraphicsPen m_pen;
    wxGraphicsBrush m_brush;
    wxGraphicsFont m_font;
    wxAntialiasMode m_antialias;
    wxCompositionMode m_composition;
    wxInterpolationQuality m_interpolation;
    bool m_enableOffset;
    
    protected:
    // implementations of overloaded public functions: we use different names
    // for them to avoid the virtual function hiding problems in the derived
    // classes
    virtual void DoDrawText(const wxString& str, wxDouble x, wxDouble y);
    virtual void DoDrawRotatedText(const wxString& str, wxDouble x, wxDouble y,
                                   wxDouble angle);
    virtual void DoDrawFilledText(const wxString& str, wxDouble x, wxDouble y,
                                  const wxGraphicsBrush& backgroundBrush);
    virtual void DoDrawRotatedFilledText(const wxString& str,
                                         wxDouble x, wxDouble y,
                                         wxDouble angle,
                                         const wxGraphicsBrush& backgroundBrush);
                                         
//    wxDECLARE_NO_COPY_CLASS(wxGraphicsContext);
//    DECLARE_ABSTRACT_CLASS(wxGraphicsContext)
};

class %delete wxGraphicsRenderer : public wxObject
{
public:
//    wxGraphicsRenderer();
    
//    virtual ~wxGraphicsRenderer() {}
    
    static wxGraphicsRenderer* GetDefaultRenderer();
    
//    static wxGraphicsRenderer* GetCairoRenderer();
    // Context
    virtual %gc wxGraphicsContext * CreateContext( const wxWindowDC& dc);
    virtual %gc wxGraphicsContext * CreateContext( const wxMemoryDC& dc);
    #if wxUSE_PRINTING_ARCHITECTURE
    virtual %gc wxGraphicsContext * CreateContext( const wxPrinterDC& dc);
    #endif
    #if defined(__WXMSW__)
    #if wxUSE_ENH_METAFILE
//    virtual %gc wxGraphicsContext * CreateContext( const wxEnhMetaFileDC& dc) = 0;
    #endif
    #endif
    
    virtual %gc wxGraphicsContext * CreateContextFromNativeContext( void * context );
    
    virtual %gc wxGraphicsContext * CreateContextFromNativeWindow( void * window );
    
    virtual %gc wxGraphicsContext * CreateContext( wxWindow* window );
    
    #if wxUSE_IMAGE
    virtual %gc wxGraphicsContext * CreateContextFromImage(wxImage& image);
    #endif // wxUSE_IMAGE
    
    // create a context that can be used for measuring texts only, no drawing allowed
    virtual %gc wxGraphicsContext * CreateMeasuringContext();
    
    // Path
    
    virtual wxGraphicsPath CreatePath();
    
    // Matrix
    
    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);
        
    // Paints
    
    virtual wxGraphicsPen CreatePen(const wxPen& pen);
    
    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush );
    
    // Gradient brush creation functions may not honour all the stops specified
    // stops and use just its boundary colours (this is currently the case
    // under OS X)
    virtual wxGraphicsBrush CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                              wxDouble x2, wxDouble y2,
                              const wxGraphicsGradientStops& stops);
                              
    virtual wxGraphicsBrush CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                              wxDouble xc, wxDouble yc,
                              wxDouble radius,
                              const wxGraphicsGradientStops& stops);
                              
    // sets the font
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = wxBLACK_INSTANCE );
    virtual wxGraphicsFont CreateFont(double sizeInPixels,
                                      const wxString& facename,
                                      int flags = wxFONTFLAG_DEFAULT,
                                      const wxColour& col = wxBLACK_INSTANCE);
                                      
    // create a native bitmap representation
    virtual wxGraphicsBitmap CreateBitmap( const wxBitmap &bitmap );
    #if wxUSE_IMAGE
    virtual wxGraphicsBitmap CreateBitmapFromImage(const wxImage& image);
    virtual wxImage CreateImageFromBitmap(const wxGraphicsBitmap& bmp);
    #endif // wxUSE_IMAGE
    
    // create a graphics bitmap from a native bitmap
    virtual wxGraphicsBitmap CreateBitmapFromNativeBitmap( void* bitmap );
    
    // create a subimage from a native image representation
    virtual wxGraphicsBitmap CreateSubBitmap( const wxGraphicsBitmap &bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h  );
    
    private:
//    wxDECLARE_NO_COPY_CLASS(wxGraphicsRenderer);
//    DECLARE_ABSTRACT_CLASS(wxGraphicsRenderer)
};

