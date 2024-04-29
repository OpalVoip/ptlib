/*
 * main.h
 *
 * PWLib application header file for XMLRPCApp
 *
 * Copyright 2002 Equivalence
 *
 */

#ifndef _XMLRPCApp_MAIN_H
#define _XMLRPCApp_MAIN_H

#include <ptlib/pprocess.h>

class XMLRPCApp : public PProcess
{
  PCLASSINFO(XMLRPCApp, PProcess)

  public:
    XMLRPCApp();
    virtual void Main() override;
};


#endif  // _XMLRPCApp_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
