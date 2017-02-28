//
// Copyright Voipster 2006. Use, modification and distribution
// is subject to the Voipster Software License, Version
// 1.0. (See accompanying file VOIPSTER_LICENSE_1_0)
//



// gsm0610iq.cpp
//
//

#include <confp.hpp>
#include "audio_codec.hpp"
#include "enforcep.hpp"
#include <string>
#include <loki/Factory.h>

namespace {

typedef audio::codec::Interface* (*ImplCreator)();

typedef Loki::Factory
<
  audio::codec::Interface  /* AbstractProduct */,
  std::string              /* IdentifierType */,
  ImplCreator              /* ProductGenerator */
> ImplementationFactory;

///////////////////////////////////////////////////////////////////////////////

class ThisPlug : public audio::codec::Plug {
public :
  ThisPlug();
  ~ThisPlug();

  audio::codec::ImplPtr CreateImplementation(const std::string& name);

private:
  ImplementationFactory implementationFactory_;
};

ThisPlug thisPlug;

} // of the anonymous namespace

IMPLEMENT_PLUG(thisPlug)

//extern Src::Interface* CreateStub();
//extern Src::Interface* CreateUnit();
extern audio::codec::Interface* CreateImpl1();

namespace {

#define REGISTER(FUNCTION_NAME) \
        ENFORCE(implementationFactory_.Register(STRINGIZE(FUNCTION_NAME), \
                                                Create##FUNCTION_NAME));
ThisPlug::ThisPlug()
{
//  REGISTER(Stub);
//  REGISTER(Unit);
  REGISTER(Impl1);
}

#define UNREGISTER(FUNCTION_NAME) \
        ENFORCE(implementationFactory_.Unregister(STRINGIZE(FUNCTION_NAME)));

ThisPlug::~ThisPlug()
{
  UNREGISTER(Impl1);
//  UNREGISTER(Unit);
//  UNREGISTER(Stub);
};

audio::codec::ImplPtr ThisPlug::CreateImplementation( const std::string& name)
{
  audio::codec::ImplPtr impl;

  try
  {
    impl = audio::codec::ImplPtr(implementationFactory_.CreateObject(name));
  }
  catch(const ImplementationFactory::Exception&)
  {
    PLUG_RAISE(audio::codec::FactoryError(name));
  }

  return impl;
}

} // of the anonymous namespace

// End Of File

