/*
 * asnper.h
 *
 * Abstract Syntax Notation Encoding Rules classes
 *
 * Portable Tools Library
 *
 */

#ifdef P_INCLUDE_PER


/** Class for ASN Packed Encoding Rules stream.
*/
class PPER_Stream : public PASN_Stream
{
    PCLASSINFO(PPER_Stream, PASN_Stream);
  public:
    PPER_Stream(int aligned = true);
    PPER_Stream(const uint8_t * buf, PINDEX size, bool aligned = true);
    PPER_Stream(const PBYTEArray & bytes, bool aligned = true);

    PPER_Stream & operator=(const PBYTEArray & bytes);

    unsigned GetBitsLeft() const;

    virtual bool Read(PChannel & chan) override;
    virtual bool Write(PChannel & chan) override;

    virtual bool NullDecode(PASN_Null &) override;
    virtual void NullEncode(const PASN_Null &) override;
    virtual bool BooleanDecode(PASN_Boolean &) override;
    virtual void BooleanEncode(const PASN_Boolean &) override;
    virtual bool IntegerDecode(PASN_Integer &) override;
    virtual void IntegerEncode(const PASN_Integer &) override;
    virtual bool EnumerationDecode(PASN_Enumeration &) override;
    virtual void EnumerationEncode(const PASN_Enumeration &) override;
    virtual bool RealDecode(PASN_Real &) override;
    virtual void RealEncode(const PASN_Real &) override;
    virtual bool ObjectIdDecode(PASN_ObjectId &) override;
    virtual void ObjectIdEncode(const PASN_ObjectId &) override;
    virtual bool BitStringDecode(PASN_BitString &) override;
    virtual void BitStringEncode(const PASN_BitString &) override;
    virtual bool OctetStringDecode(PASN_OctetString &) override;
    virtual void OctetStringEncode(const PASN_OctetString &) override;
    virtual bool ConstrainedStringDecode(PASN_ConstrainedString &) override;
    virtual void ConstrainedStringEncode(const PASN_ConstrainedString &) override;
    virtual bool BMPStringDecode(PASN_BMPString &) override;
    virtual void BMPStringEncode(const PASN_BMPString &) override;
    virtual bool ChoiceDecode(PASN_Choice &) override;
    virtual void ChoiceEncode(const PASN_Choice &) override;
    virtual bool ArrayDecode(PASN_Array &) override;
    virtual void ArrayEncode(const PASN_Array &) override;
    virtual bool SequencePreambleDecode(PASN_Sequence &) override;
    virtual void SequencePreambleEncode(const PASN_Sequence &) override;
    virtual bool SequenceKnownDecode(PASN_Sequence &, PINDEX, PASN_Object &) override;
    virtual void SequenceKnownEncode(const PASN_Sequence &, PINDEX, const PASN_Object &) override;
    virtual bool SequenceUnknownDecode(PASN_Sequence &) override;
    virtual void SequenceUnknownEncode(const PASN_Sequence &) override;

    bool IsAligned() const { return aligned; }

    bool SingleBitDecode();
    void SingleBitEncode(bool value);

    bool MultiBitDecode(unsigned nBits, unsigned & value);
    void MultiBitEncode(unsigned value, unsigned nBits);

    bool SmallUnsignedDecode(unsigned & value);
    void SmallUnsignedEncode(unsigned value);

    bool LengthDecode(unsigned lower, unsigned upper, unsigned & len);
    void LengthEncode(unsigned len, unsigned lower, unsigned upper);

    bool UnsignedDecode(unsigned lower, unsigned upper, unsigned & value);
    void UnsignedEncode(int value, unsigned lower, unsigned upper);

    void AnyTypeEncode(const PASN_Object * value);

  protected:
    bool aligned;
};

#endif


// End Of File ///////////////////////////////////////////////////////////////
