/*
 * pconfig.cxx
 *
 * Operating System utilities.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>

#if P_CONFIG_FILE

#include <ctype.h>


///////////////////////////////////////////////////////////////////////////////
// PConfig

const PString & PConfig::DefaultSectionName() { static PConstString const s("Options"); return s; }

PString PConfig::GetEnv(const char* key, const char* dflt)
{
  #if defined(P_LINUX)
    const char * env = getenv(key);
    if (!env)
      return dflt;
  #else
    size_t required;
    getenv_s(&required, NULL, 0, key);
    if (required == 0)
      return dflt;

    PString env;
    getenv_s(&required, env.GetPointerAndSetLength(required), required+1, key);
  #endif

  return env;
}


PStringToString PConfig::GetAllKeyValues(const PString & section) const
{
  PStringToString dict;

  PStringArray keys = GetKeys(section);
  for (PINDEX i = 0; i < keys.GetSize(); i++)
    dict.SetAt(keys[i], GetString(section, keys[i], ""));

  return dict;
}


#if !defined(_WIN32)

bool PConfig::GetBoolean(const PString & section, const PString & key, bool dflt) const
{
  PString str = GetString(section, key, dflt ? "T" : "F").ToUpper();
  return str[0] == 'T' || str[0] == 'Y' || str.AsInteger() != 0;
}


void PConfig::SetBoolean(const PString & section, const PString & key, bool value)
{
  SetString(section, key, value ? "True" : "False");
}


long PConfig::GetInteger(const PString & section, const PString & key, long dflt) const
{
  PString str(PString::Signed, dflt);
  return GetString(section, key, str).AsInteger();
}


void PConfig::SetInteger(const PString & section, const PString & key, long value)
{
  PString str(PString::Signed, value);
  SetString(section, key, str);
}

#endif


int64_t PConfig::GetInt64(const PString & section, const PString & key, int64_t dflt) const
{
  PString str = GetString(section, key, "");
  if (!str.IsEmpty())
    return str.AsInt64();
  return dflt;
}


void PConfig::SetInt64(const PString & section, const PString & key, int64_t value)
{
  PStringStream strm;
  strm << value;
  SetString(section, key, strm);
}


double PConfig::GetReal(const PString & section, const PString & key, double dflt) const
{
  PString str(PString::Printf, "%g", dflt);
  return GetString(section, key, str).AsReal();
}


void PConfig::SetReal(const PString & section, const PString & key, double value)
{
  PString str(PString::Printf, "%g", value);
  SetString(section, key, str);
}


PTime PConfig::GetTime(const PString & section, const PString & key) const
{
  return GetString(section, key, "1 Jan 1996");
}


PTime PConfig::GetTime(const PString & section, const PString & key, const PTime & dflt) const
{
  return GetString(section, key, dflt.AsString());
}


void PConfig::SetTime(const PString & section, const PString & key, const PTime & value)
{
  SetString(section, key, value.AsString());
}

#endif // P_CONFIG_FILE


// End Of File ///////////////////////////////////////////////////////////////
