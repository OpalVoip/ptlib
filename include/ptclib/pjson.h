/*
 * pjson.h
 *
 * JSON parser
 *
 * Portable Tools Library
 *
 * Copyright (C) 2015 by Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed
 *                 Blackboard, inc
 */

#ifndef PTLIB_PJSON_H
#define PTLIB_PJSON_H

#include <ptlib.h>

class PJSON : public PObject
{
  PCLASSINFO(PJSON, PObject)
  public:
    enum Types
    {
      e_Object,
      e_Array,
      e_String,
      e_Number,
      e_Boolean,
      e_Null
    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    class Base : PNonCopyable
    {
      public:
        Base() { }
        virtual ~Base() { }
        virtual bool IsType(Types type) const = 0;
        virtual void ReadFrom(istream & strm) = 0;
        virtual void PrintOn(ostream & strm) const = 0;
        virtual Base * DeepClone() const = 0;

      friend ostream & operator<<(ostream & s, const Base & b) { b.PrintOn(s); return s; }
    };

    class Array;

    typedef long double NumberType;

    class Object : public Base, public std::map<std::string, Base *>
    {
      public:
        Object() { }
        ~Object();
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;

        bool IsType(const PString & name, Types type) const;

        template <class T> const T * Get(const PString & name) const
        {
          const_iterator it = find(name);
          return it != end() ? dynamic_cast<const T *>(it->second) : NULL;
        }
        template <class T> T * Get(const PString & name)
        {
          iterator it = find(name);
          return it != end() ? dynamic_cast<T *>(it->second) : NULL;
        }
        const Object & GetObject(const PString & name) const { return *Get<Object>(name); }
              Object & GetObject(const PString & name)       { return *Get<Object>(name); }
        const Array & GetArray(const PString & name)   const { return *Get<Array>(name); }
              Array & GetArray(const PString & name)         { return *Get<Array>(name); }

        PString GetString(const PString & name) const;
        int GetInteger(const PString & name) const;
        int64_t GetInteger64(const PString & name) const;
        unsigned GetUnsigned(const PString & name) const;
        uint64_t GetUnsigned64(const PString & name) const;
        NumberType GetNumber(const PString & name) const;
        bool GetBoolean(const PString & name) const;
        PTime GetTime(const PString & name) const;
        PTimeInterval GetInterval(const PString & name) const;

        bool Set(const PString & name, Types type);
        bool Set(const PString & name, const Base & toInsert);
        bool Set(const PString & name, const PJSON & toInsert);
        Object & SetObject(const PString & name);
        Array & SetArray(const PString & name);
        bool SetString(const PString & name, const PString & value);
        bool SetNumber(const PString & name, NumberType value);
        bool SetBoolean(const PString & name, bool value);
        bool SetTime(const PString & name, const PTime & value);
        bool SetInterval(const PString & name, const PTimeInterval & value);

        bool Remove(const PString & name);
    };

    class Array : public Base, public std::vector<Base *>
    {
      public:
        Array() { }
        ~Array();
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;

        bool IsType(size_t index, Types type) const;

        template <class T> const T * Get(size_t index) const { return index < size() ? dynamic_cast<const T *>(at(index)) : NULL; }
        template <class T>       T * Get(size_t index)       { return index < size() ? dynamic_cast<      T *>(at(index)) : NULL; }
        const Object & GetObject(size_t index)         const { return *Get<Object>(index); }
              Object & GetObject(size_t index)               { return *Get<Object>(index); }
        const Array & GetArray(size_t index)           const { return *Get<Array>(index); }
              Array & GetArray(size_t index)                 { return *Get<Array>(index); }

        PString GetString(size_t index) const;
        int GetInteger(size_t index) const;
        int64_t GetInteger64(size_t index) const;
        unsigned GetUnsigned(size_t index) const;
        uint64_t GetUnsigned64(size_t index) const;
        NumberType GetNumber(size_t index) const;
        bool GetBoolean(size_t index) const;
        PTime GetTime(size_t index) const;
        PTimeInterval GetInterval(size_t index) const;

