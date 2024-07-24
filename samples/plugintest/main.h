/*
 * main.h
 *
 * PWLib application header file for PluginTest
 *
 * Copyright 2003 Equivalence
 *
 */

#ifndef _PluginTest_MAIN_H
#define _PluginTest_MAIN_H




class PluginTest : public PProcess
{
  PCLASSINFO(PluginTest, PProcess)

  public:
    PluginTest();
    virtual void Main() override;
};


#endif  // _PluginTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
