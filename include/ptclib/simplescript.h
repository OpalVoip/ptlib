/*
 * simplescript.h
 *
 * Ultra simple script language
 * No functions, only string variables, can only concatenate string expressions.
 *
 * Portable Tools Library
 *
 * Copyright (C) 2021 by Vox Lucida Pty. Ltd.
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
 * Contributor(s): Craig Southeren
 *                 Robert Jongbloed
 */

#ifndef PTLIB_SIMPLESCRIPT_H
#define PTLIB_SIMPLESCRIPT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/script.h>


//////////////////////////////////////////////////////////////

/**A wrapper around a ultra simple scripting language instance.
 */
class PSimpleScript : public PScriptLanguage
{
  PCLASSINFO(PSimpleScript, PScriptLanguage)
  public:
  /**@name Construction */
  //@{
    /**Create a context in which to execute a script.
     */
    PSimpleScript();
  //@}

  /**@name Scripting functions */
  //@{
    /// Get the name of this scripting language
    static PString LanguageName();

    /// Get the name of this scripting language
    virtual PString GetLanguageName() const override;

    /// Indicate language has initialised successfully
    virtual bool IsInitialised() const override;

    /**Load a script from a file.
      */
    virtual bool LoadFile(
      const PFilePath & filename  ///< Name of script file to load
    ) override;

    /** Load script text.
      */
    virtual bool LoadText(
      const PString & text  ///< Script text to load.
    ) override;

    /**Run the script.
       If \p script is NULL or empty then the currently laoded script is
       executed. If \p script is an existing file, then that will be loaded
       and executed. All other cases the string is loaded as direct script
       text and executed.
      */
    virtual bool Run(
      const char * script = NULL
    ) override;

    /**Create a composite structure.
       The exact semantics is language dependant.
       For Lua this is always a table.
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
    virtual bool Call(
      const PString & name,           ///< Name of function to execute.
      const char * sigString = NULL,  ///< Signature of arguments following
      ...
    ) override;
    virtual bool Call(
      const PString & name,       ///< Name of function to execute.
      Signature & signature ///< Signature of arguments following
    ) override;

    /**Set a notifier as a script callable function.
       See class description for how \p name is parsed.
      */
    virtual bool SetFunction(
      const PString & name,         ///< Name of function script can call
      const FunctionNotifier & func ///< Notifier excuted
    ) override;
  //@}

    const PStringToString & GetAllVariables() const { return m_variables; }

  protected:
    PString * InternalFindVar(const PString & varName) const;
    PString InternalEvaluateExpr(const PString & expr);
    PStringToString m_variables;
};
#endif  // PTLIB_SIMPLESCRIPT_H