        void Append(Types type);
        void Append(const Base & toAppend);
        void Append(const PJSON & toAppend);
        Object & AppendObject();
        Array & AppendArray();
        void AppendString(const PString & value);
        void AppendNumber(NumberType value);
        void AppendBoolean(bool value);
        void AppendTime(const PTime & value);
        void AppendInterval(const PTimeInterval & value);

        bool Remove(size_t index);
    };

    class String : public Base, public PString
    {
      public:
        String(const char * str = NULL) : PString(str) { }
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;
        String & operator=(const char * str) { PString::operator=(str); return *this; }
        String & operator=(const PString & str) { PString::operator=(str); return *this; }
    };

    class Number : public Base
    {
      protected:
        NumberType m_value;
      public:
        explicit Number(NumberType value = 0);
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;
        Number & operator=(NumberType value) { m_value = value; return *this; }
        void SetValue(NumberType value) { m_value = value; }
        NumberType GetValue() const { return m_value; }
    };

    class Boolean : public Base
    {
      protected:
        bool m_value;
      public:
        explicit Boolean(bool value = false);
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;
        Boolean & operator=(bool value) { m_value = value; return *this; }
        void SetValue(bool value) { m_value = value; }
        bool GetValue() const { return m_value; }
    };

    class Null : public Base
    {
      public:
        virtual bool IsType(Types type) const override;
        virtual void ReadFrom(istream & strm) override;
        virtual void PrintOn(ostream & strm) const override;
        virtual Base * DeepClone() const override;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////
    ///< Constructor
    PJSON();
    explicit PJSON(Types type);
    explicit PJSON(const PString & str);
    PJSON(const PJSON & other);
    PJSON & operator=(const PJSON & other);

    ~PJSON() { delete m_root; }

    virtual void ReadFrom(istream & strm) override;
    virtual void PrintOn(ostream & strm) const override;

    bool FromString(
      const PString & str
    );

    PString AsString(
      std::streamsize initialIndent = 0,
      std::streamsize subsequentIndent = 0
    ) const;

    bool IsValid() const { return m_valid; }

    bool IsType(Types type) const { return PAssertNULL(m_root) && m_root->IsType(type); }

    void Set(Types type);

    template <class T> const T & GetAs() const { return PConstCastPtrToRef<T, Base>(m_root); }
    template <class T>       T & GetAs()       { return PCastPtrToRef<T, Base>(m_root); }
    const Object  & GetObject()  const { return GetAs<Object>();  }
          Object  & GetObject()        { return GetAs<Object>();  }
    const Array   & GetArray ()  const { return GetAs<Array>();   }
          Array   & GetArray ()        { return GetAs<Array>();   }
    const String  & GetString()  const { return GetAs<String>();  }
          String  & GetString()        { return GetAs<String>();  }
    const Number  & GetNumber()  const { return GetAs<Number>();  }
          Number  & GetNumber()        { return GetAs<Number>();  }
    const Boolean & GetBoolean() const { return GetAs<Boolean>(); }
          Boolean & GetBoolean()       { return GetAs<Boolean>(); }

  protected:
    Base * m_root;
    bool   m_valid;
};

////////////////////////////////////////////////////////////////////////////////////////////

class PJSONWrapMember;

class PJSONRecord : public PObject
{
  PCLASSINFO(PJSONRecord, PObject)
  friend class PJSONWrapMember;
protected:
  std::map<PString, PJSONWrapMember *> m_jsonDataMembers;
  bool FromJSON(const PJSON::Object & obj);
  void AsJSON(PJSON::Object & obj) const;
public:
  PJSONRecord();
  PJSONRecord(const PJSONRecord &);
  virtual ~PJSONRecord() { }

  virtual void ReadFrom(istream & strm) override;
  virtual void PrintOn(ostream & strm) const override;

  bool FromString(
    const PString & str
  );

  PString AsString(
    std::streamsize initialIndent = 0,
    std::streamsize subsequentIndent = 0
  ) const;

  bool FromJSON(
    const PJSON & json
  );

