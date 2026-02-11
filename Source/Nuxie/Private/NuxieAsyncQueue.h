#pragma once

#include "Templates/Function.h"

NUXIE_API void NuxieRunOnGameThread(TFunction<void()> Work);
