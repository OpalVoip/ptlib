/*
 * main.h
 *
 * PWLib application header file for DNSTest
 *
 * Copyright 2003 Equivalence
 *
 */

#ifndef _DNSTest_MAIN_H
#define _DNSTest_MAIN_H




class DNSTest : public PProcess
{
  PCLASSINFO(DNSTest, PProcess)

  public:
    DNSTest();
    virtual void Main() override;
};


#endif  // _DNSTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
