/*
 * v8script.h
 *
 * Interface library for V8 Javascript interpreter
 *
 * Portable Tools Library
 *
 * Copyright (C) 2012 by Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 */

#ifndef PTLIB_V8SCRIPT_H
#define PTLIB_V8SCRIPT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_V8

#include <ptclib/script.h>
#include <ptclib/vartype.h>


//////////////////////////////////////////////////////////////

/** JavaScript language interpreter
 */
class PJavaScript : public PScriptLanguage
{
  PCLASSINFO(PJavaScript, PScriptLanguage)

  public:
  /**@name Construction */
  //@{
    /**Create a context in which to execute a JavaScript script
     */
    PJavaScript();

    /// Destroy the JavaScript interpreter context
    ~PJavaScript();
  //@}

  /**@name Scripting functions */
  //@{
    /// Get the name of this scripting language
    static PString LanguageName();

    /// Get the name of this scripting language
    virtual PString GetLanguageName() const override;

    /// Indicate language has initialised successfully
    virtual bool IsInitialised() const override;

    /** Load a JavaScript script text.
      */
    virtual bool LoadText(
      const PString & text  ///< Script text to load.
    ) override;

    /**Run the script.
       If \p script is NULL or empty then the currently laoded script is
       executed. If \p script is an existing file, then that will be loaded
       and executed. All other cases the string is laoded as direct script
       text and executed.
      */
    virtual bool Run(
      const char * script = NULL
    ) override;


    /**Create a composite structure.
       The exact semantics is language dependant.
       For JavaScript it is either an object or an array.

       See class description for how \p name is parsed.
      */
    virtual bool CreateComposite(
      const PString & name,       ///< Name of new composite structure
      unsigned sequenceSize = 0   ///< Size of sequence, 0 means object
    ) override;

    /**Get the type of the variable.
      */
    virtual VarTypes GetVarType(
      const PString & name  ///< Name of variable
    ) override;

    /**Get a variable in the script 
       See class description for how \p name is parsed.
      */
    virtual bool GetVar(
      const PString & name,  ///< Name of global
      PVarType & var
    ) override;

    /**Set a variable in the script 
       See class description for how \p name is parsed.
      */
    virtual bool SetVar(
      const PString & name, ///< Name of global
      const PVarType & var
    ) override;

    /**Release a variable name.
       Note the exact semantics is language dependant. It generally applies
       to global variables as most languages have automatic garbage collection
       for other variable types.
      */
    virtual bool ReleaseVariable(
      const PString & name    ///< Name of table to delete
    ) override;

    /**Call a specific function in the script.
       The \p sigString indicates the types of the arguments and return values
       for the function. The types available are:
         'b' for boolean,
         'i' for integer,
         'n' for a number (double float)
         's' for string (const char * or char *)
         'p' for user defined (void *)

       A '>' separates arguments from return values. The same letters are used
       for the tpes, but a pointer to the variable is supplied in the argument
       list, as for scanf. Note there can be multiple return values.

       if 's' is used as a return value, then the caller is expected to delete
       the returned string pointer as it is allocated on the heap.

       If \p sigString is NULL or empty then a void parameterless function is
       called.

       The second form with \p signature alows for the caller to adaptively
       respond to different return types.

       See class description for how \p name is parsed.

       @returns false if function does not exist.
      */
    bool Call(
      const PString & name,           ///< Name of function to execute.
      const char * sigString = NULL,  ///< Signature of arguments following
      ...
    ) override;
    bool Call(
      const PString & name,       ///< Name of function to execute.
      Signature & signature ///< Signature of arguments following
    ) override;


    /**Set a notifier as a JavaScript callable function.
       See class description for how \p name is parsed.
      */
    bool SetFunction(
      const PString & name,         ///< Name of function JavaScript script can call
      const FunctionNotifier & func ///< Notifier excuted
    ) override;
  //@}

  protected:
    struct Private;
    Private * m_private;
    PString   m_resultText;
    PString   m_scriptToRun;
};


#endif // P_V8

#endif  // PTLIB_V8SCRIPT_H

