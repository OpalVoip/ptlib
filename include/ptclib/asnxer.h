/*
 * asnxer.h
 *
 * Abstract Syntax Notation Encoding Rules classes
 *
 * Portable Windows Library
 *
 */

#ifdef P_INCLUDE_XER

/** Class for ASN XML Encoding Rules stream.
*/
class PXER_Stream : public PASN_Stream
{
    PCLASSINFO(PXER_Stream, PASN_Stream);
  public:
    PXER_Stream(PXMLElement * elem);
    PXER_Stream(PXMLElement * elem, const PBYTEArray & bytes);
    PXER_Stream(PXMLElement * elem, const BYTE * buf, PINDEX size);

    virtual PBoolean Read(PChannel & chan) override;
    virtual PBoolean Write(PChannel & chan) override;

    virtual PBoolean NullDecode(PASN_Null &) override;
    virtual void NullEncode(const PASN_Null &) override;
    virtual PBoolean BooleanDecode(PASN_Boolean &) override;
    virtual void BooleanEncode(const PASN_Boolean &) override;
    virtual PBoolean IntegerDecode(PASN_Integer &) override;
    virtual void IntegerEncode(const PASN_Integer &) override;
    virtual PBoolean EnumerationDecode(PASN_Enumeration &) override;
    virtual void EnumerationEncode(const PASN_Enumeration &) override;
    virtual PBoolean RealDecode(PASN_Real &) override;
    virtual void RealEncode(const PASN_Real &) override;
    virtual PBoolean ObjectIdDecode(PASN_ObjectId &) override;
    virtual void ObjectIdEncode(const PASN_ObjectId &) override;
    virtual PBoolean BitStringDecode(PASN_BitString &) override;
    virtual void BitStringEncode(const PASN_BitString &) override;
    virtual PBoolean OctetStringDecode(PASN_OctetString &) override;
    virtual void OctetStringEncode(const PASN_OctetString &) override;
    virtual PBoolean ConstrainedStringDecode(PASN_ConstrainedString &) override;
    virtual void ConstrainedStringEncode(const PASN_ConstrainedString &) override;
    virtual PBoolean BMPStringDecode(PASN_BMPString &) override;
    virtual void BMPStringEncode(const PASN_BMPString &) override;
    virtual PBoolean ChoiceDecode(PASN_Choice &) override;
    virtual void ChoiceEncode(const PASN_Choice &) override;
    virtual PBoolean ArrayDecode(PASN_Array &) override;
    virtual void ArrayEncode(const PASN_Array &) override;
    virtual PBoolean SequencePreambleDecode(PASN_Sequence &) override;
    virtual void SequencePreambleEncode(const PASN_Sequence &) override;
    virtual PBoolean SequenceKnownDecode(PASN_Sequence &, PINDEX, PASN_Object &) override;
    virtual void SequenceKnownEncode(const PASN_Sequence &, PINDEX, const PASN_Object &) override;
    virtual PBoolean SequenceUnknownDecode(PASN_Sequence &) override;
    virtual void SequenceUnknownEncode(const PASN_Sequence &) override;

    PXMLElement * GetCurrentElement()                   { return position; }
    PXMLElement * SetCurrentElement(PXMLElement * elem) { return position = elem; }

  protected:
    PXMLElement * position;
};

#endif


// End Of File ///////////////////////////////////////////////////////////////
