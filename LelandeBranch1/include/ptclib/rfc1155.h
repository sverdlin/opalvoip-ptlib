//
// rfc1155.h
//
// Code automatically generated by asnparse.
//

#ifdef P_SNMP

#ifndef PTLIB_RFC1155_H
#define PTLIB_RFC1155_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/asner.h>

//
// ObjectName
//

class PRFC1155_ObjectName : public PASN_ObjectId
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_ObjectName, PASN_ObjectId);
#endif
  public:
    PRFC1155_ObjectName(unsigned tag = UniversalObjectId, TagClass tagClass = UniversalTagClass);

    PObject * Clone() const;
};


//
// ObjectSyntax
//

class PRFC1155_SimpleSyntax;
class PRFC1155_ApplicationSyntax;

class PRFC1155_ObjectSyntax : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_ObjectSyntax, PASN_Choice);
#endif
  public:
    PRFC1155_ObjectSyntax(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_SimpleSyntax &() const;
#else
    operator PRFC1155_SimpleSyntax &();
    operator const PRFC1155_SimpleSyntax &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_ApplicationSyntax &() const;
#else
    operator PRFC1155_ApplicationSyntax &();
    operator const PRFC1155_ApplicationSyntax &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// SimpleSyntax
//

class PRFC1155_SimpleSyntax : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_SimpleSyntax, PASN_Choice);
#endif
  public:
    PRFC1155_SimpleSyntax(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_number = 2,
      e_string = 4,
      e_object = 6,
      e_empty = 5
    };

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// ApplicationSyntax
//

class PRFC1155_NetworkAddress;
class PRFC1155_Counter;
class PRFC1155_Gauge;
class PRFC1155_TimeTicks;
class PRFC1155_Opaque;

class PRFC1155_ApplicationSyntax : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_ApplicationSyntax, PASN_Choice);
#endif
  public:
    PRFC1155_ApplicationSyntax(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_counter = 1,
      e_gauge,
      e_ticks,
      e_arbitrary
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_NetworkAddress &() const;
#else
    operator PRFC1155_NetworkAddress &();
    operator const PRFC1155_NetworkAddress &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_Counter &() const;
#else
    operator PRFC1155_Counter &();
    operator const PRFC1155_Counter &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_Gauge &() const;
#else
    operator PRFC1155_Gauge &();
    operator const PRFC1155_Gauge &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_TimeTicks &() const;
#else
    operator PRFC1155_TimeTicks &();
    operator const PRFC1155_TimeTicks &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_Opaque &() const;
#else
    operator PRFC1155_Opaque &();
    operator const PRFC1155_Opaque &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// NetworkAddress
//

class PRFC1155_IpAddress;

class PRFC1155_NetworkAddress : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_NetworkAddress, PASN_Choice);
#endif
  public:
    PRFC1155_NetworkAddress(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_internet
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator PRFC1155_IpAddress &() const;
#else
    operator PRFC1155_IpAddress &();
    operator const PRFC1155_IpAddress &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// IpAddress
//

class PRFC1155_IpAddress : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_IpAddress, PASN_OctetString);
#endif
  public:
    PRFC1155_IpAddress(unsigned tag = 0, TagClass tagClass = ApplicationTagClass);

    PRFC1155_IpAddress(const char * v);
    PRFC1155_IpAddress(const PString & v);
    PRFC1155_IpAddress(const PBYTEArray & v);

    PRFC1155_IpAddress & operator=(const char * v);
    PRFC1155_IpAddress & operator=(const PString & v);
    PRFC1155_IpAddress & operator=(const PBYTEArray & v);
    PObject * Clone() const;
};


//
// Counter
//

class PRFC1155_Counter : public PASN_Integer
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_Counter, PASN_Integer);
#endif
  public:
    PRFC1155_Counter(unsigned tag = 1, TagClass tagClass = ApplicationTagClass);

    PRFC1155_Counter & operator=(int v);
    PRFC1155_Counter & operator=(unsigned v);
    PObject * Clone() const;
};


//
// Gauge
//

class PRFC1155_Gauge : public PASN_Integer
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_Gauge, PASN_Integer);
#endif
  public:
    PRFC1155_Gauge(unsigned tag = 2, TagClass tagClass = ApplicationTagClass);

    PRFC1155_Gauge & operator=(int v);
    PRFC1155_Gauge & operator=(unsigned v);
    PObject * Clone() const;
};


//
// TimeTicks
//

class PRFC1155_TimeTicks : public PASN_Integer
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_TimeTicks, PASN_Integer);
#endif
  public:
    PRFC1155_TimeTicks(unsigned tag = 3, TagClass tagClass = ApplicationTagClass);

    PRFC1155_TimeTicks & operator=(int v);
    PRFC1155_TimeTicks & operator=(unsigned v);
    PObject * Clone() const;
};


//
// Opaque
//

class PRFC1155_Opaque : public PASN_OctetString
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(PRFC1155_Opaque, PASN_OctetString);
#endif
  public:
    PRFC1155_Opaque(unsigned tag = 4, TagClass tagClass = ApplicationTagClass);

    PRFC1155_Opaque(const char * v);
    PRFC1155_Opaque(const PString & v);
    PRFC1155_Opaque(const PBYTEArray & v);

    PRFC1155_Opaque & operator=(const char * v);
    PRFC1155_Opaque & operator=(const PString & v);
    PRFC1155_Opaque & operator=(const PBYTEArray & v);
    PObject * Clone() const;
};


#endif // PTLIB_RFC1155_H

#endif // if ! H323_DISABLE_PRFC1155


// End Of File ///////////////////////////////////////////////////////////////
