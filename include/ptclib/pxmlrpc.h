/*
 * pxmlrpc.h
 *
 * XML/RPC support
 *
 * Portable Tools Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_PXMLRPC_H
#define PTLIB_PXMLRPC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/pxml.h>

#if P_XMLRPC

#include <ptclib/url.h>


class PXMLRPCBlock;
class PXMLRPCVariableBase;
class PXMLRPCStructBase;


/////////////////////////////////////////////////////////////////

class PXMLRPC : public PObject
{
    PCLASSINFO(PXMLRPC, PObject);
  public:
    enum {
      CannotCreateRequestXML          = 100,
      CannotParseResponseXML,
      CannotParseRequestXML,
      HTTPPostFailed,
      CannotReadResponseContentBody,
      ResponseRootNotMethodResponse,
      ResponseEmpty,
      ResponseUnknownFormat,
      ParamNotValue,
      ScalarWithoutElement,
      ParamNotStruct,
      MemberIncomplete,
      MemberUnnamed,
      FaultyFault,
      RequestHasWrongDocumentType,
      RequestHasNoMethodName,
      RequestHasNoParms,
      MethodNameIsEmpty,
      UnknownMethod,
      ParamNotArray,

      UserFault                       = 1000,
    };

    PXMLRPC(
      const PURL & url,
      PXML::Options options = PXML::NoOptions
    );

    void SetTimeout(const PTimeInterval & t) { m_timeout = t; }

    bool MakeRequest(const PString & method);
    bool MakeRequest(const PString & method,  PXMLRPCBlock & response);
    bool MakeRequest(PXMLRPCBlock & request, PXMLRPCBlock & response);
    bool MakeRequest(const PString & method, const PXMLRPCStructBase & args, PXMLRPCStructBase & reply);

    PString GetFaultText() const { return m_faultText; }
    PINDEX  GetFaultCode() const { return m_faultCode; }

    static bool    ISO8601ToPTime(const PString & iso8601, PTime & val, int tz = PTime::GMT);
    static PString PTimeToISO8601(const PTime & val);

  protected:
    bool PerformRequest(PXMLRPCBlock & request, PXMLRPCBlock & response);

    PURL          m_url;
    PINDEX        m_faultCode;
    PString       m_faultText;
    PTimeInterval m_timeout;
    PXML::Options m_options;
};

/////////////////////////////////////////////////////////////////

class PXMLRPCBlock : public PXML
{
    PCLASSINFO(PXMLRPCBlock, PXML);
  public:
    PXMLRPCBlock();
    PXMLRPCBlock(const PString & method);
    PXMLRPCBlock(const PString & method, const PXMLRPCStructBase & structData);

    PXMLElement * GetParams();
    PXMLElement * GetParam(PINDEX idx) const;
    PINDEX GetParamCount() const;

    // used when used as a response
    PINDEX  GetFaultCode() const                     { return m_faultCode; }
    PString GetFaultText() const                     { return m_faultText; }
    void SetFault(PINDEX code, const PString & text) { m_faultCode = code; m_faultText = text; }
    bool ValidateResponse();

    // helper functions for getting parameters
    bool GetParams(PXMLRPCStructBase & data);
    bool GetParam(PINDEX idx, PString & type, PString & result);
    bool GetExpectedParam(PINDEX idx, const PString & expectedType, PString & value);

    bool GetParam(PINDEX idx, PString & result);
    bool GetParam(PINDEX idx, int & result);
    bool GetParam(PINDEX idx, double & result);
    bool GetParam(PINDEX idx, PTime & result, int tz = PTime::GMT);
    bool GetParam(PINDEX idx, PStringToString & result);
    bool GetParam(PINDEX idx, PXMLRPCStructBase & result);
    bool GetParam(PINDEX idx, PStringArray & result);
    bool GetParam(PINDEX idx, PArray<PStringToString> & result);

    // functions for parsing values
    bool ParseScalar(PXMLElement * element, PString & type, PString & value);
    bool ParseStruct(PXMLElement * element, PStringToString & structDict);
    bool ParseStruct(PXMLElement * element, PXMLRPCStructBase & structData);
    bool ParseArray(PXMLElement * element, PStringArray & array);
    bool ParseArray(PXMLElement * element, PArray<PStringToString> & array);
    bool ParseArray(PXMLElement * element, PXMLRPCVariableBase & array);

    // functions for creating values
    PXMLElement * CreateValueElement(PXMLElement * element);
    PXMLElement * CreateScalar(const PString & type, const PString & scalar);
    PXMLElement * CreateMember(const PString & name, PXMLElement * value);

    PXMLElement * CreateScalar(const PString & str);
    PXMLElement * CreateScalar(long value);
    PXMLElement * CreateScalar(double value);
    PXMLElement * CreateDateAndTime(const PTime & time);
    PXMLElement * CreateBinary(const PBYTEArray & data);

    PXMLElement * CreateStruct(PXMLElement * & data);
    PXMLElement * CreateStruct(const PStringToString & dict);
    PXMLElement * CreateStruct(const PStringToString & dict, const PString & typeStr);
    PXMLElement * CreateStruct(const PXMLRPCStructBase & structData);

    PXMLElement * CreateArray(PXMLElement * & data);
    PXMLElement * CreateArray(const PStringArray & array);
    PXMLElement * CreateArray(const PStringArray & array, const PString & typeStr);
    PXMLElement * CreateArray(const PStringArray & array, const PStringArray & types);
    PXMLElement * CreateArray(const PArray<PStringToString> & array);
    PXMLElement * CreateArray(const PXMLRPCVariableBase & array);

    // helper functions for adding parameters
    void AddParam(PXMLElement * parm);
    void AddParam(const PString & str);
    void AddParam(long value);
    void AddParam(double value);
    void AddParam(const PTime & time);
    void AddParam(const PXMLRPCStructBase & structData);
    void AddBinary(const PBYTEArray & data);
    void AddStruct(const PStringToString & dict);
    void AddStruct(const PStringToString & dict, const PString & typeStr);
    void AddArray(const PStringArray & array);
    void AddArray(const PStringArray & array, const PString & typeStr);
    void AddArray(const PStringArray & array, const PStringArray & types);
    void AddArray(const PArray<PStringToString> & array);

  protected:
    PXMLElement * m_params;
    PString       m_faultText;
    PINDEX        m_faultCode;
};


/////////////////////////////////////////////////////////////////

class PXMLRPCVariableBase : public PObject, PNonCopyable
{
    PCLASSINFO(PXMLRPCVariableBase, PObject);
  protected:
    PXMLRPCVariableBase(const char * name, const char * type = NULL);

  public:
    const char * GetName() const { return m_name; }
    const char * GetType() const { return m_type; }

    virtual void Copy(const PXMLRPCVariableBase & other) = 0;
    virtual PString ToString(PINDEX i) const;
    virtual void FromString(PINDEX i, const PString & str);
    virtual PXMLRPCStructBase * GetStruct(PINDEX i) const;
    virtual bool IsArray() const;
    virtual PINDEX GetSize() const;
    virtual bool SetSize(PINDEX);

    template <typename T> PString ToBase64(PScalarArray<T> & data) const
    {
      return PBase64::Encode(data.GetPointer(), data.GetSize());
    }

    template <typename T> void FromBase64(const PString & str, PScalarArray<T> & data)
    {
      PBase64 decoder;
      decoder.StartDecoding();
      decoder.ProcessDecoding(str);
      data = decoder.GetDecodedData();
    }


  protected:
    const char * m_name;
    const char * m_type;
};


template <class T> class PXMLRPCArrayBase : public PXMLRPCVariableBase {
    PCLASSINFO(PXMLRPCArrayBase, PXMLRPCVariableBase);
  protected:
    PXMLRPCArrayBase(T & array, const char * name, const char * type)
      : PXMLRPCVariableBase(name, type)
      , m_array(array)
    { }

  public:
    virtual void PrintOn(ostream & strm) const override
    {
      strm << setfill('\n') << m_array << setfill(' ');
    }

    virtual void Copy(const PXMLRPCVariableBase & other) override
    {
      m_array = dynamic_cast<const PXMLRPCArrayBase &>(other).m_array;
    }

    virtual bool IsArray() const override
    {
      return true;
    }

    virtual PINDEX GetSize() const override
    {
      return m_array.GetSize();
    }

    virtual bool SetSize(PINDEX sz) override
    {
      m_array.SetSize(sz);
      return true;
    }

  protected:
    T & m_array;
};


template <typename T> class PXMLRPCArrayObjectsBase : public PXMLRPCArrayBase<PArray<T>>
{
    PCLASSINFO(PXMLRPCArrayObjectsBase, PXMLRPCArrayBase);
  protected:
    PXMLRPCArrayObjectsBase(PArray<T> & array, const char * name, const char * type)
      : PXMLRPCArrayBase<PArray<T>>(array, name, type)
    { }
    PXMLRPCArrayObjectsBase & operator=(const PXMLRPCArrayObjectsBase &) = delete;

  public:
    virtual PString ToString(PINDEX i) const override
    {
      PStringStream stream;
      stream << *this->m_array.GetAt(i);
      return stream;
    }

    virtual void FromString(PINDEX i, const PString & str) override
    {
      auto object = this->m_array.GetAt(i);
      if (!object)
        object = this->m_array.SetAt(i, CreateObject());

      PStringStream stream(str);
      stream >> *object;
    }

    virtual bool SetSize(PINDEX sz) override
    {
      if (!this->m_array.SetSize(sz))
        return false;

      for (PINDEX i = 0; i < sz; i++) {
        if (this->m_array.GetAt(i) == NULL) {
          auto object = CreateObject();
          if (object == NULL)
            return false;
          this->m_array.SetAt(i, object);
        }
      }

      return true;
    }

    virtual T * CreateObject() const = 0;
};


class PXMLRPCStructBase : public PObject, PNonCopyable
{
    PCLASSINFO(PXMLRPCStructBase, PObject);
  protected:
    PXMLRPCStructBase();
    PXMLRPCStructBase & operator=(const PXMLRPCStructBase &);

  public:
    void PrintOn(ostream & strm) const override;

    PINDEX GetNumVariables() const { return variablesByOrder.GetSize(); }
    PXMLRPCVariableBase & GetVariable(PINDEX idx) const { return variablesByOrder[idx]; }
    auto GetVariable(const char * name) const { return variablesByName.GetAt(name); }

    void AddVariable(PXMLRPCVariableBase * var);
    static PXMLRPCStructBase & GetInitialiser();

  protected:
    void EndConstructor();

    PArray<PXMLRPCVariableBase>               variablesByOrder;
    PDictionary<PString, PXMLRPCVariableBase> variablesByName;

    PXMLRPCStructBase * initialiserStack;
};


#define PXMLRPC_STRUCT_BEGIN(name) \
  class name : public PXMLRPCStructBase { \
    public: name() { EndConstructor(); } \
    public: name(const name & other) { EndConstructor(); operator=(other); } \
    public: name & operator=(const name & other) { PXMLRPCStructBase::operator=(other); return *this; }

#define PXMLRPC_VARIABLE_CLASS(base, type, variable, xmltype, init, extras) \
    private: struct PXMLRPCVar_##variable : public PXMLRPCVariableBase { \
      PXMLRPCVar_##variable() \
        : PXMLRPCVariableBase(#variable, xmltype), \
          instance(((base &)base::GetInitialiser()).variable) \
        { init } \
      virtual void PrintOn (ostream & s) const { s << instance; } \
      virtual void ReadFrom(istream & s)       { s >> instance; } \
      virtual void Copy(const PXMLRPCVariableBase & other) \
                    { instance = ((PXMLRPCVar_##variable &)other).instance; } \
      extras \
      type & instance; \
    } pxmlrpcvar_##variable

#define PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, init, extras) \
    public: type variable; \
    PXMLRPC_VARIABLE_CLASS(base, type, variable, xmltype, init, extras)

#define PXMLRPC_ARRAY_CUSTOM(base, arraytype, basetype, variable, xmltype, par, extras) \
    public: arraytype variable; \
    private: struct PXMLRPCVar_##variable : public par { \
      PXMLRPCVar_##variable() \
        : par(((base &)base::GetInitialiser()).variable, #variable, xmltype), \
          instance((arraytype &)m_array) \
        { } \
      extras \
      arraytype & instance; \
    } pxmlrpcvar_##variable
#ifdef DOCPLUSPLUS
}
#endif


#define PXMLRPC_STRUCT_END() \
  };


#define PXMLRPC_VARIABLE(base, type, variable, xmltype) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, ;, ;)


#define PXMLRPC_VARIABLE_INIT(base, type, variable, xmltype, init) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, instance=init;, ;)


#define PXMLRPC_STRING(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "string")


#define PXMLRPC_STRING_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "string", init)


#define PXMLRPC_INTEGER(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "int")


#define PXMLRPC_INTEGER_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "int", init)


#define PXMLRPC_BOOLEAN(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "boolean")


#define PXMLRPC_BOOLEAN_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "boolean", init)


#define PXMLRPC_DOUBLE(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "double")


#define PXMLRPC_DOUBLE_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "double", init)


#define PXMLRPC_DATETIME(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "dateTime.iso8601", ;, \
                    PString ToString(PINDEX) const { return instance.AsString(PTime::ShortISO8601); } )


#define PXMLRPC_BINARY(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "base64", ;, \
                    PString ToString(PINDEX) const { return ToBase64(instance); } \
                    void FromString(PINDEX, const PString & str) { FromBase64(str, instance); } )


#define PXMLRPC_STRUCT(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "struct", ;, \
                    PXMLRPCStructBase * GetStruct(PINDEX) const { return &instance; } )

#define PXMLRPC_ARRAY_STRING(base, arraytype, basetype, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, arraytype, basetype, variable, "string", PXMLRPCArrayBase<PStringArray>, )

#define PXMLRPC_ARRAY_INTEGER(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PScalarArray<type>, type, variable, "int", PXMLRPCArrayBase<PScalarArray<type>>, \
                    PString ToString(PINDEX i) const { return PString(instance[i]); } \
                    void FromString(PINDEX i, const PString & str) { instance[i] = (type)str.AsInteger(); })

#define PXMLRPC_ARRAY_DOUBLE(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PScalarArray<type>, type, variable, "double", PXMLRPCArrayBase<PScalarArray<type>>, \
                    PString ToString(PINDEX i) const { return psprintf("%f", instance[i]); } \
                    void FromString(PINDEX i, const PString & str) { instance[i] = (type)str.AsReal(); })

#define PXMLRPC_ARRAY_STRUCT(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PArray<type>, type, variable, "struct", PXMLRPCArrayObjectsBase<type>, \
                             PXMLRPCStructBase * GetStruct(PINDEX i) const { return &instance[i]; } \
                             type * CreateObject() const { return new type; })


#define PXMLRPC_FUNC_NOARG_NOREPLY(name) \
  bool name() { return MakeRequest(#name); }


#define PXMLRPC_FUNC_SINGLE_ARG(name, vartype, argtype) \
  class name##_in : public PXMLRPCStructBase { \
    public: name##_in(const argtype & var) : variable(var) { EndConstructor(); } \
        vartype(name##_in, argtype, variable);


#define PXMLRPC_FUNC_MULTI_ARGS(name) \
  PXMLRPC_STRUCT_BEGIN(name##_in)


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_MULTI_REPLY(name) \
  }; PXMLRPC_STRUCT_BEGIN(name##_out)


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_NO_ARGS(name) \
  }; \
  bool name(name##_out & reply) \
    { return MakeRequest(#name, name##_in(), reply); }


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_STRUCT_ARG(name) \
  }; \
  class name##_in_carrier : public PXMLRPCStructBase { \
    public: name##_in_carrier(const name##_in & var) : variable(var) { EndConstructor(); } \
    private: struct var_class : public PXMLRPCVariableBase { \
      var_class(const name##_in & var) \
        : PXMLRPCVariableBase("variable", "struct"), instance(var) { } \
      virtual void PrintOn (ostream & s) const { s << instance; } \
      virtual PXMLRPCStructBase * GetStruct(PINDEX) const { return (PXMLRPCStructBase *)&instance; } \
      virtual void Copy(const PXMLRPCVariableBase &) { } \
      const name##_in & instance; \
    } variable; \
  }; \
  bool name(const name##_in & args, name##_out & reply) \
    { return MakeRequest(#name, name##_in_carrier(args), reply); }


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_NORM_ARGS(name) \
  }; \
  bool name(const name##_in & args, name##_out & reply) \
    { return MakeRequest(#name, args, reply); }



/////////////////////////////////////////////////////////////////

#endif // P_XMLRPC

#endif // PTLIB_PXMLRPC_H


// End Of File ///////////////////////////////////////////////////////////////
