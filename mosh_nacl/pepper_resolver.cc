// pepper_resolver.cc - DNS resolver from the Pepper API.

// Copyright 2016 Richard Woodbury
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "mosh_nacl/pepper_resolver.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "ppapi/cpp/host_resolver.h"

using std::move;
using std::string;
using std::vector;

void PepperResolver::Resolve(string domain_name, Type type, Callback callback) {
  CallbackCaller caller(callback);

  PP_HostResolver_Hint hint;
  switch (type) {
    case Type::A:
      hint = {PP_NETADDRESS_FAMILY_IPV4, 0};
      break;
    case Type::AAAA:
      hint = {PP_NETADDRESS_FAMILY_IPV6, 0};
      break;
    case Type::SSHFP:
      caller.Call(Error::TYPE_NOT_SUPPORTED, Authenticity::INSECURE, {});
      return;
      // No default case; all enums accounted for.
  }

  caller.Release();
  pp::Module::Get()->core()->CallOnMainThread(
      0,
      cc_factory_.NewCallback(&PepperResolver::ResolveOnMainThread,
                              move(domain_name), hint, callback));
}

void PepperResolver::ResolveOnMainThread(__attribute__((unused))
                                         uint32_t unused,
                                         string domain_name,
                                         PP_HostResolver_Hint hint,
                                         Callback callback) {
  resolver_.Resolve(
      domain_name.c_str(), 0, hint,
      cc_factory_.NewCallback(&PepperResolver::ResolverCallback, callback));
}

void PepperResolver::ResolverCallback(int32_t result, Callback callback) {
  CallbackCaller caller(callback);

  if (result == PP_ERROR_NAME_NOT_RESOLVED) {
    caller.Call(Error::NOT_RESOLVED, Authenticity::INSECURE, {});
    return;
  }
  if (result != PP_OK) {
    return;
  }

  vector<string> results;
  for (int i = 0; i < resolver_.GetNetAddressCount(); ++i) {
    results.push_back(
        resolver_.GetNetAddress(i).DescribeAsString(false).AsString());
  }
  caller.Call(Error::OK, Authenticity::INSECURE, move(results));
}
