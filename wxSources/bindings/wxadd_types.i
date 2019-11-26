// ===========================================================================
// Purpose:     Additional wxWidgets storage types
// Author:      Toshi Nagata
// Created:     2019/10/19
// Copyright:   (c) 2019 Toshi Nagata. All rights reserved.
// Licence:     wxWidgets licence
// wxWidgets:   3.0.3
// ===========================================================================

#include "wx/dynarray.h"
#include "wx/buffer.h"

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

class %delete wxMemoryBuffer
{
public:
    // ctor and dtor
    wxMemoryBuffer(size_t size = wxMemoryBufferData::DefBufSize);

    // copy and assignment
    wxMemoryBuffer(const wxMemoryBuffer& src);

    // Accessors
    void  *GetData() const;
    size_t GetBufSize() const;
    size_t GetDataLen() const;

    bool IsEmpty() const;

    void   SetBufSize(size_t size);
    void   SetDataLen(size_t len);

    void Clear();

    // Ensure the buffer is big enough and return a pointer to it
    void *GetWriteBuf(size_t sizeNeeded);

    // Update the length after the write
    void  UngetWriteBuf(size_t sizeUsed);

    // Like the above, but appends to the buffer
    void *GetAppendBuf(size_t sizeNeeded);

    // Update the length after the append
    void  UngetAppendBuf(size_t sizeUsed);

    // Other ways to append to the buffer
    void  AppendByte(char data);

    void  AppendData(const void *data, size_t len);

    // gives up ownership of data, returns the pointer; after this call,
    // data isn't freed by the buffer and its content is resent to empty
    void *release();
    
    // Implement assignments
    void  AssignByte(int index, unsigned char data);
    void  AssignData(int index, unsigned char *data, size_t len);
    unsigned char Byte(int index);
};
