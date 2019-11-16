// ----------------------------------------------------------------------------
// Overridden functions for the wxWidgets binding for wxLua
//
// Please keep these functions in the same order as the .i file and in the
// same order as the listing of the functions in that file.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Overrides for wxgraphics.i
// ----------------------------------------------------------------------------

%override wxLua_wxArrayDouble_ToLuaTable
// int ToLuaTable() const
static int LUACALL wxLua_wxArrayDouble_ToLuaTable(lua_State *L)
{
    wxArrayDouble *doubleArray = (wxArrayDouble *)wxluaT_getuserdatatype(L, 1, wxluatype_wxArrayDouble);
    size_t idx, count = doubleArray->GetCount();
    lua_createtable(L, count, 0);
    for (idx = 0; idx < count; ++idx)
    {
        lua_pushnumber(L, (*doubleArray)[idx]);
        lua_rawseti(L, -2, idx + 1);
    }
    return 1;
}
%end

%override wxLua_wxAffineMatrix2DBase_Get
//  void Get() const
static int LUACALL wxLua_wxAffineMatrix2DBase_Get(lua_State *L)
{
    wxMatrix2D *mat2D = new wxMatrix2D();
    wxPoint2DDouble *tr = new wxPoint2DDouble();
    wxAffineMatrix2DBase *self = (wxAffineMatrix2DBase *)wxluaT_getuserdatatype(L, 1, wxluatype_wxAffineMatrix2DBase);
    self->Get(mat2D, tr);
    // add to tracked memory list
    wxluaO_addgcobject(L, mat2D, wxluatype_wxMatrix2D);
    wxluaO_addgcobject(L, tr, wxluatype_wxPoint2DDouble);
    // push the constructed class pointer
    wxluaT_pushuserdatatype(L, mat2D, wxluatype_wxMatrix2D);
    wxluaT_pushuserdatatype(L, tr, wxluatype_wxPoint2DDouble);
    return 2;
}
%end

%override wxLua_wxGraphicsMatrix_Get
//  virtual void Get() const;
static int LUACALL wxLua_wxGraphicsMatrix_Get(lua_State *L)
{
    wxDouble a[6];
    int idx;
    wxGraphicsMatrix *mat = (wxGraphicsMatrix *)wxluaT_getuserdatatype(L, 1, wxluatype_wxGraphicsMatrix);
    mat->Get(&a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
    lua_createtable(L, 6, 0);
    for (idx = 0; idx < 6; ++idx) {
        lua_pushnumber(L, a[idx]);
        lua_rawseti(L, -2, idx + 1);
    }
    return 1;
}
%end

%override wxLua_wxGraphicsMatrix_TransformPoint1
//     wxPoint2DDouble TransformPoint(wxPoint2DDouble point) const;
static int LUACALL wxLua_wxGraphicsMatrix_TransformPoint1(lua_State *L)
{
    // wxPoint2DDouble point
    wxPoint2DDouble point = *(wxPoint2DDouble*)wxluaT_getuserdatatype(L, 2, wxluatype_wxPoint2DDouble);
    // get this
    wxGraphicsMatrix * self = (wxGraphicsMatrix *)wxluaT_getuserdatatype(L, 1, wxluatype_wxGraphicsMatrix);
    // call TransformPoint
    self->TransformPoint(&point.m_x, &point.m_y);
    // allocate a new object using the copy constructor
    wxPoint2DDouble* returns = new wxPoint2DDouble(point);
    // add the new object to the tracked memory list
    wxluaO_addgcobject(L, returns, wxluatype_wxPoint2DDouble);
    // push the result datatype
    wxluaT_pushuserdatatype(L, returns, wxluatype_wxPoint2DDouble);
    return 1;
}
%end

%override wxLua_wxGraphicsMatrix_TransformPoint
//     wxPoint2DDouble TransformPoint(wxDouble x, wxDouble y) const;
static int LUACALL wxLua_wxGraphicsMatrix_TransformPoint(lua_State *L)
{
    // wxDouble y
    wxDouble y = (wxDouble)wxlua_getnumbertype(L, 3);
    // wxDouble x
    wxDouble x = (wxDouble)wxlua_getnumbertype(L, 2);
    // get this
    wxGraphicsMatrix * self = (wxGraphicsMatrix *)wxluaT_getuserdatatype(L, 1, wxluatype_wxGraphicsMatrix);
    // call TransformPoint
    self->TransformPoint(&x, &y);
    // allocate a new object
    wxPoint2DDouble* returns = new wxPoint2DDouble(x, y);
    // add the new object to the tracked memory list
    wxluaO_addgcobject(L, returns, wxluatype_wxPoint2DDouble);
    // push the result datatype
    wxluaT_pushuserdatatype(L, returns, wxluatype_wxPoint2DDouble);
    return 1;
}
%end

%override wxLua_wxGraphicsMatrix_TransformDistance1
//     wxPoint2DDouble TransformDistance(wxPoint2DDouble point) const;
static int LUACALL wxLua_wxGraphicsMatrix_TransformDistance1(lua_State *L)
{
    // wxPoint2DDouble point
    wxPoint2DDouble point = *(wxPoint2DDouble*)wxluaT_getuserdatatype(L, 2, wxluatype_wxPoint2DDouble);
    // get this
    wxGraphicsMatrix * self = (wxGraphicsMatrix *)wxluaT_getuserdatatype(L, 1, wxluatype_wxGraphicsMatrix);
    // call TransformDistance
    self->TransformDistance(&point.m_x, &point.m_y);
    // allocate a new object using the copy constructor
    wxPoint2DDouble* returns = new wxPoint2DDouble(point);
    // add the new object to the tracked memory list
    wxluaO_addgcobject(L, returns, wxluatype_wxPoint2DDouble);
    // push the result datatype
    wxluaT_pushuserdatatype(L, returns, wxluatype_wxPoint2DDouble);
    return 1;
}
%end

%override wxLua_wxGraphicsMatrix_TransformDistance
//     wxPoint2DDouble TransformDistance(wxDouble x, wxDouble y) const;
static int LUACALL wxLua_wxGraphicsMatrix_TransformDistance(lua_State *L)
{
    // wxDouble y
    wxDouble y = (wxDouble)wxlua_getnumbertype(L, 3);
    // wxDouble x
    wxDouble x = (wxDouble)wxlua_getnumbertype(L, 2);
    // get this
    wxGraphicsMatrix * self = (wxGraphicsMatrix *)wxluaT_getuserdatatype(L, 1, wxluatype_wxGraphicsMatrix);
    // call TransformDistance
    self->TransformDistance(&x, &y);
    // allocate a new object
    wxPoint2DDouble* returns = new wxPoint2DDouble(x, y);
    // add the new object to the tracked memory list
    wxluaO_addgcobject(L, returns, wxluatype_wxPoint2DDouble);
    // push the result datatype
    wxluaT_pushuserdatatype(L, returns, wxluatype_wxPoint2DDouble);
    return 1;
}
%end