  void AsJSON(
    PJSON & json
  ) const;
};

class PJSONWrapMember
{
public:
  PJSONWrapMember(const char * memberName);
  PJSONWrapMember(const PJSONWrapMember & other);
  PJSONWrapMember & operator=(const PJSONWrapMember & other);
  virtual ~PJSONWrapMember() { }

  virtual PJSON::Types GetType() const = 0;
  virtual bool FromJSON(const PJSON::Base & field) = 0;
  virtual void AsJSON(PJSON::Base & field) const = 0;

protected:
  PString m_memberName;
  void EndRecordConstruction();
};

template <typename TYPE> class PJSONMember : public PJSONWrapMember
{
public:
  PJSONMember(const char * name, const TYPE & value = TYPE())
    : PJSONWrapMember(name)
    , m_value(value)
  {
  }

  PJSONMember & operator=(const PJSONMember & other) { PJSONWrapMember::operator=(other); m_value = other.m_value;  return *this; }
  PJSONMember & operator=(const TYPE & value) { m_value = value; return *this; }
  operator TYPE() const { return m_value; }

  virtual PJSON::Types GetType() const { return PJSON::e_String; }
  virtual bool FromJSON(const PJSON::Base & field) { PStringStream s(dynamic_cast<const PJSON::String &>(field)); s >> m_value; return s.good(); }
  virtual void AsJSON(PJSON::Base & field) const { dynamic_cast<PJSON::String &>(field) = PSTRSTRM(m_value); }

protected:
  TYPE m_value;
};

template <> class PJSONMember<PString> : public PJSONWrapMember
{
public:
  PJSONMember(const char * name, const PString & value = PString::Empty()) : PJSONWrapMember(name), m_value(value) { }
  PJSONMember & operator=(const PJSONMember & other) { PJSONWrapMember::operator=(other); m_value = other.m_value;  return *this; }
  PJSONMember & operator=(const PString & value) { m_value = value; return *this; }
  PJSONMember & operator=(const std::string & value) { m_value = value; return *this; }
  PJSONMember & operator=(const char * value) { m_value = value; return *this; }
  operator PString() const { return m_value; }
  virtual PJSON::Types GetType() const { return PJSON::e_String; }
  virtual bool FromJSON(const PJSON::Base & field) { m_value = dynamic_cast<const PJSON::String &>(field); return true; }
  virtual void AsJSON(PJSON::Base & field) const { dynamic_cast<PJSON::String &>(field) = m_value; }
protected:
  PString m_value;
};

template <> class PJSONMember<PTime> : public PJSONWrapMember
{
public:
  PJSONMember(const char * name, const PTime & value = PTime(0)) : PJSONWrapMember(name), m_value(value) { }
  PJSONMember & operator=(const PJSONMember & other) { PJSONWrapMember::operator=(other); m_value = other.m_value;  return *this; }
  PJSONMember & operator=(const PTime & value) { m_value = value; return *this; }
  operator PTime() const { return m_value; }
  virtual PJSON::Types GetType() const { return PJSON::e_String; }
  virtual bool FromJSON(const PJSON::Base & field) { return m_value.Parse(dynamic_cast<const PJSON::String &>(field)); }
  virtual void AsJSON(PJSON::Base & field) const { dynamic_cast<PJSON::String &>(field) = m_value.AsString(PTime::LongISO8601); }
protected:
  PTime m_value;
};

template <> class PJSONMember<bool> : public PJSONWrapMember
{
public:
  PJSONMember(const char * name, const bool value = false) : PJSONWrapMember(name), m_value(value) { }
  PJSONMember & operator=(const PJSONMember & other) { PJSONWrapMember::operator=(other); m_value = other.m_value;  return *this; }
  PJSONMember & operator=(const bool value) { m_value = value; return *this; }
  operator bool() const { return m_value; }
  virtual PJSON::Types GetType() const { return PJSON::e_Boolean; }
  virtual bool FromJSON(const PJSON::Base & field) { m_value = dynamic_cast<const PJSON::Boolean &>(field).GetValue(); return true; }
  virtual void AsJSON(PJSON::Base & field) const { dynamic_cast<PJSON::Boolean &>(field).SetValue(m_value); }
protected:
  bool m_value;
};

template <typename TYPE> class PJSONWrapNumericMember : public PJSONWrapMember
{
public:
  PJSONWrapNumericMember(const char * name, TYPE value) : PJSONWrapMember(name), m_value(value) { }
  operator TYPE() const { return m_value; }
  virtual PJSON::Types GetType() const { return PJSON::e_Number; }
  virtual bool FromJSON(const PJSON::Base & field) { m_value = static_cast<TYPE>(dynamic_cast<const PJSON::Number &>(field).GetValue()); return true; }
  virtual void AsJSON(PJSON::Base & field) const { dynamic_cast<PJSON::Number &>(field).SetValue(static_cast<PJSON::NumberType>(m_value)); }
protected:
  TYPE m_value;
};

#define P_JSON_DECLARE_NUMERIC_MEMBER(TYPE) \
    template <> struct PJSONMember<TYPE> : public PJSONWrapNumericMember<TYPE> { \
        PJSONMember(const char * name, TYPE value = 0) : PJSONWrapNumericMember<TYPE>(name, value) { } \
        PJSONMember & operator=(const PJSONMember & other) { m_value = other.m_value; return *this; } \
        PJSONMember & operator=(TYPE value) { m_value = value; return *this; } \
    }
P_JSON_DECLARE_NUMERIC_MEMBER( int8_t );
P_JSON_DECLARE_NUMERIC_MEMBER(uint8_t );
P_JSON_DECLARE_NUMERIC_MEMBER( int16_t);
P_JSON_DECLARE_NUMERIC_MEMBER(uint16_t);
P_JSON_DECLARE_NUMERIC_MEMBER( int32_t);
P_JSON_DECLARE_NUMERIC_MEMBER(uint32_t);
P_JSON_DECLARE_NUMERIC_MEMBER( int64_t);
P_JSON_DECLARE_NUMERIC_MEMBER(uint64_t);
#if _MSC_VER
P_JSON_DECLARE_NUMERIC_MEMBER(long);
P_JSON_DECLARE_NUMERIC_MEMBER(unsigned long);
#endif
P_JSON_DECLARE_NUMERIC_MEMBER(float);
P_JSON_DECLARE_NUMERIC_MEMBER(double);
P_JSON_DECLARE_NUMERIC_MEMBER(long double);
#undef P_JSON_DECLARE_NUMERIC_MEMBER

template <typename TYPE> class PJSONMemberRecord : public PJSONWrapMember, public TYPE
{
public:
  PJSONMemberRecord(const char * name) : PJSONWrapMember(name) { EndRecordConstruction(); }
  PJSONMemberRecord(const char * name, const TYPE & init) : PJSONWrapMember(name), TYPE(init) { EndRecordConstruction(); }
  PJSONMemberRecord(const PJSONMemberRecord & other) : PJSONWrapMember(other), TYPE(other) { EndRecordConstruction(); }
  PJSONMemberRecord & operator=(const TYPE & other) { TYPE::operator=(other); return *this; }
  virtual PJSON::Types GetType() const { return PJSON::e_Object; }
  virtual bool FromJSON(const PJSON::Base & field) { return TYPE::FromJSON(dynamic_cast<const PJSON::Object &>(field)); }
  virtual void AsJSON(PJSON::Base & field) const { TYPE::AsJSON(dynamic_cast<PJSON::Object &>(field)); }
};

template <typename TYPE, typename WRAP = PJSONMember<TYPE> > class PJSONMemberArray : public PJSONWrapMember, public std::vector<TYPE>
{
public:
  PJSONMemberArray(const char * name) : PJSONWrapMember(name) { }
  PJSONMemberArray(const char * name, const TYPE & init) : PJSONWrapMember(name), std::vector<TYPE>(1, init) { }
  PJSONMemberArray & operator=(const TYPE & other) { this->resize(1); this->front() = other; return *this; }
  virtual PJSON::Types GetType() const { return PJSON::e_Array; }
  virtual bool FromJSON(const PJSON::Base & field)
  {
    const PJSON::Array & arr = dynamic_cast<const PJSON::Array &>(field);
    this->resize(arr.size());
    bool good = true;
    for (size_t i = 0; i < this->size(); ++i) {
      WRAP wrap(NULL, this->at(i));
      if (wrap.FromJSON(*arr[i]))
        this->at(i) = wrap;
      else
        good = false;
    }
    return good;
  }
  virtual void AsJSON(PJSON::Base & field) const
  {
    PJSON::Array & arr = dynamic_cast<PJSON::Array &>(field);
    for (size_t i = 0; i < this->size(); ++i) {
      WRAP const wrap(NULL, this->at(i));
      arr.Append(wrap.GetType());
      wrap.AsJSON(*arr.back());
    }
  }
};

template <typename TYPE> class PJSONMemberArrayRecord : public PJSONMemberArray<TYPE, PJSONMemberRecord<TYPE> >
{
public:
  PJSONMemberArrayRecord(const char * name) : PJSONMemberArray<TYPE, PJSONMemberRecord<TYPE> >(name) { }
  PJSONMemberArrayRecord(const char * name, const TYPE & init) : PJSONMemberArray<TYPE, PJSONMemberRecord<TYPE> >(name, init) { }
  PJSONMemberArrayRecord & operator=(const TYPE & other) { this->resize(1); this->front() = other; return *this; }
};

#define P_JSON_MEMBER_ARG3(TYPE, member, BASE) \
    struct PJSON_##member : BASE<TYPE> { \
      PJSON_##member()           : BASE<TYPE>(#member) { } \
      PJSON_##member(TYPE value) : BASE<TYPE>(#member, value) { } \
      PJSON_##member & operator=(const TYPE other) { BASE<TYPE>::operator=(other); return *this; } \
    } member
#define P_JSON_MEMBER_ARG2(TYPE, member) P_JSON_MEMBER_ARG3(TYPE, member, ::PJSONMember)
#define P_JSON_MEMBER_PART2(narg, args) P_JSON_MEMBER_ARG##narg args
#define P_JSON_MEMBER_PART1(narg, args) P_JSON_MEMBER_PART2(narg, args)
#define P_JSON_MEMBER(...) P_JSON_MEMBER_PART1(PARG_COUNT(__VA_ARGS__), (__VA_ARGS__))


#if P_SSL

/** Encode/Decode JSON payload as a JSON Web Token.
    The JSON is always of Object type.
  */
class PJWT : public PJSON
{
    PCLASSINFO(PJWT, PJSON);
  public:
    /// Construct empty Object type JSON
    PJWT();

