#pragma once
#include "local_multiplayer.h"
// Include after the original declarations in the generated entrypoint header.
// Redirect direct guest calls AND both guest-function registration tables.
// Original imports remain accessible from local_multiplayer.cpp for player 1.
#define __imp__XamUserGetSigninState BBR_XamUserGetSigninState
#define __imp__XamUserGetName BBR_XamUserGetName
#define __imp__XamUserGetXUID BBR_XamUserGetXUID
#define __imp__XamUserReadProfileSettings BBR_XamUserReadProfileSettings
#define __imp__XamUserWriteProfileSettings BBR_XamUserWriteProfileSettings
#define __imp__XamUserCheckPrivilege BBR_XamUserCheckPrivilege
#define __imp__XamShowSigninUI BBR_XamShowSigninUI
#define __imp__XamInputGetState BBR_XamInputGetState
