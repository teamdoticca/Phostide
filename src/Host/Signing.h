#pragma once

#include <string>

namespace phostide {

// Subject DN of a valid Authenticode signer (timestamped signatures stay valid after
// the short-lived Trusted Signing cert expires), or empty if unsigned / invalid.
// Offline only (cached URL retrieval): never blocks on the network.
std::wstring AuthenticodeSignerSubject(const std::wstring& file);

// Signer subject of this process's executable (cached); empty for dev builds.
const std::wstring& HostSignerSubject();

}  // namespace phostide