    /** Create and decode the JWT.
        Use IsValid() afterward to cerify if decode was successful.
     */
    explicit PJWT(
      const PString & str,
      const PString & secret = PString::Empty(),
      const PTime & verifyTime = PTime(0)
    );

    /// Available token algorithms 
    P_DECLARE_STREAMABLE_ENUM(Algorithm,
      none,
      HS256,  // HMAC SHA-256
      HS384,  // HMAC SHA-384
      HS512   // HMAC SHA-512
    );

    /**Encode the JWT using the shared secret and algorithm.
      */
    PString Encode(
      const PString & secret = PString::Empty(),  ///< Shared secret
      const Algorithm algorithm = HS256           ///< Algorithm
    );

    /**Decode the JWT using the shared secret and algorithm.
      */
    bool Decode(
      const PString & str,                        ///< Encoded JWT string
      const PString & secret = PString::Empty(),  ///< Shared secret
      const PTime & verifyTime = PTime(0)         ///< Optional time to use for verification
    );

    void SetIssuer(const PString & str);
    PString GetIssuer() const;
    void SetSubject(const PString & str);
    PString GetSubject() const;
    void SetAudience(const PString & str);
    PString GetAudience() const;
    void SetExpiration(const PTime & when);
    PTime GetExpiration() const;
    void SetNotBefore(const PTime & when);
    PTime GetNotBefore() const;
    void SetIssuedAt(const PTime & when);
    PTime GetIssuedAt() const;
    void SetTokenId(const PString & str);
    PString GetTokenId() const;

    void SetPrivate(const PString & key, const PString & str);
    PString GetPrivate(const PString & key) const;
};

#endif // P_SSL

#endif  // PTLIB_PJSON_H
